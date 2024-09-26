#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"
<<<<<<< Updated upstream
#include <vector>

=======
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/raw_ostream.h"

#include "ConstantFolding.h"
#include "ConstantPropagation.h"
#include "DeadCodeElimination.h"

#include <vector>
#include <map>

>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
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
=======
            ConstantPropagation *Propagation = new ConstantPropagation();
            ConstantFolding *Folding = new ConstantFolding();
            DeadCodeElimination *Elimination = new DeadCodeElimination();        

            bool prepChanged;
            /*do {
                prepChanged = false;
                errs() << "Running Constant Propagation\n";
                prepChanged |= Propagation->runOnFunction(F);
                errs() << "Running Constant Folding\n";
                prepChanged |= Folding->runOnFunction(F);
                errs() << "Running Dead Code Elimination\n";
                prepChanged |= Elimination->runOnFunction(F);
            } while(prepChanged);
            Changed = prepChanged;*/

            for (Loop *L: LI) {
                if (!L->getLoopPreheader()) {
                    errs() << "No loop preheader, skipping loop.\n";
                    continue;
                }

                std::vector<Instruction *> instructionsToMove;

                for (BasicBlock *BB: L->blocks()) {
                    for (Instruction &I: *BB) {
                        Changed |= isInvariantInstruction(&I, L, DT, instructionsToMove);
>>>>>>> Stashed changes
                    }
                }
                for(Instruction* I : instructionsToMove) {
                    errs() << "Instruction to move: " << *I << "\n";
                    errs() << "Where to move it: " << *L->getLoopPreheader()->getTerminator() << "\n";
                    I->moveBefore(L->getLoopPreheader()->getTerminator());
                }
            }

<<<<<<< Updated upstream
=======
            /*do {
                prepChanged = false;
                errs() << "Running Constant Propagation\n";
                prepChanged |= Propagation->runOnFunction(F);
                errs() << "Running Constant Folding\n";
                prepChanged |= Folding->runOnFunction(F);
                errs() << "Running Dead Code Elimination\n";
                prepChanged |= Elimination->runOnFunction(F);
            } while(prepChanged);*/

>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
            return false;
        }

=======

            else if (auto *SI = dyn_cast<StoreInst>(I)) {
                if (!isChangedInLoop(SI, SI->getPointerOperand(), L)) {
                    Value *StoredVal = SI->getValueOperand();
                    if (isa<Constant>(StoredVal)) {
                        instructionsToMove.push_back(I);
                    }
                }
            }

            else if (isIncrementOrDecrement(I) && I->getParent() != L->getHeader() && I->getParent() != L->getLoopLatch()) {
                Value *IterationCount = getLoopIterationCount(L);
                if(IterationCount != nullptr)
                {
                    errs() << "IterationCount: " << *IterationCount << "\n";
                    ConstantInt *IterationsConst = dyn_cast<ConstantInt>(IterationCount);
                    if (IterationsConst != nullptr) {
                        errs() << "IterationsConst: " << *IterationsConst << "\n";
                        if (auto *SI = dyn_cast<StoreInst>(I->getNextNode())) {
                            if (auto *LI = dyn_cast<LoadInst>(I->getPrevNode())) {
                                if (!isReferencedInLoop(SI, LI, SI->getPointerOperand(), L)) {                                
                                    ConstantInt *ConstOperand = nullptr;
                                    if (ConstantInt *CI0 = dyn_cast<ConstantInt>(I->getOperand(0))) {
                                        ConstOperand = CI0;
                                    } else if (ConstantInt *CI1 = dyn_cast<ConstantInt>(I->getOperand(1))) {
                                        ConstOperand = CI1;
                                    } else {
                                        return false;
                                    }

                                    IRBuilder<> builder(L->getLoopPreheader()->getTerminator());
                                    Value *Result = builder.CreateMul(ConstOperand, IterationsConst);
                                    I->setOperand(ConstOperand == I->getOperand(0) ? 0 : 1, Result);

                                    instructionsToMove.push_back(LI);
                                    instructionsToMove.push_back(I);
                                    instructionsToMove.push_back(SI);

                                    for (Instruction *Instr : instructionsToMove) {
                                        errs() << *Instr << "\n";
                                    }

                                    return true;
                                }
                            }
                        }
                    }
                }
            }

            return false;
        }

        Value* getLoopIterationCount(Loop *L) {
            BasicBlock* Header = L->getHeader();
            Value *HeaderOp = nullptr;

            for (Instruction &I : *Header) {
                if (auto *CI = dyn_cast<CmpInst>(&I)) {
                    if (CI->getOpcode() == Instruction::ICmp) {
                        if(HeaderOp != nullptr){
                            return nullptr;
                        }
                        HeaderOp = CI->getOperand(1);
                        if(!isa<ConstantInt>(HeaderOp) && isChangedInLoop(&I, HeaderOp, L)){
                            return nullptr;
                        }
                    }
                }
            }


            BasicBlock* Latch = L->getLoopLatch();
            Value *LatchOp = nullptr;

            for (Instruction &I : *Latch) {
                if (auto *AddInst = dyn_cast<BinaryOperator>(&I)) {
                    if (AddInst->getOpcode() == Instruction::Add) {
                        if(LatchOp != nullptr){
                            return nullptr;
                        }
                        LatchOp = AddInst->getOperand(1);
                        if (!isa<ConstantInt>(LatchOp)) {
                            return nullptr;
                        }
                    }
                }
            }

            if (isa<ConstantInt>(HeaderOp) && isa<ConstantInt>(LatchOp)) {
                IRBuilder<> builder(L->getLoopPreheader()->getTerminator());

                errs() << "HeaderOp: " << *HeaderOp << "\n";
                errs() << "LatchOp: " << *LatchOp << "\n";

                return  builder.CreateSDiv(HeaderOp, LatchOp);
            }

            return nullptr;
        }

        bool isDesiredInstructionType(Instruction *I) {
            return isa<BinaryOperator>(I) ||
                   isa<SelectInst>(I) ||
                   isa<CastInst>(I) ||
                   isa<GetElementPtrInst>(I);
        }

