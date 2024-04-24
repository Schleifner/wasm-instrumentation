#include "CovInstrumentationWalker.hpp"

namespace wasmInstrumentation {
void CovInstrumentationWalker::introduceReportFun() noexcept {
  bool needImport = true;
  ModuleUtils::iterDefinedFunctions(*module,
                                    [this, &needImport](const BinaryenFunctionRef &func) noexcept {
                                      if (func->name == IString(this->reportFunName)) {
                                        // escape already declared function
                                        // LCOV_EXCL_START
                                        needImport = false;
                                        // LCOV_EXCL_STOP
                                      }
                                    });
  ModuleUtils::iterImportedFunctions(*module,
                                     [this, &needImport](const BinaryenFunctionRef &func) noexcept {
                                       if (func->name == IString(this->reportFunName)) {
                                         // escape already imported function
                                         // LCOV_EXCL_START
                                         needImport = false;
                                         // LCOV_EXCL_STOP
                                       }
                                     });
  if (needImport) {
    Builder builder(*module);
    std::array<BinaryenType, 3U> iii_{BinaryenTypeInt32(), BinaryenTypeInt32(),
                                      BinaryenTypeInt32()};
    const BinaryenType iii = BinaryenTypeCreate(iii_.data(), iii_.size());
    BinaryenAddFunctionImport(module, reportFunName, "covInstrument", "traceExpression", iii,
                              Type::none);
  }
}

void CovInstrumentationWalker::visitFunction(Function *const curr) noexcept {
  using Parent = PostWalker<CovInstrumentationWalker,
                            UnifiedExpressionVisitor<CovInstrumentationWalker, void>>;
  Parent::visitFunction(curr);

  const Index functionIndex = basicBlockWalker.getFunctionIndexByName(curr->name.str);
  // function in instruction
  if (functionIndex != static_cast<Index>(-1)) {
    const std::array<BinaryenExpressionRef, 3U> callInReportArgs = {
        moduleBuilder.makeConst(functionIndex), moduleBuilder.makeConst(static_cast<Index>(-1)),
        moduleBuilder.makeConst(1U)};
    Call *const callIn = moduleBuilder.makeCall(this->reportFunName, callInReportArgs, Type::none);

    Block *const newBody = moduleBuilder.makeBlock();
    newBody->list.push_back(callIn);
    newBody->list.push_back(curr->body);
    newBody->finalize(curr->body->type);
    curr->body = newBody;
  }
}

void CovInstrumentationWalker::visitExpression(Expression *curr) noexcept {
  BinaryenFunctionRef func = getFunction();
  const std::vector<InstrumentPosition> *positionIterator =
      basicBlockWalker.getCovInstrumentPosition(curr);
  if (positionIterator != nullptr) {
    for (const InstrumentPosition &position : *positionIterator) {
      Block *replacement = moduleBuilder.makeBlock();
      const Index functionIndex = basicBlockWalker.getFunctionIndexByName(func->name.str);
      const std::array<BinaryenExpressionRef, 3U> reportArgs = {
          moduleBuilder.makeConst(functionIndex), moduleBuilder.makeConst(position.basicBlockIndex),
          moduleBuilder.makeConst(0U)};
      Expression *report = moduleBuilder.makeCall(reportFunName, reportArgs, Type::none);
      Expression **replacePtr = getCurrentPointer();
      if (position.pre) {
        replacement->list.push_back(report);
        replacement->list.push_back(*replacePtr);
      } else {
        replacement->list.push_back(*replacePtr);
        replacement->list.push_back(report);
      }
      replacement->finalize(curr->type);
      *replacePtr = replacement;
    }
  }

  if (curr->is<Call>()) {
    // function out instrumentation
    Call *const call = curr->cast<Call>();
    const Index targetFunctionIndex = basicBlockWalker.getFunctionIndexByName(call->target.str);
    if (targetFunctionIndex != static_cast<Index>(-1)) {
      Block *replacement = moduleBuilder.makeBlock();
      const std::array<BinaryenExpressionRef, 3U> callOutReportArgs = {
          moduleBuilder.makeConst(targetFunctionIndex),
          moduleBuilder.makeConst(static_cast<Index>(-1)), moduleBuilder.makeConst(2U)};
      Expression *callOut = moduleBuilder.makeCall(reportFunName, callOutReportArgs, Type::none);
      Expression **replacePtr = getCurrentPointer();
      replacement->list.push_back(*replacePtr);
      replacement->list.push_back(callOut);
      replacement->finalize(curr->type);
      *replacePtr = replacement;
    }
  }
}

void CovInstrumentationWalker::covWalk() noexcept {
  introduceReportFun();
  ModuleUtils::iterDefinedFunctions(*module, [this](const BinaryenFunctionRef &func) noexcept {
    if (basicBlockWalker.getBasicBlockAnalysis().shouldIncludeFile(func->name.toString())) {
      walkFunctionInModule(func, module);
    }
  });
}

} // namespace wasmInstrumentation
