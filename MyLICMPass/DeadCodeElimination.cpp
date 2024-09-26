#include "DeadCodeElimination.h"

void DeadCodeElimination::handleOperand(Value *Operand)
{
    if (Variables.find(Operand) != Variables.end()) {
          Variables[Operand] = true;
    }
    if (VariablesMap.find(Operand) != VariablesMap.end()) {
          Variables[VariablesMap[Operand]] = true;
    }
}

bool DeadCodeElimination::eliminateDeadInstructions(Function &F)
{
    InstructionsToRemove.clear();

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (!I.getType()->isVoidTy() && !isa<CallInst>(&I)) {
          Variables[&I] = false;
        }

        if (isa<LoadInst>(&I)) {
          VariablesMap[&I] = I.getOperand(0);
        }

        if (isa<StoreInst>(&I)) {
          handleOperand(I.getOperand(0));
        }
        else {
          for (size_t i = 0; i < I.getNumOperands(); i++) {
            handleOperand(I.getOperand(i));
          }
        }
      }
    }

    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (isa<StoreInst>(&I)) {
          if (!Variables[I.getOperand(1)]) {
            InstructionsToRemove.push_back(&I);
          }
        }
        else {
          if (Variables.find(&I) != Variables.end() && !Variables[&I]) {
            InstructionsToRemove.push_back(&I);
          }
        }
      }
    }

    if (InstructionsToRemove.size() > 0) {
      InstructionRemoved = true;
    }

    for (Instruction *Instr : InstructionsToRemove) {
      Instr->eraseFromParent();
    }

    return InstructionRemoved;
}

bool DeadCodeElimination::eliminateUnreachableInstructions(Function &F)
{
    std::vector<BasicBlock *> UnreachableBlocks;
    OurCFG *CFG = new OurCFG(F);
    CFG->DFS(&F.front());

    for (BasicBlock &BB : F) {
      if (!CFG->isReachable(&BB)) {
        UnreachableBlocks.push_back(&BB);
      }
    }

    if (UnreachableBlocks.size() > 0) {
      InstructionRemoved = true;
    }

    for (BasicBlock *UnreachableBlock : UnreachableBlocks) {
      UnreachableBlock->eraseFromParent();
    }

    return InstructionRemoved;
}

bool DeadCodeElimination::runOnFunction(Function &F) {
    bool Changed = false;
    do {
      InstructionRemoved = false;
      Changed |= eliminateDeadInstructions(F);
      Changed |= eliminateUnreachableInstructions(F);
    } while (InstructionRemoved);

    return Changed;
}

char DeadCodeElimination::ID = 0;
static RegisterPass<DeadCodeElimination> X("dead-code-elimination", "Our simple constant folding",
                                              false /* Only looks at CFG */,
                                              false /* Analysis Pass */);