#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include "../src/CoverageInstru.hpp"
#include "../src/InstrumentResponse.hpp"
#include "TestUtil.hpp"
TEST(asc_cov_instru_test, e2e) {
  const std::filesystem::path project_path = WasmInstrumentationTestUtil::getProjectPath();
  const std::filesystem::path build_path = project_path / "build";
  const std::filesystem::path path = project_path / "test" / "test-asc" / "build" / "debug.wasm";
  const std::filesystem::path mapPath =
      project_path / "test" / "test-asc" / "build" / "debug.wasm.map";
  const std::filesystem::path targetPath = path.parent_path() / "debug.wasm.instrumented.wasm";
  const std::filesystem::path targetDebugInfoPath =
      path.parent_path() / "debug.wasm.debuginfo.json";
  const std::filesystem::path targetExpectInfoPath =
      path.parent_path() / "debug.wasm.expectInfo.json";
  const char *reportName = "assembly/env/traceExpression";
  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = path.c_str();
  config.debugInfoOutputFilePath = targetDebugInfoPath.c_str();
  config.sourceMap = mapPath.c_str();
  config.targetName = targetPath.c_str();
  config.reportFunction = reportName;
  config.expectInfoOutputFilePath = targetExpectInfoPath.c_str();
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::NORMAL);
}
TEST(asc_cov_instru_test, empty_config) {
  wasmInstrumentation::InstrumentationConfig config;
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::CONFIG_ERROR);
}

TEST(asc_cov_instru_test, illegal_path_config) {
  const std::filesystem::path project_path = WasmInstrumentationTestUtil::getProjectPath();
  const std::filesystem::path build_path = project_path / "build";
  const std::filesystem::path path = project_path / "test" / "test-asc" / "build" / "debug.wasm";
  const std::filesystem::path mapPath =
      project_path / "test" / "test-asc" / "build" / "debug.wasm.map";
  const std::filesystem::path targetPath = path.parent_path() / "debug.wasm.instrumented.wasm";
  const std::filesystem::path targetExpectInfoPath =
      path.parent_path() / "debug.wasm.expectInfo.json";
  const char *reportName = "assembly/env/traceExpression";
  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = path.c_str();
  config.debugInfoOutputFilePath = "this path will not be existed";
  config.sourceMap = mapPath.c_str();
  config.targetName = targetPath.c_str();
  config.reportFunction = reportName;
  config.expectInfoOutputFilePath = targetExpectInfoPath.c_str();
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(),
            wasmInstrumentation::InstrumentationResponse::CONFIG_FILEPATH_ERROR);
}
