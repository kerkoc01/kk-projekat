#ifndef LLVM_PROJECT_CONSTANTFOLDING_H
#define LLVM_PROJECT_CONSTANTFOLDING_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include<vector>

using namespace llvm;

class ConstantFolding : public FunctionPass {
private:
  std::vector<Instruction *> InstructionsToRemove;

  bool handleBinaryOperator(Instruction &I);
  bool handleCompareInstruction(Instruction &I);
  bool handleBranchInstruction(Instruction &I);
  bool iterateInstructions(Function &F);

public:
  static char ID;
  ConstantFolding() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;
};

#endif // LLVM_PROJECT_CONSTANTFOLDING_H