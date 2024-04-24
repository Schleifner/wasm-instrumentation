#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include "../src/CoverageInstru.hpp"
#include "../src/InstrumentResponse.hpp"
#include "utils/utils.h"

using namespace std::filesystem;

TEST(e2e, asc) {
  const path project_path = testUtils::getProjectPath();
  const path build_path = project_path / "build";
  const path wasmPath = project_path / "test" / "test-asc" / "build" / "debug.wasm";
  const path mapPath = project_path / "test" / "test-asc" / "build" / "debug.wasm.map";
  const path targetPath = wasmPath.parent_path() / "debug.wasm.instrumented.wasm";
  const path targetDebugInfoPath = wasmPath.parent_path() / "debug.wasm.debuginfo.json";
  const path targetExpectInfoPath = wasmPath.parent_path() / "debug.wasm.expectInfo.json";
  const char *reportName = "assembly/env/traceExpression";
  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = wasmPath.c_str();
  config.debugInfoOutputFilePath = targetDebugInfoPath.c_str();
  config.sourceMap = mapPath.c_str();
  config.targetName = targetPath.c_str();
  config.reportFunction = reportName;
  config.expectInfoOutputFilePath = targetExpectInfoPath.c_str();
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::NORMAL);
}
TEST(e2e, empty_config) {
  wasmInstrumentation::InstrumentationConfig config;
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::CONFIG_ERROR);
}

TEST(e2e, illegal_path_config) {
  const path project_path = testUtils::getProjectPath();
  const path build_path = project_path / "build";
  const path wasmPath = project_path / "test" / "test-asc" / "build" / "debug.wasm";
  const path mapPath = project_path / "test" / "test-asc" / "build" / "debug.wasm.map";
  const path targetPath = wasmPath.parent_path() / "debug.wasm.instrumented.wasm";
  const path targetExpectInfoPath = wasmPath.parent_path() / "debug.wasm.expectInfo.json";
  const char *reportName = "assembly/env/traceExpression";
  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = wasmPath.c_str();
  config.debugInfoOutputFilePath = "this path will not be existed";
  config.sourceMap = mapPath.c_str();
  config.targetName = targetPath.c_str();
  config.reportFunction = reportName;
  config.expectInfoOutputFilePath = targetExpectInfoPath.c_str();
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(),
            wasmInstrumentation::InstrumentationResponse::CONFIG_FILEPATH_ERROR);
}
