#include "MockInstrumentationWalker.hpp"
#include <regex>
#include <support/index.h>
#include <wasm.h>
// mock test will be tested with wasm-testing-framework project, escape this class
// LCOV_EXCL_START
namespace wasmInstrumentation {

void MockInstrumentationWalker::visitCall(Call *const curr) noexcept {
  /* generate expect infos */
  if (std::any_of(this->expectTestFuncNames.begin(), this->expectTestFuncNames.end(),
                  [&curr](const std::string_view &str) noexcept {
                    return curr->target.hasSubstring(str);
                  })) {
    const std::unordered_map<Expression *, Function::DebugLocation> currDebugLocations =
        getFunction()->debugLocations;

    const auto currentDebugLocationIterator = currDebugLocations.find(curr);
    if (currentDebugLocationIterator != currDebugLocations.cend()) {
      Function::DebugLocation const &currDebugLocation = currentDebugLocationIterator->second;
      const std::string &fileName = module->debugInfoFileNames[currDebugLocation.fileIndex];
      std::stringstream expectInfo;
      expectInfo << fileName << ":" << std::to_string(currDebugLocation.lineNumber) << ":"
                 << std::to_string(currDebugLocation.columnNumber);
      expectInfos[expectIndex] = expectInfo.str();
    }
    curr->operands.back() = moduleBuilder.makeConst(Literal(expectIndex));
    expectIndex += 1;
  }

  /* Function Call Mock */
  const auto functionRefsIterator = funcRefs.find(curr->target.toString());
  if (functionRefsIterator != funcRefs.end()) {
    const Index localIdx = Builder::addVar(getFunction(), Type::i32);
    const auto [tableName, originFuncIdx] = functionRefsIterator->second;
    const std::array<Expression *, 2U> callArgs = {moduleBuilder.makeConst(originFuncIdx),
                                                   moduleBuilder.makeConst(true)};
    If *const mockReplacement = moduleBuilder.makeIf(
        moduleBuilder.makeBinary(
            BinaryOp::NeInt32,
            moduleBuilder.makeLocalTee(
                localIdx, moduleBuilder.makeCall(checkMock, callArgs, Type::i32), Type::i32),
            moduleBuilder.makeConst(-1)),
        moduleBuilder.makeCallIndirect(tableName, moduleBuilder.makeLocalGet(localIdx, Type::i32),
                                       curr->operands,
                                       getModule()->getFunction(curr->target)->type),
        curr);
    replaceCurrent(mockReplacement);
  }
}

void MockInstrumentationWalker::visitCallIndirect(CallIndirect *const curr) noexcept {
  if (funcRefs.size() == 0U) {
    return;
  }
  const std::array<Expression *, 2U> args = {curr->target, moduleBuilder.makeConst(false)};
  curr->target = moduleBuilder.makeCall(checkMock, args, Type::i32);
}

bool MockInstrumentationWalker::mockFunctionDuplicateImportedCheck() const noexcept {
  bool checkRepeat = false;
  ModuleUtils::iterDefinedFunctions(*module,
                                    [&checkRepeat, this](const BinaryenFunctionRef &func) noexcept {
                                      if (func->name.str == this->checkMock) {
                                        checkRepeat = true;
                                      }
                                    });
  ModuleUtils::iterImportedFunctions(
      *module, [&checkRepeat, this](const BinaryenFunctionRef &func) noexcept {
        if (func->name.str == this->checkMock) {
          checkRepeat = true;
        }
      });
  if (!checkRepeat) {
    std::array<BinaryenType, 2U> ii_ =
        std::array<BinaryenType, 2U>{BinaryenTypeInt32(), BinaryenTypeInt32()};
    const BinaryenType ii = BinaryenTypeCreate(ii_.data(), ii_.size());
    BinaryenAddFunctionImport(module, this->checkMock.data(), "mockInstrument", "checkMock", ii,
                              BinaryenTypeInt32());
  }
  return checkRepeat;
}

uint32_t MockInstrumentationWalker::mockWalk() noexcept {
  if (mockFunctionDuplicateImportedCheck()) {
    return 1U; // failed
  } else {
    ModuleUtils::iterDefinedFunctions(*module, [this](Function *const func) noexcept {
      if (!std::regex_match(func->name.toString(), std::regex("~lib/.+"))) {
        walkFunctionInModule(func, module);
      }
    });
    return 0U;
  }
}
// LCOV_EXCL_STOP
} // namespace wasmInstrumentation
