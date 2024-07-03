#include "ConstantPropagationInstruction.h"

ConstantPropagationInstruction::ConstantPropagationInstruction(llvm::Instruction *Instr,
                                                               const std::vector<Value *> &Variables)
{
  this->Instr = Instr;
  for (Value *Variable : Variables) {
    setStatusBefore(Variable, Bottom);
    setStatusAfter(Variable, Bottom);
  }
}

void ConstantPropagationInstruction::setStatusAfter(llvm::Value *Variable, Status S, int value)
{
  StatusAfter[Variable] = {S, value};
}

void ConstantPropagationInstruction::setStatusBefore(llvm::Value *Variable, Status S, int value)
{
  StatusBefore[Variable] = {S, value};
}

Status ConstantPropagationInstruction::getStatusAfter(llvm::Value *Variable)
{
  return StatusAfter[Variable].first;
}

Status ConstantPropagationInstruction::getStatusBefore(llvm::Value *Variable)
{
  return StatusBefore[Variable].first;
}

int ConstantPropagationInstruction::getValueBefore(llvm::Value *Variable)
{
  return StatusBefore[Variable].second;
}

int ConstantPropagationInstruction::getValueAfter(llvm::Value *Variable)
{
  return StatusAfter[Variable].second;
}

void ConstantPropagationInstruction::addPredecessor(ConstantPropagationInstruction *Predecessor)
{
  Predecessors.push_back(Predecessor);
}

Instruction *ConstantPropagationInstruction::getInstruction()
{
  return Instr;
}

std::vector<ConstantPropagationInstruction *> ConstantPropagationInstruction::getPredecessors()
{
  return Predecessors;
}
