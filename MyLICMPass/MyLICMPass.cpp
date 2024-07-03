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
#include "llvm/Support/raw_ostream.h"
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

            //performConstantFolding(F);

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
            } while (CurrChanged);

            //TODO: Julijana - I ovde pozovi constant folding i constant propagation
            //performConstantFolding(F);

            errs() << Changed << " changed!\n";
            return Changed;
        }

        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.setPreservesAll();
        }

        bool isInvariantInstruction(Instruction *I, Loop *L, DominatorTree &DT,
                                    std::vector<Instruction *> instructionsToMove) {
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

            if (isIncrementOrDecrement(I)) {
                uint64_t Iterations;
                if (getLoopIterationCount(L, Iterations)) {
                    if (auto *SI = dyn_cast<StoreInst>(I->getNextNode())) {
                        Value *storedPointer = SI->getPointerOperand();
                        if (!isReferencedInLoop(SI, SI->getPointerOperand(), L) &&
                            L->getLoopLatch() != I->getParent() &&
                            L->getHeader() != I->getParent()) {

                            if (auto *LI = dyn_cast<LoadInst>(I->getPrevNode())) {
                                Value *loadedValue = LI->getPointerOperand();
                                LI->eraseFromParent();

                                // 2. Identify operands
                                ConstantInt *ConstOperand = nullptr;
                                if (isa<ConstantInt>(I->getOperand(0))) {
                                    ConstOperand = cast<ConstantInt>(I->getOperand(0));
                                    I->setOperand(1, loadedValue);
                                } else {
                                    ConstOperand = cast<ConstantInt>(I->getOperand(1));
                                    I->setOperand(0, loadedValue);
                                }

                                // 3. Constant * iterations
                                APInt Op = ConstOperand->getValue();
                                APInt Result = Op * Iterations;
                                ConstantInt *NewConst = ConstantInt::get(ConstOperand->getContext(), Result);
                                I->setOperand(ConstOperand == I->getOperand(0) ? 0 : 1, NewConst);

                                instructionsToMove.push_back(I);
                                instructionsToMove.push_back(SI);

                                return true;
                            }
                        }
                    }
                }
            }

            return false;
        }

        bool getLoopIterationCount(Loop *L, uint64_t &Iterations) {
            // Get the canonical induction variable
            PHINode *InductionVar = L->getCanonicalInductionVariable();
            if (!InductionVar)
                return false;

            // Get the preheader, header, and latch blocks
            BasicBlock *Preheader = L->getLoopPreheader();
            BasicBlock *Header = L->getHeader();
            BasicBlock *Latch = L->getLoopLatch();
            if (!Preheader || !Header || !Latch)
                return false;

            // Get the initial value of the induction variable
            Value *InitialValue = InductionVar->getIncomingValueForBlock(Preheader);
            ConstantInt *InitVal = dyn_cast<ConstantInt>(InitialValue);
            if (!InitVal)
                return false;

            // Get the step value of the induction variable
            Value *StepValue = nullptr;
            for (auto &I : *Latch) {
                if (auto *Inc = dyn_cast<BinaryOperator>(&I)) {
                    if (Inc->getOperand(0) == InductionVar || Inc->getOperand(1) == InductionVar) {
                        StepValue = (Inc->getOperand(0) == InductionVar) ? Inc->getOperand(1) : Inc->getOperand(0);
                        break;
                    }
                }
            }
            ConstantInt *StepVal = dyn_cast<ConstantInt>(StepValue);
            if (!StepVal)
                return false;

            // Get the loop's exit condition
            BranchInst *BI = dyn_cast<BranchInst>(Header->getTerminator());
            if (!BI || !BI->isConditional())
                return false;

            ICmpInst *Cond = dyn_cast<ICmpInst>(BI->getCondition());
            if (!Cond)
                return false;

            Value *Bound = Cond->getOperand(1);
            ConstantInt *BoundVal = dyn_cast<ConstantInt>(Bound);
            if (!BoundVal)
                return false;

            // Calculate the number of iterations
            APInt Initial = InitVal->getValue();
            APInt Step = StepVal->getValue();
            APInt End = BoundVal->getValue();

            ICmpInst::Predicate Pred = Cond->getPredicate();
            bool IsSigned = Cond->isSigned();

            if (Pred == ICmpInst::ICMP_SLT || Pred == ICmpInst::ICMP_ULT) {
                if (IsSigned) {
                    if (Step.isNegative()) {
                        Iterations = ((Initial - End - Step + 1).sdiv(-Step)).getLimitedValue();
                    } else {
                        Iterations = ((End - Initial - Step + 1).sdiv(Step)).getLimitedValue();
                    }
                } else {
                    Iterations = ((End - Initial - Step + 1).udiv(Step)).getLimitedValue();
                }
            } else if (Pred == ICmpInst::ICMP_SLE || Pred == ICmpInst::ICMP_ULE) {
                if (IsSigned) {
                    if (Step.isNegative()) {
                        Iterations = ((Initial - End - Step).sdiv(-Step)).getLimitedValue();
                    } else {
                        Iterations = ((End - Initial - Step).sdiv(Step)).getLimitedValue();
                    }
                } else {
                    Iterations = ((End - Initial - Step).udiv(Step)).getLimitedValue();
                }
            } else if (Pred == ICmpInst::ICMP_SGT || Pred == ICmpInst::ICMP_UGT) {
                if (IsSigned) {
                    if (!Step.isNegative()) {
                        Iterations = ((Initial - End - Step + 1).sdiv(Step)).getLimitedValue();
                    } else {
                        Iterations = ((End - Initial - Step + 1).sdiv(-Step)).getLimitedValue();
                    }
                } else {
                    Iterations = ((Initial - End - Step + 1).udiv(Step)).getLimitedValue();
                }
            } else if (Pred == ICmpInst::ICMP_SGE || Pred == ICmpInst::ICMP_UGE) {
                if (IsSigned) {
                    if (!Step.isNegative()) {
                        Iterations = ((Initial - End - Step).sdiv(Step)).getLimitedValue();
                    } else {
                        Iterations = ((End - Initial - Step).sdiv(-Step)).getLimitedValue();
                    }
                } else {
                    Iterations = ((Initial - End - Step).udiv(Step)).getLimitedValue();
                }
            } else {
                return false;
            }

            return true;
        }

        bool isDesiredInstructionType(Instruction *I) {
            return isa<BinaryOperator>(I) ||
                   isa<SelectInst>(I) ||
                   isa<CastInst>(I) ||
                   isa<GetElementPtrInst>(I);
        }

        bool areAllOperandsConstantsOrComputedOutsideLoop(Instruction *I, Loop *L) {
            for (Use &U: I->operands()) {
                Value *V = U.get();
                if (!isa<Constant>(V)) {
                    if (Instruction * OpInst = dyn_cast<Instruction>(V)) {
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

        std::vector<BasicBlock *> getExitBlocks(Loop *L) {
            std::vector < BasicBlock * > exitBlocks;
            for (BasicBlock *BB: L->blocks()) {
                for (BasicBlock *Succ: successors(BB)) {
                    if (!L->contains(Succ)) {
                        exitBlocks.push_back(Succ);
                    }
                }
            }
            return exitBlocks;
        }

        bool doesBlockDominateAllExitBlocks(BasicBlock *BB, Loop *L, DominatorTree *DT) {
            std::vector < BasicBlock * > exitBlocks = getExitBlocks(L);
            for (BasicBlock *ExitBB: exitBlocks) {
                if (!DT->dominates(BB, ExitBB)) {
                    return false;
                }
            }
            return true;
        }

        bool isDefinedOutsideLoop(Value *V, Loop *L) {
            if (Instruction * Inst = dyn_cast<Instruction>(V)) {
                return !L->contains(Inst->getParent());
            }
            return true;
        }

        bool isReferencedInLoop(Instruction *StartInst, Value *Ptr, Loop *L) {
            for (BasicBlock *BB: L->blocks()) {
                for (Instruction &I: *BB) {
                    if (&I != StartInst) {
                        if (auto *SI = dyn_cast<StoreInst>(&I)) {
                            if (SI->getPointerOperand() == Ptr) {
                                return true;
                            }
                        }
                        if (auto *CI = dyn_cast<CmpInst>(&I)) {
                            for (Use &U: CI->operands()) {
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


        void performConstantFolding(Function &F) {
            std::vector < Instruction * > InstructionsToRemove;

            for (BasicBlock &BB: F) {
                for (Instruction &I: BB) {
                    if (isa<BinaryOperator>(&I)) {
                        handleBinaryOperator(I, InstructionsToRemove);
                    } else if (isa<ICmpInst>(&I)) {
                        handleCompareInstruction(I, InstructionsToRemove);
                    } else if (isa<BranchInst>(&I)) {
                        handleBranchInstruction(I, InstructionsToRemove);
                    }
                }
            }

            for (Instruction *Instr: InstructionsToRemove) {
                Instr->eraseFromParent();
            }
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
                        // Always take the true branch
                        BasicBlock *TrueBB = BranchInstr->getSuccessor(0);
                        BranchInst::Create(TrueBB, BranchInstr->getParent());
                    } else {
                        // Always take the false branch
                        BasicBlock *FalseBB = BranchInstr->getSuccessor(1);
                        BranchInst::Create(FalseBB, BranchInstr->getParent());
                    }

                    InstructionsToRemove.push_back(&I);
                }
            }
        }
    };
}

char MyLICMPass::ID = 0;
static RegisterPass<MyLICMPass> X("my-licm", "My Loop Invariant Code Motion Pass", false, false);

