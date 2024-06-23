#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
#include <vector>

using namespace llvm;

namespace {
    struct MyLICMPass : public FunctionPass {
        static char ID;
        MyLICMPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            bool Changed = false;
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
            DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

            errs() << "Processing function: " << F.getName() << "\n";

            for (Loop *L : LI) {
                errs() << "Loop Preheader: " << *L->getLoopPreheader()->getTerminator() << "\n";
                std::vector<Instruction*> instructionsToMove;
                for (BasicBlock *BB : L->blocks()) {
                    for (Instruction &I : *BB) {
                        errs() << "Instruction: " << I << "\n";
                        errs() << "isDesiredInstructionType: " << isDesiredInstructionType(&I)<< "\n";
                        errs() << "areAllOperandsConstantsOrComputedOutsideLoop: " << areAllOperandsConstantsOrComputedOutsideLoop(&I, L) << "\n";
                        errs() << "isSafeToSpeculativelyExecute: " << isSafeToSpeculativelyExecute(&I) << "\n";
                        errs() << "doesBlockDominateAllExitBlocks: " << doesBlockDominateAllExitBlocks(BB, L, &DT) << "\n";
                        if (isDesiredInstructionType(&I) &&
                            areAllOperandsConstantsOrComputedOutsideLoop(&I, L) &&
                            isSafeToSpeculativelyExecute(&I) &&
                            doesBlockDominateAllExitBlocks(BB, L, &DT)) {
                            Changed = true;
                        }
                        else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                            errs() << "Store Instruction!\n";
                            if(!isChangedAfterInstruction(SI, SI->getPointerOperand(), L)) {
                                Value *StoredVal = SI->getValueOperand();
                                if (isa<Constant>(StoredVal)) {
                                    errs() << "Moving!\n";
                                    instructionsToMove.push_back(&I);
                                    Changed = true;
                                } else if (isDefinedOutsideLoop(StoredVal, L)) {
                                    //TODO:Izbrisati load i store i zameniti SI u petlji sa promenljivom koja se storeuje u SI
                                    Changed = true;
                                }
                            }
                        }
                        errs() << "\n\n\n";
                    }
                }
                for(Instruction* I : instructionsToMove) {
                    errs() << "Instruction to move: " << *I << "\n";
                    errs() << "Where to move it: " << *L->getLoopPreheader()->getTerminator() << "\n";
                    I->moveBefore(L->getLoopPreheader()->getTerminator());
                }
            }

            errs() << Changed << " changed!\n";

            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.setPreservesAll();
        }

        bool isDesiredInstructionType(Instruction *I) {
            if (isa<BinaryOperator>(I) ||
                isa<SelectInst>(I) ||
                isa<CastInst>(I) ||
                isa<GetElementPtrInst>(I)) {
                return true;
            }
            return false;
        }

        bool areAllOperandsConstantsOrComputedOutsideLoop(Instruction *I, Loop *L) {
            for (Use &U : I->operands()) {
                Value *V = U.get();
                if (!isa<Constant>(V)) {
                    if (Instruction *OpInst = dyn_cast<Instruction>(V)) {
                        if (L->contains(OpInst->getParent())) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            }
            return true;
        }

        std::vector<BasicBlock*> getExitBlocks(Loop *L) {
            std::vector<BasicBlock*> exitBlocks;
            for (BasicBlock *BB : L->blocks()) {
                for (BasicBlock *Succ : successors(BB)) {
                    if (!L->contains(Succ)) {
                        exitBlocks.push_back(Succ);
                    }
                }
            }
            return exitBlocks;
        }

        bool doesBlockDominateAllExitBlocks(BasicBlock *BB, Loop *L, DominatorTree *DT) {
            std::vector<BasicBlock*> exitBlocks = getExitBlocks(L);
            for (BasicBlock *ExitBB : exitBlocks) {
                if (!DT->dominates(BB, ExitBB)) {
                    return false;
                }
            }
            return true;
        }

        bool isDefinedOutsideLoop(Value *V, Loop *L) {
            if (Instruction *Inst = dyn_cast<Instruction>(V)) {
                return !L->contains(Inst->getParent());
            }
            return true;
        }

        bool isChangedAfterInstruction(Instruction *StartInst, Value *Ptr, Loop *L) {
            bool StartFound = false;
            for (BasicBlock *BB : L->blocks()) {
                for (Instruction &I : *BB) {
                    if (&I == StartInst) {
                        StartFound = true;
                        continue;
                    }

                    if (StartFound) {
                        if (auto *SI = dyn_cast<StoreInst>(&I)) {
                            if (SI->getPointerOperand() == Ptr) {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

    };
}

char MyLICMPass::ID = 0;
static RegisterPass<MyLICMPass> X("my-licm", "My Loop Invariant Code Motion Pass", false, false);
