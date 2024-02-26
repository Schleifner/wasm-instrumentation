#ifndef __ASC_COV_TEST_UTIL_HPP__
#define __ASC_COV_TEST_UTIL_HPP__
#include "json/value.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <unordered_set>
#include <utility>
static constexpr std::string_view PROJECT_NAME("wasm-instrumentation");
///
/// @brief wasm instrumentation test utility class
///
class WasmInstrumentationTestUtil final {
public:
  ///
  /// @brief Get the project path
  ///
  /// @return path of project
  static const std::filesystem::path getProjectPath() noexcept {
    const std::filesystem::path currentFile(std::filesystem::current_path().u8string());
    const std::string currentPath(currentFile.c_str());
    const std::filesystem::path project_path(
        currentPath.substr(0U, currentPath.rfind(PROJECT_NAME) + PROJECT_NAME.size()));
    return project_path;
  }

  ///
  /// @brief Compare two debug info json object
  ///
  /// @param debugInfoJson1
  /// @param debugInfoJson2
  /// @return Return true if the are same debug info json object
  static bool compareDebugInfoJson(Json::Value &debugInfoJson1,
                                   Json::Value &debugInfoJson2) noexcept {

    if ((!debugInfoJson1.isObject()) && (!debugInfoJson2.isObject())) {
      std::cerr << "Not same with types\n";
      return false;
    }

    // compare debug files
    const auto &files1 = debugInfoJson1["debugFiles"];
    const auto &files2 = debugInfoJson2["debugFiles"];
    if (!(files1 == files2)) {
      std::cerr << "Not same with files\n";
      return false;
    }

    // compare function debug info
    const Json::Value &debugInfos1 = debugInfoJson1["debugInfos"];
    const Json::Value &debugInfos2 = debugInfoJson2["debugInfos"];
    const Json::Value::Members &functionNames = debugInfos1.getMemberNames();
    const Json::Value::Members &compareFunctionNames = debugInfos2.getMemberNames();
    if (functionNames != compareFunctionNames) {
      std::cerr << "Not same with function names\n";
      return false;
    }
    for (const std::string_view functionName : functionNames) {
      const Json::Value &functionDebugInfo = debugInfos1[functionName.data()];
      const Json::Value &compareFunctionDebugInfo = debugInfos2[functionName.data()];
      if (functionDebugInfo["index"] != compareFunctionDebugInfo["index"]) {
        std::cerr << "Not same with function index\n";
        return false;
      }

      if (functionDebugInfo["lineInfo"] != compareFunctionDebugInfo["lineInfo"]) {
        std::cerr << "Not same with function line info\n";
        return false;
      }

      // for branch info, compare without order
      std::unordered_map<uint32_t, std::unordered_set<uint32_t>> branchInfos;
      if (functionDebugInfo["branchInfo"].size() != compareFunctionDebugInfo["branchInfo"].size()) {
        std::cerr << "Not same with function branch info size\n";
        return false;
      }
      for (const Json::Value &branchPair : functionDebugInfo["branchInfo"]) {
        branchInfos[branchPair[0U].asUInt()].insert(branchPair[1U].asUInt());
      }
      for (const Json::Value &branchPair : compareFunctionDebugInfo["branchInfo"]) {
        const auto &findIterator = branchInfos.find(branchPair[0U].asUInt());
        if (findIterator == branchInfos.cend()) {
          std::cerr << "Not same with function branch info\n";
          return false;
        }
        if (findIterator->second.find(branchPair[1U].asUInt()) == findIterator->second.cend()) {
          std::cerr << "Not same with function branch info\n";
          return false;
        }
      }
    }
    return true;
  }
};

#endif