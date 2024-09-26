#ifndef LLVM_PROJECT_CONSTANTPROPAGATION_H
#define LLVM_PROJECT_CONSTANTPROPAGATION_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/LegacyPassManager.h"

#include <vector>
#include <unordered_set>

#include "ConstantPropagationInstruction.h"

using namespace llvm;

class ConstantPropagation : public FunctionPass {
private:
  std::vector<Value *> Variables;
  std::vector<ConstantPropagationInstruction *> Instructions;

  void findAllInstructions(Function &F);
  void findAllVariables(Function &F);
  void setStatusForFirstInstruction();
  bool checkRuleOne(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
      if (Predecessor->getStatusAfter(Variable) == Top) {
        return CPI->getStatusBefore(Variable) == Top;
      }
    }

    return true;
  }

  void applyRuleOne(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusBefore(Variable, Top);
  }

  bool checkRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    std::unordered_set<int> Values;

    for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
      if (Predecessor->getStatusAfter(Variable) == Const) {
        Values.insert(Predecessor->getValueAfter(Variable));
      }
    }

    if (Values.size() > 1) {
      return CPI->getStatusBefore(Variable) == Top;
    }

    return true;
  }

  void applyRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusBefore(Variable, Top);
  }

  bool checkRuleThree(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    std::unordered_set<int> Values;

    for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
      if (Predecessor->getStatusAfter(Variable) == Const) {
        Values.insert(Predecessor->getValueAfter(Variable));
      }
      if (Predecessor->getStatusAfter(Variable) == Top) {
        return true;
      }
    }

    if (Values.size() == 1) {
      return CPI->getStatusBefore(Variable) == Const &&
             CPI->getValueBefore(Variable) == *Values.begin();
    }

    return true;
  }

  void applyRuleThree(ConstantPropagationInstruction *CPI, Value *Variable, int Value)
  {
    CPI->setStatusBefore(Variable, Const, Value);
  }

  bool checkRuleFour(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
      if (Predecessor->getStatusAfter(Variable) == Top || Predecessor->getStatusAfter(Variable) == Const) {
        return true;
      }
    }

    // Nalazimo se u prvoj instrukciji u funkciji
    if (CPI->getPredecessors().size() == 0) {
      return true;
    }

    return CPI->getStatusBefore(Variable) == Bottom;
  }

  void applyRuleFour(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusBefore(Variable, Bottom);
  }

  bool checkRuleFive(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    if (CPI->getStatusBefore(Variable) == Bottom) {
      return CPI->getStatusAfter(Variable) == Bottom;
    }

    return true;
  }

  void applyRuleFive(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusAfter(Variable, Bottom);
  }

  bool checkRuleSix(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    Instruction *Instr = CPI->getInstruction();
    if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable) {
      if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(0))) {
        return CPI->getStatusAfter(Variable) == Const && CPI->getValueAfter(Variable) == ConstInt->getSExtValue();
      }
    }

    return true;
  }

  void applyRuleSix(ConstantPropagationInstruction *CPI, Value *Variable, int Value)
  {
    CPI->setStatusAfter(Variable, Const, Value);
  }

  bool checkRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    Instruction *Instr = CPI->getInstruction();
    if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable && !isa<ConstantInt>(Instr->getOperand(0))) {
      return CPI->getStatusAfter(Variable) == Top;
    }

    return true;
  }

  void applyRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusAfter(Variable, Top);
  }

  bool checkRuleEight(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    if (isa<StoreInst>(CPI->getInstruction()) && CPI->getInstruction()->getOperand(1) == Variable) {
      return true;
    }

    return CPI->getStatusBefore(Variable) == CPI->getStatusAfter(Variable);
  }

  void applyRuleEight(ConstantPropagationInstruction *CPI, Value *Variable)
  {
    CPI->setStatusAfter(Variable, CPI->getStatusBefore(Variable), CPI->getValueBefore(Variable));
  }
  
  void propagateVariable(Value *Variable);
  void runAlgorithm();
  bool modifyIR();

public:
  static char ID;
  ConstantPropagation() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;
};

#endif // LLVM_PROJECT_CONSTANTPROPAGATION_H