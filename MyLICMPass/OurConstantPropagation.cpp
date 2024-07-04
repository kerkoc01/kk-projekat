#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Operator.h"
#include "ConstantPropagationInstruction.h"
#include <unordered_set>
#include "llvm/IR/LegacyPassManager.h"

using namespace llvm;

// Forward declarations of helper functions
void findAllInstructions(Function &F, std::vector<Value *> &Variables, std::vector<ConstantPropagationInstruction *> &Instructions);
void findAllVariables(Function &F, std::vector<Value *> &Variables);
void setStatusForFirstInstruction(std::vector<ConstantPropagationInstruction *> &Instructions, std::vector<Value *> &Variables);
bool checkRuleOne(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleOne(ConstantPropagationInstruction *CPI, Value *Variable);
bool checkRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable);
bool checkRuleThree(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleThree(ConstantPropagationInstruction *CPI, Value *Variable, int Value);
bool checkRuleFour(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleFour(ConstantPropagationInstruction *CPI, Value *Variable);
bool checkRuleFive(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleFive(ConstantPropagationInstruction *CPI, Value *Variable);
bool checkRuleSix(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleSix(ConstantPropagationInstruction *CPI, Value *Variable, int Value);
bool checkRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable);
bool checkRuleEight(ConstantPropagationInstruction *CPI, Value *Variable);
void applyRuleEight(ConstantPropagationInstruction *CPI, Value *Variable);
void propagateVariable(Value *Variable, std::vector<ConstantPropagationInstruction *> &Instructions);
void runAlgorithm(std::vector<Value *> &Variables, std::vector<ConstantPropagationInstruction *> &Instructions);
void modifyIR(std::vector<ConstantPropagationInstruction *> &Instructions);

void performConstantPropagation(Function &F) {
  std::vector<Value *> Variables;
  std::vector<ConstantPropagationInstruction *> Instructions;

  // Step 1: Find all variables in the function
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isa<AllocaInst>(&I)) {
        Variables.push_back(&I);
      }
    }
  }

  // Step 2: Find all instructions and create CPI objects
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

  // Step 3: Set initial statuses for the first instructions
  for (Value *Variable : Variables) {
    Instructions.front()->setStatusBefore(Variable, Top);
  }

  // Step 4: Constant propagation algorithm
  bool RuleApplied;
  do {
    RuleApplied = false;
    for (ConstantPropagationInstruction *CPI : Instructions) {
      for (Value *Variable : Variables) {
        if (!CPI->isProcessed(Variable)) {
          bool Rule1 = checkRuleOne(CPI, Variable);
          bool Rule2 = checkRuleTwo(CPI, Variable);
          bool Rule3 = checkRuleThree(CPI, Variable);
          bool Rule4 = checkRuleFour(CPI, Variable);
          bool Rule5 = checkRuleFive(CPI, Variable);
          bool Rule6 = checkRuleSix(CPI, Variable);
          bool Rule7 = checkRuleSeven(CPI, Variable);
          bool Rule8 = checkRuleEight(CPI, Variable);

          if (!Rule1) {
            applyRuleOne(CPI, Variable);
            RuleApplied = true;
            break;
          }
          else if (!Rule2) {
            applyRuleTwo(CPI, Variable);
            RuleApplied = true;
            break;
          }
          else if (!Rule3) {
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
          else if (!Rule4) {
            applyRuleFour(CPI, Variable);
            RuleApplied = true;
            break;
          }
          else if (!Rule5) {
            applyRuleFive(CPI, Variable);
            RuleApplied = true;
            break;
          }
          else if (!Rule6) {
            ConstantInt *ConstInt = dyn_cast<ConstantInt>(CPI->getInstruction()->getOperand(0));
            applyRuleSix(CPI, Variable, ConstInt->getSExtValue());
            RuleApplied = true;
            break;
          }
          else if (!Rule7) {
            applyRuleSeven(CPI, Variable);
            RuleApplied = true;
            break;
          }
          else if (!Rule8) {
            applyRuleEight(CPI, Variable);
            RuleApplied = true;
            break;
          }
          CPI->markProcessed(Variable);
        }
      }
    }
  } while (RuleApplied);

  // Step 5: Modify the LLVM IR based on the propagated constants
  for (ConstantPropagationInstruction *CPI : Instructions) {
    Instruction *I = CPI->getInstruction();
    for (Use &U : I->operands()) {
      Value *V = U.get();
      if (isa<Instruction>(V) && CPI->getStatusAfter(V) == Const) {
        U.set(ConstantInt::get(I->getContext(), APInt(32, CPI->getValueAfter(V))));
      }
    }
  }
}

bool checkRuleOne(ConstantPropagationInstruction *CPI, Value *Variable) {
  for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
    if (Predecessor->getStatusAfter(Variable) == Top) {
      return CPI->getStatusBefore(Variable) == Top;
    }
  }
  return true;
}

