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
#include "llvm/IR/IRBuilder.h" 
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
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
                BasicBlock *Preheader = L->getLoopPreheader();
                if (!Preheader) continue;

                IRBuilder<> Builder(Preheader->getTerminator());

                // Create global or local variable for counting (incrementing)
                AllocaInst *Counter = Builder.CreateAlloca(Type::getInt32Ty(F.getContext()), nullptr, "counter");
                Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(F.getContext()), 0), Counter);

                std::vector<Instruction*> instructionsToMove;

                for (BasicBlock *BB : L->blocks()) {
                    for (Instruction &I : *BB) {
                        errs() << "Instruction: " << I << "\n";
                        errs() << "isDesiredInstructionType: " << isDesiredInstructionType(&I) << "\n";
                        errs() << "areAllOperandsConstantsOrComputedOutsideLoop: " << areAllOperandsConstantsOrComputedOutsideLoop(&I, L) << "\n";
                        errs() << "isSafeToSpeculativelyExecute: " << isSafeToSpeculativelyExecute(&I) << "\n";
                        errs() << "doesBlockDominateAllExitBlocks: " << doesBlockDominateAllExitBlocks(BB, L, &DT) << "\n";
                        
                        // Add increment at the end of each iteration
                        if (isa<BranchInst>(I) && L->contains(&I)) {
                            IRBuilder<> LoopBuilder(&I);
                            LoadInst *LoadCounter = LoopBuilder.CreateLoad(Type::getInt32Ty(F.getContext()), Counter);
                            Value *Inc = LoopBuilder.CreateAdd(LoadCounter, ConstantInt::get(Type::getInt32Ty(F.getContext()), 1), "inc");
                            LoopBuilder.CreateStore(Inc, Counter);

                            // Add print of the current value of Counter
                            Value *CounterVal = LoopBuilder.CreateLoad(Type::getInt32Ty(F.getContext()), Counter);
                            FunctionCallee DbgFunc = F.getParent()->getOrInsertFunction("llvm_dbg_value", FunctionType::get(Type::getVoidTy(F.getContext()), {Type::getInt32Ty(F.getContext())}, false));
                            LoopBuilder.CreateCall(DbgFunc, {CounterVal});
                        }

                        if (isDesiredInstructionType(&I) &&
                            areAllOperandsConstantsOrComputedOutsideLoop(&I, L) &&
                            isSafeToSpeculativelyExecute(&I) &&
                            doesBlockDominateAllExitBlocks(BB, L, &DT)) {
                            instructionsToMove.push_back(&I);
                            Changed = true;
                        } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                            errs() << "Store Instruction!\n";
                            if(!isChangedAfterInstruction(SI, SI->getPointerOperand(), L)) {
                                Value *StoredVal = SI->getValueOperand();
                                if (isa<Constant>(StoredVal)) {
                                    errs() << "Moving!\n";
                                    instructionsToMove.push_back(&I);
                                    Changed = true;
                                } else if (isDefinedOutsideLoop(StoredVal, L)) {
                                    // Remove all Load/Store instructions and replace them with a new variable
                                    std::vector<Instruction*> toRemove;
                                    for (BasicBlock *InnerBB : L->blocks()) {
                                        for (Instruction &InnerI : *InnerBB) {
                                            if (LoadInst *LI = dyn_cast<LoadInst>(&InnerI)) {
                                                if (LI->getPointerOperand() == SI->getPointerOperand()) {
                                                    // Create a new variable outside the loop
                                                    AllocaInst *NewVar = Builder.CreateAlloca(LI->getType(), nullptr, "hoistedVar");
                                                    Builder.CreateStore(StoredVal, NewVar);

                                                    // Replace all uses of Load with the new variable
                                                    LI->replaceAllUsesWith(Builder.CreateLoad(LI->getType(), NewVar));
                                                    toRemove.push_back(&InnerI);
                                                }
                                            }
                                            if (StoreInst *StI = dyn_cast<StoreInst>(&InnerI)) {
                                                if (StI->getPointerOperand() == SI->getPointerOperand()) {
                                                    toRemove.push_back(&InnerI);
                                                }
                                            }
                                        }
                                    }

                                    // Remove all Load/Store instructions that were replaced
                                    for (Instruction *Instr : toRemove) {
                                        Instr->eraseFromParent();
                                    }

                                    // Move the Store instruction outside the loop
                                    Builder.CreateStore(StoredVal, SI->getPointerOperand());
                                    Changed = true;
                                }
                            }
                        }
                        errs() << "\n\n\n";
                    }
                }

                for (Instruction *I : instructionsToMove) {
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
                    if (StartFound) {
                        if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                            if (SI->getPointerOperand() == Ptr) {
                                return true;
                            }
                        }
                    }
                    if (&I == StartInst) {
                        StartFound = true;
                    }
                }
            }
            return false;
        }
    };
}

char MyLICMPass::ID = 0;
static RegisterPass<MyLICMPass> X("mylicm", "My LICM Pass", false, false);