>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
=======
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

        void handleBinaryOperator(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
            Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
            if (ConstantInt * LhsValue = dyn_cast<ConstantInt>(Lhs)) {
                if (ConstantInt * RhsValue = dyn_cast<ConstantInt>(Rhs)) {
                    int64_t Value = 0;
                    switch (I.getOpcode()) {
                        case Instruction::Add:
                            Value = LhsValue->getSExtValue() + RhsValue->getSExtValue();
                            break;
                        case Instruction::Sub:
                            Value = LhsValue->getSExtValue() - RhsValue->getSExtValue();
                            break;
                        case Instruction::Mul:
                            Value = LhsValue->getSExtValue() * RhsValue->getSExtValue();
                            break;
                        case Instruction::SDiv:
                            if (RhsValue->getSExtValue() == 0) {
                                errs() << "Division by zero is not allowed!\n";
                                return;
                            }
                            Value = LhsValue->getSExtValue() / RhsValue->getSExtValue();
                            break;
                        default:
                            return;
                    }

                    I.replaceAllUsesWith(ConstantInt::get(I.getType(), Value));
                    InstructionsToRemove.push_back(&I);
                }
            }
        }


        void handleCompareInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
            Value *Lhs = I.getOperand(0), *Rhs = I.getOperand(1);
            if (ConstantInt * LhsValue = dyn_cast<ConstantInt>(Lhs)) {
                if (ConstantInt * RhsValue = dyn_cast<ConstantInt>(Rhs)) {
                    bool Value = false;
                    ICmpInst *Cmp = cast<ICmpInst>(&I);
                    switch (Cmp->getPredicate()) {
                        case ICmpInst::ICMP_EQ:
                            Value = LhsValue->getSExtValue() == RhsValue->getSExtValue();
                            break;
                        case ICmpInst::ICMP_NE:
                            Value = LhsValue->getSExtValue() != RhsValue->getSExtValue();
                            break;
                        case ICmpInst::ICMP_SGT:
                            Value = LhsValue->getSExtValue() > RhsValue->getSExtValue();
                            break;
                        case ICmpInst::ICMP_SLT:
                            Value = LhsValue->getSExtValue() < RhsValue->getSExtValue();
                            break;
                        case ICmpInst::ICMP_SGE:
                            Value = LhsValue->getSExtValue() >= RhsValue->getSExtValue();
                            break;
                        case ICmpInst::ICMP_SLE:
                            Value = LhsValue->getSExtValue() <= RhsValue->getSExtValue();
                            break;
                        default:
                            return;
                    }

                    I.replaceAllUsesWith(ConstantInt::get(I.getType(), Value));
                    InstructionsToRemove.push_back(&I);
                }
            }
        }


        void handleBranchInstruction(Instruction &I, std::vector<Instruction *> &InstructionsToRemove) {
            BranchInst *BranchInstr = dyn_cast<BranchInst>(&I);
            if (BranchInstr->isConditional()) {
                if (ConstantInt * Condition = dyn_cast<ConstantInt>(BranchInstr->getCondition())) {
                    if (Condition->isOne()) {
                        BasicBlock *TrueBB = BranchInstr->getSuccessor(0);
                        BranchInst::Create(TrueBB, BranchInstr->getParent());
                    } else {
                        BasicBlock *FalseBB = BranchInstr->getSuccessor(1);
                        BranchInst::Create(FalseBB, BranchInstr->getParent());
                    }

                    InstructionsToRemove.push_back(&I);
                }
            }
        }

>>>>>>> Stashed changes
    };
}

char MyLICMPass::ID = 0;
<<<<<<< Updated upstream
static RegisterPass<MyLICMPass> X("my-licm", "My Loop Invariant Code Motion Pass", false, false);
=======
static RegisterPass<MyLICMPass> X("my-licm", "My Loop Invariant Code Motion Pass", false, false);
>>>>>>> Stashed changes