void applyRuleOne(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusBefore(Variable, Top);
}

bool checkRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable) {
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

void applyRuleTwo(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusBefore(Variable, Top);
}

bool checkRuleThree(ConstantPropagationInstruction *CPI, Value *Variable) {
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
    return CPI->getStatusBefore(Variable) == Const && CPI->getValueBefore(Variable) == *Values.begin();
  }
  return true;
}

void applyRuleThree(ConstantPropagationInstruction *CPI, Value *Variable, int Value) {
  CPI->setStatusBefore(Variable, Const, Value);
}

bool checkRuleFour(ConstantPropagationInstruction *CPI, Value *Variable) {
  for (ConstantPropagationInstruction *Predecessor : CPI->getPredecessors()) {
    if (Predecessor->getStatusAfter(Variable) == Top || Predecessor->getStatusAfter(Variable) == Const) {
      return true;
    }
  }
  if (CPI->getPredecessors().size() == 0) {
    return true;
  }
  return CPI->getStatusBefore(Variable) == Bottom;
}

void applyRuleFour(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusBefore(Variable, Bottom);
}

bool checkRuleFive(ConstantPropagationInstruction *CPI, Value *Variable) {
  if (CPI->getStatusBefore(Variable) == Bottom) {
    return CPI->getStatusAfter(Variable) == Bottom;
  }
  return true;
}

void applyRuleFive(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusAfter(Variable, Bottom);
}

bool checkRuleSix(ConstantPropagationInstruction *CPI, Value *Variable) {
  Instruction *Instr = CPI->getInstruction();
  if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable) {
    if (ConstantInt *ConstInt = dyn_cast<ConstantInt>(Instr->getOperand(0))) {
      return CPI->getStatusAfter(Variable) == Const && CPI->getValueAfter(Variable) == ConstInt->getSExtValue();
    }
  }
  return true;
}

void applyRuleSix(ConstantPropagationInstruction *CPI, Value *Variable, int Value) {
  CPI->setStatusAfter(Variable, Const, Value);
}

bool checkRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable) {
  Instruction *Instr = CPI->getInstruction();
  if (isa<StoreInst>(Instr) && Instr->getOperand(1) == Variable && !isa<ConstantInt>(Instr->getOperand(0))) {
    return CPI->getStatusAfter(Variable) == Top;
  }
  return true;
}

void applyRuleSeven(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusAfter(Variable, Top);
}

bool checkRuleEight(ConstantPropagationInstruction *CPI, Value *Variable) {
  if (isa<StoreInst>(CPI->getInstruction()) && CPI->getInstruction()->getOperand(1) == Variable) {
    return true;
  }
  return CPI->getStatusBefore(Variable) == CPI->getStatusAfter(Variable);
}

void applyRuleEight(ConstantPropagationInstruction *CPI, Value *Variable) {
  CPI->setStatusAfter(Variable, CPI->getStatusBefore(Variable), CPI->getValueBefore(Variable));
}
