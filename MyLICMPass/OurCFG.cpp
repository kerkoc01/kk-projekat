//
// Created by strahinja on 3/15/24.
//

#include "OurCFG.h"

OurCFG::OurCFG(llvm::Function &F)
{
  FunctionName = F.getName().str();
  CreateCFG(F);
}

void OurCFG::CreateCFG(Function &F)
{
  for (BasicBlock &BB : F) {
//    for (BasicBlock *Successor : successors(&BB)) {
//      AdjacencyList[&BB].push_back(Successor);
//    }
    for (Instruction &Instr : BB) {
      if (BranchInst *BranchInstr = dyn_cast<BranchInst>(&Instr)) {
        AdjacencyList[&BB].push_back(BranchInstr->getSuccessor(0));
        if (BranchInstr->isConditional()) {
          AdjacencyList[&BB].push_back(BranchInstr->getSuccessor(1));
        }
      }
    }
  }
}

void OurCFG::DFS(llvm::BasicBlock *Current)
{
  Visited.insert(Current);

  for (BasicBlock *Successor : AdjacencyList[Current]) {
    if (Visited.find(Successor) == Visited.end()) {
      DFS(Successor);
    }
  }
}

bool OurCFG::isReachable(llvm::BasicBlock *BB)
{
  return Visited.find(BB) != Visited.end();
}

void OurCFG::DumpGraphToFile()
{
  std::error_code error;
  raw_fd_ostream File(FunctionName + ".dot", error);

  File << "digraph \"CFG for '" + FunctionName + "' function\" {\n";
  File << "\tlabel=\"CFG for '" + FunctionName + "' function\";\n\n";

  for (const auto &p : AdjacencyList) {
    DumpBlockToFile(File, p.first);
  }

  File << "}\n";
}

void OurCFG::DumpBlockToFile(raw_fd_ostream &File, llvm::BasicBlock *Current)
{
  File << "\tNode" << Current << "[shape=record,color=\"#b70d28ff\", style=filled, fillcolor=\"#b70d2870\",label=\"{";
  for (const Instruction &Instr : *Current) {
    File << Instr << "\\l";
    if (const BranchInst *BranchInstr = dyn_cast<BranchInst>(&Instr)) {
      if (BranchInstr->isConditional()) {
        File << "|{<s0>T|<s1>F}}\"];\n";
      }
      else {
        File << "}\"];\n";
      }
    }
  }

  bool MultipleSuccessors = AdjacencyList[Current].size() > 1;

  int index = 0;
  for (const BasicBlock *Successor : AdjacencyList[Current]) {
    if (MultipleSuccessors) {
      File << "\tNode" << Current << ":s" << index++ << " -> Node" << Successor << ";\n";
    }
    else {
      File << "\tNode" << Current << " -> Node" << Successor << ";\n";
    }
  }
}