#include "MockInstrumentationWalker.hpp"
#include <support/index.h>
#include <wasm.h>
// mock test will be tested with wasm-testing-framework project, escape this class
// LCOV_EXCL_START
using namespace wasmInstrumentation;

void MockInstrumentationWalker::visitCall(wasm::Call *const curr) noexcept {
  const auto functionRefsIterator = funcRefs.find(curr->target.toString());
  if (functionRefsIterator == funcRefs.cend()) {
    if (std::any_of(this->expectTestFuncNames.begin(), this->expectTestFuncNames.end(),
                    [&curr](const std::string_view &str) noexcept {
                      return curr->target.hasSubstring(str);
                    })) {
      const std::unordered_map<wasm::Expression *, wasm::Function::DebugLocation>
          currDebugLocations = getFunction()->debugLocations;
      const auto currentDebugLocationIterator = currDebugLocations.find(curr);
      if (currentDebugLocationIterator != currDebugLocations.cend()) {
        wasm::Function::DebugLocation const &currDebugLocation =
            currentDebugLocationIterator->second;
        const std::string &fileName = module->debugInfoFileNames[currDebugLocation.fileIndex];
        std::stringstream expectInfo;
        expectInfo << fileName << ":" << std::to_string(currDebugLocation.lineNumber) << ":"
                   << std::to_string(currDebugLocation.columnNumber);
        expectInfos[expectIndex] = expectInfo.str();
      }
      curr->operands.back() = moduleBuilder.makeConst(wasm::Literal(expectIndex));
      expectIndex += 1;
    }
    return;
  }
  const wasm::Index localIdx = wasm::Builder::addVar(getFunction(), wasm::Type::i32);
  const auto [tableName, originFuncIdx] = functionRefsIterator->second;
  const std::array<wasm::Expression *, 2U> callArgs = {moduleBuilder.makeConst(originFuncIdx),
                                                       moduleBuilder.makeConst(true)};
  wasm::If *const mockReplacement = moduleBuilder.makeIf(
      moduleBuilder.makeBinary(
          wasm::BinaryOp::NeInt32,
          moduleBuilder.makeLocalTee(localIdx,
                                     moduleBuilder.makeCall(checkMock, callArgs, wasm::Type::i32),
                                     wasm::Type::i32),
          moduleBuilder.makeConst(-1)),
      moduleBuilder.makeCallIndirect(tableName,
                                     moduleBuilder.makeLocalGet(localIdx, wasm::Type::i32),
                                     curr->operands, getModule()->getFunction(curr->target)->type),
      curr);
  replaceCurrent(mockReplacement);
}

void MockInstrumentationWalker::visitCallIndirect(wasm::CallIndirect *const curr) noexcept {
  if (funcRefs.size() == 0U) {
    return;
  }
  const std::array<wasm::Expression *, 2U> args = {curr->target, moduleBuilder.makeConst(false)};
  curr->target = moduleBuilder.makeCall(checkMock, args, wasm::Type::i32);
}

bool MockInstrumentationWalker::mockFunctionDuplicateImportedCheck() const noexcept {
  bool checkRepeat = false;
  wasm::ModuleUtils::iterDefinedFunctions(
      *module, [&checkRepeat, this](const BinaryenFunctionRef &func) noexcept {
        if (func->name.str == this->checkMock) {
          checkRepeat = true;
        }
      });
  wasm::ModuleUtils::iterImportedFunctions(
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
    wasm::ModuleUtils::iterDefinedFunctions(*module, [this](wasm::Function *const func) noexcept {
      walkFunctionInModule(func, this->module);
    });
    return 0U;
  }
}
// LCOV_EXCL_STOP
