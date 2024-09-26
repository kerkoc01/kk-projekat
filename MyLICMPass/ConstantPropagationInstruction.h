//
// Created by strahinja on 4/5/24.
//

#ifndef LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H
#define LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <unordered_map>
#include <vector>

using namespace llvm;

enum Status {
  Top,
  Bottom,
  Const
};

class ConstantPropagationInstruction
{
private:
  Instruction *Instr;
  std::unordered_map<Value *, std::pair<Status, int>> StatusBefore;
  std::unordered_map<Value *, std::pair<Status, int>> StatusAfter;
  std::vector<ConstantPropagationInstruction *> Predecessors;

public:
  ConstantPropagationInstruction(Instruction *, const std::vector<Value *>&);
  Status getStatusBefore(Value *);
  Status getStatusAfter(Value *);
  void setStatusBefore(Value *, Status S, int value = -1);
  void setStatusAfter(Value *, Status S, int value = -1);
  int getValueBefore(Value *);
  int getValueAfter(Value *);
  void addPredecessor(ConstantPropagationInstruction *);
  std::vector<ConstantPropagationInstruction *> getPredecessors();
  Instruction *getInstruction();
};

#endif // LLVM_PROJECT_CONSTANTPROPAGATIONINSTRUCTION_H
