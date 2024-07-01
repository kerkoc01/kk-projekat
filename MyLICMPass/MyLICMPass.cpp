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
            bool CurrChanged = false;
            LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
            DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

            errs() << "Processing function: " << F.getName() << "\n";

            /*TODO: Julijana - uradi ipak constant folding i constant propagation.
                               Ovo ima vec gotovo, a i na vezbama je radjeno.
                               Bolje kao na vezbama, jer je gotovo 5 redova koda, al samo ako imas vremena
                               Uradi da bude u funkciji. Nzm dal su tamo tako.
                               Menjanjem konstantih promenljivih konstantama pisemo optimizaciju samo za konstante.*/
            do {
                for (Loop *L: LI) {
                    if (!L->getLoopPreheader()) {
                        errs() << "No loop preheader, skipping loop.\n";
                        continue;
                    }

                    std::vector < Instruction * > instructionsToMove;

                    for (BasicBlock *BB: L->blocks()) {
                        for (Instruction &I: *BB) {
                            CurrChanged = isInvariantInstruction(&I, L, DT, instructionsToMove);
                            Changed |= CurrChanged;
                        }
                    }

                    for (Instruction *I: instructionsToMove) {
                        errs() << "Instruction to move: " << *I << "\n";
                        errs() << "Where to move it: " << *L->getLoopPreheader()->getTerminator() << "\n";
                        I->moveBefore(L->getLoopPreheader()->getTerminator());
                    }
                }
            } while(CurrChanged);

            //TODO: Julijana - I ovde pozovi constant folding i constant propagation

            errs() << Changed << " changed!\n";
            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.setPreservesAll();
        }

        bool isInvariantInstruction(Instruction *I, Loop *L, DominatorTree &DT, std::vector < Instruction * > instructionsToMove) {
            if (isDesiredInstructionType(I) &&
                areAllOperandsConstantsOrComputedOutsideLoop(I, L) &&
                isSafeToSpeculativelyExecute(I) &&
                doesBlockDominateAllExitBlocks(I->getParent(), L, &DT)) {
                instructionsToMove.push_back(I);
                return true;
            }

            if (auto *SI = dyn_cast<StoreInst>(I)) {
                if (!isReferencedInLoop(SI, SI->getPointerOperand(), L)) {
                    Value *StoredVal = SI->getValueOperand();
                    if (isa<Constant>(StoredVal)) {
                        instructionsToMove.push_back(I);
                        return true;
                    }
                }
            }

            if(isIncrementOrDecrement(I)){
                auto *SI = cast<StoreInst>(I->getNextNode());
                Value *storedPointer = SI->getPointerOperand();
                if (!isReferencedInLoop(SI, SI->getPointerOperand(), L) && L->getLoopLatch() != I->getParent() && L->getHeader() != I->getParent()) {
                /*TODO: Petra - Ovde izbrises load koji je sigurno iznad ove instrukcije
                                Onda promenis jedan operand (koji je promenljiva) da bude ono sto je bilo i load-u
                                Onda pomnozis drugi operand (koji je konstanta) sa brojem iteracija petlje (mozda mora nova instrukcija da se doda iznad ove za ovo)
                                Onda ovu instrukciju/e i store koji je ispred ove instrukcije push_back u instructionsToMove*/
                errs() << "Increment!\n" << *I << "\n";
                return true;
                }
            }

            return false;
        }

        bool isDesiredInstructionType(Instruction *I) {
            return isa<BinaryOperator>(I) ||
                   isa<SelectInst>(I) ||
                   isa<CastInst>(I) ||
                   isa<GetElementPtrInst>(I);
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

        bool isReferencedInLoop(Instruction *StartInst, Value *Ptr, Loop *L) {
            for (BasicBlock *BB : L->blocks()) {
                for (Instruction &I : *BB) {
                    if(&I != StartInst) {
                        if (auto *SI = dyn_cast<StoreInst>(&I)) {
                            if (SI->getPointerOperand() == Ptr) {
                                return true;
                            }
                        }
                        if (auto *CI = dyn_cast<CmpInst>(&I)) {
                            for (Use &U : CI->operands()) {
                                Value *V = U.get();
                                if (V == Ptr) {
                                    return true;
                                }
                            }
                        }
                    }

                }
            }
            return false;
        }

        bool isIncrementOrDecrement(Instruction *I) {
            if (auto *BI = dyn_cast<BinaryOperator>(I)) {
                if (BI->getOpcode() == Instruction::Add || BI->getOpcode() == Instruction::Sub) {
                    if (I->getPrevNode() && isa<LoadInst>(I->getPrevNode())) {
                        auto *LI = cast<LoadInst>(I->getPrevNode());
                        Value *loadedValue = LI->getPointerOperand();

                        if (I->getNextNode() && isa<StoreInst>(I->getNextNode())) {
                            auto *SI = cast<StoreInst>(I->getNextNode());
                            Value *storedPointer = SI->getPointerOperand();
                            Value *storedValue = SI->getValueOperand();

                            if (storedPointer == loadedValue && storedValue == I) {
                                if (isa<ConstantInt>(BI->getOperand(1)) || isa<ConstantInt>(BI->getOperand(0))) {
                                    return true;
                                }
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

