#include "CoverageInstru.hpp"
namespace wasmInstrumentation {

void CoverageInstru::innerAnalysis(BasicBlockAnalysis &basicBlockAnalysis) const noexcept {
  if (config->skipLib) {
    basicBlockAnalysis.addExclude("~lib/.+");
  }
  Json::Reader jsonReader;
  Json::Value includesJsonValue;
  Json::Value excludesJsonValue;
  if (!config->includes.empty()) {
    jsonReader.parse(std::string(config->includes), includesJsonValue);
    if (includesJsonValue.isArray()) {
      const uint32_t includesJsonSize = includesJsonValue.size();
      for (uint32_t i = 0U; i < includesJsonSize; ++i) {
        basicBlockAnalysis.addInclude(includesJsonValue[i].asString());
      }
    }
  }
  if (!config->excludes.empty()) {
    jsonReader.parse(std::string(config->excludes), excludesJsonValue);
    if (excludesJsonValue.isArray()) {
      const uint32_t excludeJsonSize = excludesJsonValue.size();
      for (uint32_t i = 0U; i < excludeJsonSize; ++i) {
        basicBlockAnalysis.addExclude(excludesJsonValue[i].asString());
      }
    }
  }
}

InstrumentationResponse CoverageInstru::instrument() const noexcept {
  if (config->fileName.empty() || config->reportFunction.empty() || config->sourceMap.empty() ||
      config->targetName.empty() || config->expectInfoOutputFilePath.empty() ||
      config->debugInfoOutputFilePath.empty()) {
    std::cout << *config << std::endl;
    return InstrumentationResponse::CONFIG_ERROR; // config error
  }
  std::filesystem::path filePath(config->fileName);
  std::filesystem::path targetFilePath(config->targetName);
  std::filesystem::path debugInfoPath(config->debugInfoOutputFilePath);
  std::filesystem::path sourceMapPath(config->sourceMap);
  if ((!std::filesystem::exists(filePath)) ||
      (!std::filesystem::exists(debugInfoPath.parent_path())) ||
      (!std::filesystem::exists(sourceMapPath)) ||
      (!std::filesystem::exists(targetFilePath.parent_path()))) {
    std::cout << *config << std::endl;
    return InstrumentationResponse::CONFIG_FILEPATH_ERROR; // config file path error
  }

  Module module;
  ModuleReader reader;

  reader.read(std::string(config->fileName), module, std::string(config->sourceMap));
  BasicBlockAnalysis basicBlockAnalysis = BasicBlockAnalysis();
  innerAnalysis(basicBlockAnalysis);
  BasicBlockWalker basicBlockWalker = BasicBlockWalker(&module, basicBlockAnalysis);
  basicBlockWalker.basicBlockWalk();
  const std::unordered_map<std::string_view, FunctionAnalysisResult> &results =
      basicBlockWalker.getResults();
  Json::Value json;
  Json::Value debugInfoJson;
  Json::Value debugFileJson;
  for (auto &[function, result] : results) {
    Json::Value innerJson;
    innerJson["index"] = result.functionIndex;
    Json::Value branchInfoArray(Json::ValueType::arrayValue);
    for (const auto &branchInfo : result.branchInfo) {
      Json::Value inner_array;
      inner_array.append(branchInfo.first);
      inner_array.append(branchInfo.second);
      branchInfoArray.append(std::move(inner_array));
    }
    innerJson["branchInfo"] = branchInfoArray;
    Json::Value debugLineJson;
    for (const auto &basicBlock : result.basicBlocks) {
      if (basicBlock.basicBlockIndex != static_cast<Index>(-1)) {
        Json::Value debugLineItemJsonArray(Json::ValueType::arrayValue);
        for (const auto &debugLine : basicBlock.debugLocations) {
          Json::Value debugInfo;
          debugInfo.append(debugLine.fileIndex);
          debugInfo.append(debugLine.lineNumber);
          debugInfo.append(debugLine.columnNumber);
          debugLineItemJsonArray.append(std::move(debugInfo));
        }
        debugLineJson[basicBlock.basicBlockIndex] = debugLineItemJsonArray;
      }
    }
    innerJson["lineInfo"] = debugLineJson;
    debugInfoJson[function.data()] = innerJson;
  }
  for (const std::string &debugInfoFileName : module.debugInfoFileNames) {
    debugFileJson.append(debugInfoFileName);
  }
  json["debugInfos"] = debugInfoJson;
  json["debugFiles"] = debugFileJson;
  std::ofstream jsonWriteStream(config->debugInfoOutputFilePath.data(), std::ios::trunc);
  Json::StreamWriterBuilder jsonBuilder;
  jsonBuilder["indentation"] = "";
  std::unique_ptr<Json::StreamWriter> jsonWriter(jsonBuilder.newStreamWriter());
  if (jsonWriter->write(json, &jsonWriteStream) != 0) {
    // Hard to control IO error
    // LCOV_EXCL_START
    return InstrumentationResponse::DEBUG_INFO_GENERATION_ERROR; // debug info json write failed
    // LCOV_EXCL_STOP
  }
  jsonWriteStream.close();
  if (jsonWriteStream.fail() || jsonWriteStream.bad()) {
    // Hard to control IO error
    // LCOV_EXCL_START
    return InstrumentationResponse::DEBUG_INFO_GENERATION_ERROR; // debug info json write failed
    // LCOV_EXCL_STOP
  }
  CovInstrumentationWalker covWalker(&module, config->reportFunction.data(), basicBlockWalker);
  covWalker.covWalk();

  MockInstrumentationWalker mockWalker(&module);
  mockWalker.mockWalk();

  const BinaryenModuleAllocateAndWriteResult result =
      BinaryenModuleAllocateAndWrite(&module, nullptr);
  std::ofstream wasmFileStream(this->config->targetName.data(), std::ios::trunc | std::ios::binary);
  wasmFileStream.write(static_cast<char *>(result.binary),
                       static_cast<std::streamsize>(result.binaryBytes));
  wasmFileStream.close();
  free(result.binary);
  free(result.sourceMap);
  if (wasmFileStream.fail() || wasmFileStream.bad()) {
    // Hard to control IO error
    // LCOV_EXCL_START
    return InstrumentationResponse::FILE_GENERATION_ERROR; // debug info json write failed
    // LCOV_EXCL_STOP
  }

  Json::Value expectInfosJson;
  for (const auto &[key, value] : mockWalker.getExpectInfos()) {
    // Mock test will verified with wasm-testing-framework project, escape this
    // LCOV_EXCL_START
    expectInfosJson[std::to_string(key)] = value;
    // LCOV_EXCL_STOP
  }
  std::ofstream expectInfosJsonWriteStream(config->expectInfoOutputFilePath.data(),
                                           std::ios::trunc);
  Json::StreamWriterBuilder expectInfosJsonBuilder;
  expectInfosJsonBuilder["indentation"] = "";
  std::unique_ptr<Json::StreamWriter> expectInfosJsonWriter(jsonBuilder.newStreamWriter());
  if (expectInfosJsonWriter->write(expectInfosJson, &expectInfosJsonWriteStream) != 0) {
    // Hard to control IO error
    // LCOV_EXCL_START
    return InstrumentationResponse::EXPECT_INFO_GENERATION_ERROR; // expectation info generation
                                                                  // failed
    // LCOV_EXCL_STOP
  }
  expectInfosJsonWriteStream.close();
  if (expectInfosJsonWriteStream.fail() || expectInfosJsonWriteStream.bad()) {
    // Hard to control IO error
    // LCOV_EXCL_START
    return InstrumentationResponse::EXPECT_INFO_GENERATION_ERROR; // expectation info generation
                                                                  // failed
    // LCOV_EXCL_STOP
  }
  return InstrumentationResponse::NORMAL;
}

#if defined(__EMSCRIPTEN__)
extern "C" EMSCRIPTEN_KEEPALIVE wasmInstrumentation::InstrumentationResponse
wasm_instrument(char const *const fileName, char const *const targetName,
                char const *const reportFunction, char const *const sourceMap,
                char const *const expectInfoOutputFilePath,
                char const *const debugInfoOutputFilePath, char const *const includes,
                char const *const excludes, bool skipLib) noexcept {

  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = fileName;
  config.targetName = targetName;
  config.reportFunction = reportFunction;
  config.sourceMap = sourceMap;
  config.expectInfoOutputFilePath = expectInfoOutputFilePath;
  config.debugInfoOutputFilePath = debugInfoOutputFilePath;
  config.includes = includes;
  config.excludes = excludes;
  config.skipLib = skipLib;
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  return instrumentor.instrument();
}
#endif

} // namespace wasmInstrumentation
