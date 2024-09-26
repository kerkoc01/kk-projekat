#ifndef LLVM_PROJECT_DEADCODEELIMINATION_H
#define LLVM_PROJECT_DEADCODEELIMINATION_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include<vector>
#include<unordered_map>

#include "OurCFG.h"

using namespace llvm;

class DeadCodeElimination : public FunctionPass {
private:
    std::unordered_map<Value *, bool> Variables;
    std::unordered_map<Value *, Value *> VariablesMap;
    std::vector<Instruction *> InstructionsToRemove;
    bool InstructionRemoved;

    void handleOperand(Value *Operand);
    bool eliminateDeadInstructions(Function &F);
    bool eliminateUnreachableInstructions(Function &F);

public:
  static char ID;
  DeadCodeElimination() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;
};

#endif // LLVM_PROJECT_DEADCODEELIMINATION_H