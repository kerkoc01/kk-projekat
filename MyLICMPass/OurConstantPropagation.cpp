#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include <unordered_set>
#include "ConstantPropagationInstruction.h"

using namespace llvm;

void performConstantPropagation(Function &F) {
  std::vector<Value *> Variables;
  std::vector<ConstantPropagationInstruction *> Instructions;

  // Function to find all instructions and their predecessors
  auto findAllInstructions = [&](Function &F) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        ConstantPropagationInstruction *CPI = new ConstantPropagationInstruction(&I, Variables);
        Instructions.push_back(CPI);

        Instruction *Previous = I.getPrevNonDebugInstruction();

        if (Previous == nullptr) {
          for (BasicBlock *Pred : predecessors(&BB)) {
            Instruction *Terminator = Pred->getTerminator();
            CPI->addPredecessor(*std::find_if(Instructions.begin(), Instructions.end(),
                                              [Terminator](ConstantPropagationInstruction *CPI) { return CPI->getInstruction() == Terminator; }));
          }
        } else {
          CPI->addPredecessor(Instructions[Instructions.size() - 2]);
        }
      }
    }
  };

  // Function to find all variables (alloca instructions)
  auto findAllVariables = [&](Function &F) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (isa<AllocaInst>(&I)) {
          Variables.push_back(&I);
        }
      }
    }
  };

  // Function to set initial status for the first instruction
  auto setStatusForFirstInstruction = [&]() {
    for (Value *Variable : Variables) {
      Instructions.front()->setStatusBefore(Variable, Top);
    }
  };

  // Function to propagate variables based on rules
  auto propagateVariable = [&](Value *Variable) {
    bool RuleApplied;

    while (true) {
      RuleApplied = false;

      for (ConstantPropagationInstruction *CPI : Instructions) {
        if (!checkRuleOne(CPI, Variable)) {
          applyRuleOne(CPI, Variable);
          RuleApplied = true;
          break;
        }

        if (!checkRuleTwo(CPI, Variable)) {
          applyRuleTwo(CPI, Variable);
          RuleApplied = true;
          break;
        }

        if (!checkRuleThree(CPI, Variable)) {
          int Value;

          for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
            if (Predecessor->getStatusAfter(Variable) == Const) {
              Value = Predecessor->getValueAfter(Variable);
              break;
            }
          }

          applyRuleThree(CPI, Variable, Value);
          RuleApplied = true;
          break;
        }

        if (!checkRuleFour(CPI, Variable)) {
          applyRuleFour(CPI, Variable);
          RuleApplied = true;
          break;
        }

        if (!checkRuleFive(CPI, Variable)) {
          applyRuleFive(CPI, Variable);
          RuleApplied = true;
          break;
        }

        if (!checkRuleSix(CPI, Variable)) {
          ConstantInt *ConstInt = dyn_cast<ConstantInt>(CPI->getInstruction()->getOperand(0));
          applyRuleSix(CPI, Variable, ConstInt->getSExtValue());
          RuleApplied = true;
          break;
        }

        if (!checkRuleSeven(CPI, Variable)) {
          applyRuleSeven(CPI, Variable);
          RuleApplied = true;
          break;
        }

        if (!checkRuleEight(CPI, Variable)) {
          applyRuleEight(CPI, Variable);
          RuleApplied = true;
          break;
        }
      }

      if (!RuleApplied) {
        break;
      }
    }
  };

  // Function to run the propagation algorithm
  auto runAlgorithm = [&]() {
    for (Value *Variable : Variables) {
      propagateVariable(Variable);
    }
  };

  // Function to modify the IR based on propagation results
  auto modifyIR = [&]() {
    std::unordered_map<Value *, Value *> VariablesMap;

    for (ConstantPropagationInstruction *CPI : Instructions) {
      if (isa<LoadInst>(CPI->getInstruction())) {
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
        }
      } else if (isa<BinaryOperator>(Instr) || isa<ICmpInst>(Instr)) {
        Value *Lhs = Instr->getOperand(0), *Rhs = Instr->getOperand(1);
        Value *VarLHS = VariablesMap[Lhs], *VarRHS = VariablesMap[Rhs];

        if (VarLHS != nullptr && CPI->getStatusBefore(VarLHS) == Const) {
          Lhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarLHS)));
        }

        if (VarRHS != nullptr && CPI->getStatusBefore(VarRHS) == Const) {
          Rhs->replaceAllUsesWith(ConstantInt::get(Type::getInt32Ty(Instr->getContext()), CPI->getValueBefore(VarRHS)));
        }
      }
    }
  };

  // Execute the steps
  findAllVariables(F);
  findAllInstructions(F);
  setStatusForFirstInstruction();
  runAlgorithm();
  modifyIR();
}

