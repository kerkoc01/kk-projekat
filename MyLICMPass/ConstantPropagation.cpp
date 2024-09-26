#include "ConstantPropagation.h"

void ConstantPropagation::findAllInstructions(Function &F)
{
    errs() << "Finding instructions\n";
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        ConstantPropagationInstruction *CPI = new ConstantPropagationInstruction(&I, Variables);
        Instructions.push_back(CPI);

        Instruction *Previous = I.getPrevNonDebugInstruction();

        if (Previous == nullptr) {
          for (BasicBlock *Pred : predecessors(&BB)) {
            Instruction *Terminator = Pred->getTerminator();
            CPI->addPredecessor(*std::find_if(Instructions.begin(), Instructions.end(),
            [Terminator](ConstantPropagationInstruction *CPI){ return CPI->getInstruction() == Terminator; }));
          }
        }
        else {
          CPI->addPredecessor(Instructions[Instructions.size() - 2]);
        }
      }
    }
}

void ConstantPropagation::findAllVariables(Function &F)
{
    errs() << "Finding variables\n";
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (isa<AllocaInst>(&I)) {
          Variables.push_back(&I);
        }
      }
    }
}

void ConstantPropagation::setStatusForFirstInstruction()
{
    errs() << "Setting status\n";
    for (Value *Variable : Variables) {
      Instructions.front()->setStatusBefore(Variable, Top);
    }
}

void ConstantPropagation::propagateVariable(Value *Variable)
{

    errs() << "RULES!\n";

    bool RuleApplied;

    while (true) {
      RuleApplied = false;

      for (ConstantPropagationInstruction *CPI : Instructions) {
        if (!checkRuleOne(CPI, Variable)) {
          errs() << "RULE1\n";
          applyRuleOne(CPI, Variable);
          RuleApplied = true;
        } else if (!checkRuleTwo(CPI, Variable)) {
          errs() << "RULE2\n";
          applyRuleTwo(CPI, Variable);
          RuleApplied = true;
        } else if (!checkRuleThree(CPI, Variable)) {
          errs() << "RULE3\n";
          int Value;
          bool reached = false;
          for (ConstantPropagationInstruction *Predecessor :
               CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Const) {
              reached = true;
              Value = Predecessor->getValueAfter(Variable);
              break;
            }
          }
          applyRuleThree(CPI, Variable, Value);
          RuleApplied = true;
        } else if (!checkRuleFour(CPI, Variable)) {
          errs() << "RULE4\n";
          applyRuleFour(CPI, Variable);
          RuleApplied = true;
        } else if (!checkRuleFive(CPI, Variable)) {
          errs() << "RULE5\n";
          applyRuleFive(CPI, Variable);
          RuleApplied = true;
        } else if (!checkRuleSix(CPI, Variable)) {
          errs() << "RULE6\n";
          ConstantInt *ConstInt =
              dyn_cast<ConstantInt>(CPI->getInstruction()->getOperand(0));
          applyRuleSix(CPI, Variable, ConstInt->getSExtValue());
          RuleApplied = true;
        } else if (!checkRuleSeven(CPI, Variable)) {
          errs() << "RULE7\n";
          applyRuleSeven(CPI, Variable);
          RuleApplied = true;
        } else if (!checkRuleEight(CPI, Variable)) {
          errs() << "RULE8\n";
          applyRuleEight(CPI, Variable);
          RuleApplied = true;
        }
      }

      if (!RuleApplied) {
        break;
      }
    }
}

void ConstantPropagation::runAlgorithm()
{
    for (Value *Variable : Variables) {
      propagateVariable(Variable);
    }
}

bool ConstantPropagation::modifyIR()
{
    bool Changed = false;
    std::unordered_map<Value *, Value *> VariablesMap;

    errs() << "PROPAGATION MODIFYING IR\n";

    for (ConstantPropagationInstruction *CPI : Instructions) {
      if (isa<LoadInst> (CPI->getInstruction())) {
        VariablesMap[CPI->getInstruction()] = CPI->getInstruction()->getOperand(0);
      }
    }

    for (ConstantPropagationInstruction *CPI : Instructions) {
      Instruction *Instr = CPI->getInstruction();

      if (isa<StoreInst>(Instr)) {
        Value *Operand = Instr->getOperand(0);
        if (CPI->getStatusBefore(VariablesMap[Operand]) == Const) {
          int Value = CPI->getValueBefore(VariablesMap[Operand]);
          ConstantInt *ConstInt = ConstantInt::get(Type::getInt32Ty(Instr->getContext()), Value);
          Operand->replaceAllUsesWith(ConstInt);
          Changed = true;
        }
      }
      else if (isa<BinaryOperator>(Instr) || isa<ICmpInst>(Instr)) {
        Value *Lhs = Instr->getOperand(0), *Rhs = Instr->getOperand(1);
        Value *VarLHS = VariablesMap[Lhs], *VarRHS = VariablesMap[Rhs];

        if (VarLHS != nullptr && CPI->getStatusBefore(VarLHS) == Const) {
          Lhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarLHS)));
          Changed = true;
        }

        if (VarRHS != nullptr && CPI->getStatusBefore(VarRHS) == Const) {
          Rhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarRHS)));
          Changed = true;
        }
      }
      else if (isa<CallInst>(Instr)){
        Value *Operand, *VarOperand;

        for (size_t i = 0;i < Instr->getNumOperands(); i++) {
          Operand = Instr->getOperand(i);
          VarOperand = VariablesMap[Operand];

          if (VarOperand != nullptr && CPI->getStatusBefore(VarOperand) == Const) {
            Operand->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarOperand)));
            Changed = true;
          }
        }
      }
    }

    return Changed;
}

bool ConstantPropagation::runOnFunction(Function &F) {
    Variables.clear();
    Instructions.clear();
    findAllVariables(F);
    findAllInstructions(F);
    setStatusForFirstInstruction();
    runAlgorithm();
    return modifyIR();
}

char ConstantPropagation::ID = 0;
static RegisterPass<ConstantPropagation> X("our-constant-propagation", "Our simple constant propagation pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);