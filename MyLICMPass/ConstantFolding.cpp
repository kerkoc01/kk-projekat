#include "ConstantFolding.h"

bool ConstantFolding::handleBinaryOperator(Instruction &I)
{
    Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
    ConstantInt *LhsValue, *RhsValue;
    int Value;

    if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
      return false;
    }

    if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
      return false;
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
    else {
        return false;
    }

    I.replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(I.getContext()), Value));
    return true;
}

bool ConstantFolding::handleCompareInstruction(Instruction &I)
{
    Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
    ConstantInt *LhsValue, *RhsValue;
    bool Value;

    if (!(LhsValue = dyn_cast<ConstantInt>(Lhs))) {
      return false;
    }

    if (!(RhsValue = dyn_cast<ConstantInt>(Rhs))) {
      return false;
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
    return true;
}

bool ConstantFolding::handleBranchInstruction(Instruction &I)
{
    BranchInst *BranchInstr = dyn_cast<BranchInst>(&I);
    if (BranchInstr->isConditional()) {
      ConstantInt *Condition =
          dyn_cast<ConstantInt>(BranchInstr->getCondition());

      if (Condition == nullptr) {
        return false;
      }

      if (Condition->getZExtValue() == 1) {
        BranchInst::Create(BranchInstr->getSuccessor(0),
                           BranchInstr->getParent());
      } else {
        BranchInst::Create(BranchInstr->getSuccessor(1),
                           BranchInstr->getParent());
      }

      InstructionsToRemove.push_back(&I);
      return true;
    }

    return false;
}

bool ConstantFolding::iterateInstructions(Function &F)
{
    bool Changed = false;
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (isa<BinaryOperator>(&I)) {
          Changed |= handleBinaryOperator(I);
        }
        else if (isa<ICmpInst>(&I)) {
          Changed |= handleCompareInstruction(I);
        }
        else if (isa<BranchInst>(&I)) {
          Changed |= handleBranchInstruction(I);
        }
      }
    }

    for (Instruction *Instr : InstructionsToRemove) {
      Instr->eraseFromParent();
    }

    return Changed;
}

bool ConstantFolding::runOnFunction(Function &F) {
    return iterateInstructions(F);
}

char ConstantFolding::ID = 0;
static RegisterPass<ConstantFolding> X("constant-folding", "Our simple constant folding",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

