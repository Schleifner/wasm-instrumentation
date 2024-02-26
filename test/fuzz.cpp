#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string_view>
#include "../src/CoverageInstru.hpp"
#include "../src/InstrumentResponse.hpp"
#include "TestUtil.hpp"

TEST(asc_cov_instru_test, fuzz_asc) {
  const std::filesystem::path project_path = WasmInstrumentationTestUtil::getProjectPath();
  const std::filesystem::path build_path = project_path / "build";
  const std::filesystem::path path = project_path / "test" / "fuzz" / "assemblyscript.debug.wasm";
  const std::filesystem::path mapPath =
      project_path / "test" / "fuzz" / "assemblyscript.debug.wasm.map";
  const std::filesystem::path targetPath =
      build_path / "assemblyscript.debug.wasm.instrumented.wasm";
  const std::filesystem::path targetDebugInfoPath =
      build_path / "assemblyscript.debug.wasm.debuginfo.json";
  const std::filesystem::path targetExpectInfoPath =
      build_path / "assemblyscript.debug.wasm.expectinfo.json";
  const char reportName[] = {"assembly/env/traceExpression"};
  wasmInstrumentation::InstrumentationConfig config;
  config.fileName = path.c_str();
  config.debugInfoOutputFilePath = targetDebugInfoPath.c_str();
  config.sourceMap = mapPath.c_str();
  config.targetName = targetPath.c_str();
  config.expectInfoOutputFilePath = targetExpectInfoPath.c_str();
  config.reportFunction = const_cast<char *>(reportName);
  wasmInstrumentation::CoverageInstru instrumentor(&config);
  ASSERT_EQ(instrumentor.instrument(), wasmInstrumentation::InstrumentationResponse::NORMAL);
}
