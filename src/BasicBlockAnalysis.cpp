#include "BasicBlockAnalysis.hpp"
#include <algorithm>
#include <memory>
#include <regex>
#include <string>
#include <utility>
using namespace wasmInstrumentation;

bool BasicBlockAnalysis::shouldIncludeFile(const std::string &&fileName) const noexcept {

  bool shouldInclude = false;
  bool shouldExclude = false;
  if (includes.size() == 0) {
    shouldInclude = true;
  } else {
    for (const auto &include : includes) {
      if (std::regex_match(fileName, include)) {
        shouldInclude = true;
        break;
      }
    }
  }

  for (const auto &exclude : this->excludes) {
    if (std::regex_match(fileName, exclude)) {
      shouldExclude = true;
    }
  }
  return (shouldInclude && (!shouldExclude));
}