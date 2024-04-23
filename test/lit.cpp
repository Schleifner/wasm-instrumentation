#include "json/reader.h"
#include "json/value.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "../src/CoverageInstru.hpp"
#include "../src/InstrumentResponse.hpp"
#include "./TestUtil.hpp"
namespace fs = std::filesystem;
TEST(asc_cov_instru_test, lit) {
  // step 1, prepare
  const std::filesystem::path projectPath = WasmInstrumentationTestUtil::getProjectPath();
  const std::filesystem::path wasmOptPath =
      projectPath / "build" / "thirdparty" / "binaryen" / "bin" / "wasm-opt";
  const std::filesystem::path watFolder = projectPath / "test" / "lit" / "wat";
  const std::filesystem::path buildDir = projectPath / "test" / "lit" / "build";
  const std::filesystem::path runJs = projectPath / "test" / "lit" / "run.cjs";
  const std::filesystem::path checkPy = projectPath / "test" / "check.py";
  const std::filesystem::path fixtureFolder = projectPath / "test" / "lit" / "fixture";

  if (!fs::exists(buildDir)) {
    fs::create_directory(buildDir);
  }

  // step 2, build

  const std::string separator(" ");
  std::vector<std::string> wasmStrs;
  for (const auto &entity : fs::recursive_directory_iterator(watFolder)) {
    if (!fs::is_directory(entity)) {
      std::stringstream cmd;
      cmd << wasmOptPath;
      wasmStrs.push_back(entity.path().filename());
      cmd << separator << entity << separator << "-o" << separator << buildDir << "/"
          << entity.path().filename() << ".out.wasm" << separator << "-osm" << separator << buildDir
          << "/" << entity.path().filename() << ".out.wasm.map" << separator << "-g" << separator
          << "-q";
      ASSERT_EQ(system(cmd.str().c_str()), 0);
    }
  }
  const char *include = "[\"main\",\"assembly/.*\"]";
  const char *exclude = "[\"shouldExclude.*\"]";
  Json::Reader jsonReader;
  // step 3, instrument , run and check;
  for (Json::String const &wasm : wasmStrs) {
    const std::filesystem::path wasmFile = buildDir / (wasm + ".out.wasm");
    const std::filesystem::path wasmFileMap = buildDir / (wasm + ".out.wasm.map");
    const std::filesystem::path wasmTarget = buildDir / (wasm + ".instrumented.wasm");
    const std::filesystem::path debugTarget = buildDir / (wasm + ".debug.json");
    const std::filesystem::path expectTarget = buildDir / (wasm + ".expect.json");
    const char *traceFunName = "assembly/env/traceExpression";
    wasmInstrumentation::InstrumentationConfig config;
    std::cout << "running lit for:" << wasmFile << std::endl;
    config.fileName = wasmFile.c_str();
    config.debugInfoOutputFilePath = debugTarget.c_str();
    config.expectInfoOutputFilePath = expectTarget.c_str();
    config.sourceMap = wasmFileMap.c_str();
    config.targetName = wasmTarget.c_str();
    config.reportFunction = traceFunName;
    config.includes = include;
    config.excludes = exclude;
    wasmInstrumentation::CoverageInstru instrumentor(&config);
    ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::NORMAL);
    std::stringstream cmd;
    cmd << "node " << runJs << " " << wasmTarget << " >" << buildDir << "/" << wasm << ".run.log";
    std::filesystem::path const fixtureFilePath = fixtureFolder / (wasm + ".debug.json");
    std::ifstream fixtureStream(fixtureFilePath);
    std::ifstream debugInfoStream(debugTarget);
    Json::Value fixtureJson;
    Json::Value debugInfoJson;
    jsonReader.parse(fixtureStream, fixtureJson, false);
    jsonReader.parse(debugInfoStream, debugInfoJson, false);

    ASSERT_TRUE(WasmInstrumentationTestUtil::compareDebugInfoJson(fixtureJson, debugInfoJson));
    std::stringstream runLogCmpCmd;
    runLogCmpCmd << "python3 " << checkPy << separator << fixtureFolder << "/" << wasm << ".run.log"
                 << separator << buildDir << "/" << wasm << ".run.log";
    ASSERT_EQ(system(cmd.str().c_str()), 0);
    ASSERT_EQ(system(runLogCmpCmd.str().c_str()), 0);
  }
}
