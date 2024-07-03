#include "ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Instruction.h"
#include <vector>

using namespace llvm;

void handleBinaryOperator(Instruction &I, std::vector<Instruction *> &InstructionsToRemove);
void handleCompareInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove);
void handleBranchInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove);

bool performConstantFolding(Function &F) {
    std::vector<Instruction *> InstructionsToRemove;
    bool changed = false;

    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (isa<BinaryOperator>(&I)) {
                handleBinaryOperator(I, InstructionsToRemove);
            } else if (isa<ICmpInst>(&I)) {
                handleCompareInstruction(I, InstructionsToRemove);
            } else if (isa<BranchInst>(&I)) {
                handleBranchInstruction(I, InstructionsToRemove);
            }
        }
    }

    for (Instruction *Instr : InstructionsToRemove) {
        Instr->eraseFromParent();
        changed = true;
    }

    return changed;
}

void handleBinaryOperator(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
    Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
    ConstantInt *LhsValue, *RhsValue;
    int Value;

    if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
        return;
    }

    if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
        return;
    }

    if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
      return;
    }

    if (isa<AddOperator>(&I)) {
      Value = LhsValue->getSExtValue() + RhsValue->getSExtValue();
    }
    else if (isa<SubOperator>(&I)) {
      Value = LhsValue->getSExtValue() - RhsValue->getSExtValue();
    }
    else if (isa<MulOperator>(&I)) {
      Value = LhsValue->getSExtValue() * RhsValue->getSExtValue();
    }
    else if (isa<SDivOperator>(&I)) {
      if (RhsValue->getSExtValue() == 0) {
        errs() << "Division by zero not allowed!\n";
        exit(1);
      }

      Value = LhsValue->getSExtValue() / RhsValue->getSExtValue();
    }


    I.replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(I.getContext()), Value));
    InstructionsToRemove.push_back(&I);
}

void handleCompareInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
    Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
    ConstantInt *LhsValue, *RhsValue;
    bool Value;

    if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
        return;
    }

    if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
        return;
    }

    ICmpInst *Cmp = dyn_cast<ICmpInst>(&I);
    auto Pred = Cmp->getSignedPredicate();

    if (Pred == ICmpInst::ICMP_EQ) {
      Value = LhsValue == RhsValue;
    }
    else if (Pred == ICmpInst::ICMP_NE) {
      Value = LhsValue != RhsValue;
    }
    else if (Pred == ICmpInst::ICMP_SGT) {
      Value = LhsValue > RhsValue;
    }
    else if (Pred == ICmpInst::ICMP_SLT) {
      Value = LhsValue < RhsValue;
    }
    else if (Pred == ICmpInst::ICMP_SGE) {
      Value = LhsValue >= RhsValue;
    }
    else if (Pred == ICmpInst::ICMP_SLE) {
      Value = LhsValue <= RhsValue;
    }

    I.replaceAllUsesWith(ConstantInt::get(Type::getInt1Ty(I.getContext()), Value));
    InstructionsToRemove.push_back(&I);
}

void handleBranchInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
    BranchInst *BranchInstr = dyn_cast<BranchInst>(&I);
    if (BranchInstr->isConditional()) {
        ConstantInt *Condition = dyn_cast<ConstantInt>(BranchInstr->getCondition());

        if (Condition == nullptr) {
            return;
        }

        if (Condition->getZExtValue() == 1) {
            BranchInst::Create(BranchInstr->getSuccessor(0), BranchInstr->getParent());
        } else {
            BranchInst::Create(BranchInstr->getSuccessor(1), BranchInstr->getParent());
        }

        InstructionsToRemove.push_back(&I);
    }
}
