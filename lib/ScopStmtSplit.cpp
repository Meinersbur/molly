#include "ScopStmtSplit.h"

#include <polly/ScopPass.h> // polly::ScopPass
#include <polly/ScopDetection.h>
#include <polly/ScopInfo.h>
#include <llvm/ADT/SmallVector.h>
#include "MollyFieldAccess.h"
#include "FieldDetection.h"
#include "MollyUtils.h"
#include <polly/Dependences.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <functional>
#include <llvm/Analysis/ValueTracking.h>
#include "islpp/Map.h"
#include <polly/Support/ScopHelper.h>
#include <llvm/Analysis/LoopInfo.h>
#include "MollyContextPass.h"
#include <llvm/Analysis/ScalarEvolution.h>

using namespace molly;
using namespace llvm;
using namespace std;

using polly::Scop;
using polly::ScopStmt;
using polly::MemoryAccess;


namespace molly {

  class ScopStmtSplitPass : public polly::ScopPass {
  public:
    static char ID;
    ScopStmtSplitPass() : ScopPass(ID) {
    }

    //ScopDetection *scops;
    //FieldDetectionAnalysis *fields;
    bool changed;
    LoopInfo *LI;
    //DominatorTree *DT;
    BasicBlock *allocaBlock;
    Scop *scop;

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      //ScopPass::getAnalysisUsage(AU); // Calls setPreservesAll() !!!

      AU.addRequired<LoopInfo>();
      AU.addRequired<RegionInfo>();

      // Molly
      AU.addPreserved<MollyContextPass>();
      AU.addPreserved<FieldDetectionAnalysis>(); // no changes
     
      // Polly
      AU.addPreserved<polly::ScopDetection>(); // by splitScopStmt
      AU.addPreserved<polly::ScopInfo>(); // by splitScopStmt

      // LLVM
      AU.addPreserved<DominatorTree>(); // by SplitBlock
      AU.addPreserved<LoopInfo>(); // by SplitBlock
      AU.addPreserved<RegionInfo>(); // by splitScopStmt
      AU.addPreserved<ScalarEvolution>(); // not touched

      // Does not preserve:
      // polly::Dependences 

      // Open:
      // DominanceFrontier
      // PostDominatorTree
      // TempScopInfo
    }


    void collectInstructionOps(User *user, SmallVectorImpl<Instruction*> &list) {
      for (auto itOp = user->op_begin(), endOp = user->op_end(); itOp!=endOp; ++itOp) {
        auto op = itOp->get();
        assert(itOp->getUser() == user);
        if (auto opInstr = dyn_cast<Instruction>(op)) {
          list.push_back(opInstr);
        } else if (auto opUser = dyn_cast<User>(op)) {
          collectInstructionOps(opUser, list);
        }
      }
    }


    void collectInstructionOpChain(User *user, SmallVectorImpl<Instruction*> &list,  std::function<bool(Instruction*)> predAdd, std::function<bool(User*)> predFollow) {
      SmallVector<User *, 32> todo;
      todo.push_back(user);

      while (!todo.empty()) {
        auto cur = todo.pop_back_val();

        for (auto itOp = cur->op_begin(), endOp = cur->op_end(); itOp!=endOp; ++itOp) {
          auto op = itOp->get();

          if (auto opInstr = dyn_cast<Instruction>(op)) {
            if (predAdd(opInstr)) 
              list.push_back(opInstr);
          }
          if (auto opUser = dyn_cast<User>(op)) {
            if (predFollow(opUser)) 
              todo.push_back(opUser);
          }
        }
      }
    }


    void collectInstructionUses(Value *val, SmallVectorImpl<Instruction*> &list) {
      for (auto itUse = val->use_begin(), endUse = val->use_end(); itUse!=endUse; ++itUse) {
        auto user = *itUse;
        if (auto userInstr = dyn_cast<Instruction>(user)) {
          list.push_back(userInstr);
        } else {
          collectInstructionUses(user, list);
        }
      }
    }


    bool isIndVar(const Value *val) {
      auto instr = dyn_cast<Instruction>(val);
      if (!instr)
        return false;

        Loop *L = LI->getLoopFor(instr->getParent());
      return L && instr == L->getCanonicalInductionVariable();
    }


    // IndependentBlocks::isEscapeUse
    bool isEscapeUse(const Value *Use) {
      const Region *R = &scop->getRegion();

      // Non-instruction user will never escape.
      if (!isa<Instruction>(Use)) return false;

      return !R->contains(cast<Instruction>(Use));
    }


    // IndependentBlocks::isEscapeOperand
    bool isEscapeOperand(const Value *Operand, const BasicBlock *CurBB) {
      const Region *R = &scop->getRegion();

      const Instruction *OpInst = dyn_cast<Instruction>(Operand);

      // Non-instruction operands will never escape.
      if (OpInst == 0) return false;

      // Induction variables are valid operands.
      if (isIndVar(OpInst)) return false;

      // A value from a different BB is used in the same region.
      return R->contains(OpInst) && (OpInst->getParent() != CurBB);
    }

    bool doAllOpsBelongToBB(Value *val, BasicBlock *bb) {
      assert(val);
      assert(bb);
      if (auto user = dyn_cast<User>(val)) {
        for (auto itOp = user->op_begin(), endOp = user->op_end(); itOp!=endOp; ++itOp) {
          auto op = itOp->get();
          auto instr = dyn_cast<Instruction>(op);
          if (!instr)
            continue; // Constant
          if (instr->getParent() == bb)
            continue;// Fine, this is what we want

          if (isIndVar(instr))
            continue; // Is allowed to be in some other BB
          if (!isEscapeOperand(instr, bb)) 
            continue;

          return false;
        }
      }
      return true;
    }


    bool doAllUsersBelongToBB(Value *value, BasicBlock *bb) {
      assert(value);
      assert(bb);
      if (isIndVar(value))
        return true; // induction variables are allowed to span BBs
      for (auto itUse = value->use_begin(), endUse = value->use_end(); itUse!=endUse; ++itUse) {
        auto use = *itUse;

        auto instr = dyn_cast<Instruction>(use);
        if (!instr)
          continue; // Constant
        if (instr->getParent() == bb)
          continue;// Fine, this is what we want
        if (isEscapeUse(use)) //TODO: This checks only if the value escapes the SCoP, but what if used 
          return false;

        auto otherStmt = scop->getScopStmtFor(instr->getParent());
        if (!otherStmt)
          continue; // Some trivial BB, for instance to compute the next IV
        assert(otherStmt->getBasicBlock() != bb);
        return false;
      }
      return true;
    }


    bool isIndependentBlock(BasicBlock *bb) {
      for(auto it = bb->begin(), end = bb->end(); it!=end;++it) {
        auto instr = &*it;
        if (!doAllOpsBelongToBB(instr, bb))
          return false;

        if (!doAllUsersBelongToBB(instr, bb))
          return false;
      }
      return true;
    }


    // from polly::IndependentBlocks
    bool isMovable(Instruction *instr) {
      if (isa<TerminatorInst>(instr))
        return false;
      if (isa<PHINode>(instr))
        return false; // Do not move induction variables, always stays in first block
      return !instr->mayReadFromMemory() && !instr->mayWriteToMemory() && isSafeToSpeculativelyExecute(instr);
    }


    ScopStmt *splitScopStmt(ScopStmt *oldStmt, Instruction *splitBefore) {
      auto oldBB = oldStmt->getBasicBlock();
      assert(isIndependentBlock(oldBB));

      auto newBB = SplitBlock(oldBB, splitBefore, this);
      auto SE = getAnalysisIfAvailable<ScalarEvolution>();

      // TODO: Move movable instructions into the second BB to reduce the number of trivial dependences
      // polly::IndependentBlocks::moveOperandTree() does about the same
#if 0
      SmallVector<Instruction*, 32> worklist;
      collectInstructionList(newBB, worklist);
      for (auto itInstr = worklist.begin(), endInstr = worklist.end(); itInstr!=endInstr; ++itInstr) {
        auto instr = *itInstr;

        if (!isMovable(instr))
          continue;

        bool noOpsInNewBB = true;
        for (auto itOp = instr->op_begin(), endOp = instr->op_end(); itOp!=endOp; ++itOp) {
          assert(itOp->getUser() == instr);
          auto op = itOp->get();
          assert(isa<Instruction>(op) || isa<Constant>(op));
          auto opInstr = dyn_cast<Instruction>(op);
          if (!opInstr)
            continue; // Constants do no belong to a BB and therefore don't need to be moved

          if (opInstr->getParent() == newBB) {
            noOpsInNewBB = false;
            break;
          } else if (opInstr->getParent() == oldBB) {
            continue;
          } else {
            llvm_unreachable("Should not happen if the blocks were independent before");
          }
        }

        if (noOpsInNewBB) {
          instr->moveBefore(oldBB->getTerminator());
        }
      }
#endif

      // This time, if there are dependences left crossing the blocks, insert "spills" so blocks keep independent
      SmallVector<Instruction*, 32> oldWorklist;
      collectInstructionList(oldBB, oldWorklist);
      for (auto itInstr = oldWorklist.begin(), endInstr = oldWorklist.end(); itInstr!=endInstr;++itInstr) {
        auto instr = *itInstr;
        if (isIndVar(instr))
          continue; // Induction variables do not need to spill

        assert(instr->getParent() == oldBB);
        AllocaInst *slot = NULL;
        LoadInst *reloadInstr = NULL;

        for (auto itUse = instr->use_begin(), endUse = instr->use_end(); itUse!=endUse; ++itUse) {
          auto useInstr = cast<Instruction>(*itUse);
          auto argIdx = itUse.getOperandNo();

          if (useInstr->getParent() == oldBB) {
            // No spill required
          } else if (useInstr->getParent() == newBB) {
            // Do spill
            
            if (!reloadInstr) {
              if (SE)
                SE->forgetValue(instr); // Change of use-def chain (FIXME: The value does not change, is it really necessary to do this?)
              slot = new AllocaInst(instr->getType(), 0,  instr->getName() + ".splitcut2a", allocaBlock); // We are a ScopPass, but this modifies a node outside the Scop!!! 
              (void) new StoreInst(instr, slot, oldBB->getTerminator());
              reloadInstr = new LoadInst(slot, instr->getName()+".a2splitcut", false, newBB->getFirstNonPHI()); //TODO: Update ScopStmt.Access for this memory access
            }
            assert(useInstr->getOperand(argIdx) == instr);
            useInstr->setOperand(argIdx, reloadInstr);
          } else {
            // Interesting case: the user is outside the split block
            // Could be the calculation of the induction variable for the next iteration
            // But nothing concerning the split of this BB that hasn't been like this before
          }
        }
      }

      // Preserve analyses
      auto RI = getAnalysisIfAvailable<RegionInfo>(); // Why isn't SplitBlock doing this?
      if (RI) {
        //assert(RI->isRegion(newBB, oldBB));
        RI->setRegionFor(newBB, RI->getRegionFor(oldBB));
        //RI->createRegion(oldBB, newBB);
        // We could create a new region here?
      }


      SmallVector<Loop*, 8> loopHierachy;
      auto depth = oldStmt->getNumIterators();
      for (auto i = depth-depth; i < depth; i+=1) {
        auto loop = const_cast<Loop*>(oldStmt->getLoopForDimension(i));
        loopHierachy.push_back(loop);
      }

      auto oldScatter = isl::Map::wrap(oldStmt->getScattering());
      auto dim = oldScatter.addOutDim_inplace();
      auto newScatter = oldScatter.copy();

      oldScatter.fix(dim, 0);
      newScatter.fix(dim, 1);




      auto newStmt = new ScopStmt(*oldStmt->getParent(), *oldStmt->getRegion(), *newBB, loopHierachy, oldStmt->getDomain()); 
      newStmt->setScattering(newScatter.take());
      oldStmt->setScattering(oldScatter.take());


      SmallVector<MemoryAccess*, 8> memaccsToMove;
      for (auto itAcc = oldStmt->memacc_begin(), endAcc = oldStmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto acc = *itAcc;

        if (acc->getAccessInstruction()->getParent() != newBB)
          continue;

        memaccsToMove.push_back(acc);
      }

      for (auto itAcc = memaccsToMove.begin(), endAcc = memaccsToMove.end(); itAcc!=endAcc; ++itAcc) {
        auto acc = *itAcc;
        assert(!acc->getNewAccessRelation() && "Not supported here");
        newStmt->addAccess(acc->getAccessType(), acc->getBaseAddr(), acc->getAccessRelation(), acc->getAccessInstruction()); //TODO: We could also move the existing object instead of creating a new one
        oldStmt->removeAccess(acc);
      }


 


      assert(isIndependentBlock(oldBB));
      assert(isIndependentBlock(newBB));
      return newStmt;
    }


    void processScopStmt(ScopStmt *stmt) {
#if 0
      SmallVector<FieldAccess,4> accesses;
      for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto memacc = *itAcc;
        auto fieldacc = fields->getFieldAccess(memacc);
        if (fieldacc.isValid())
          accesses.push_back(fieldacc);
      }

      if (accesses.size() <= 1)
        return; // Nothing to do
      changed = true;
#endif
      // bool changed = false;
      auto scop = stmt->getParent();
      auto bb = stmt->getBasicBlock();
      //fields = &getAnalysis<FieldDetectionAnalysis>();

      // copy the list before modifying it
      SmallVector<Instruction*, 32> instrs;
      collectInstructionList(bb, instrs);

      MollyFieldAccess lastAccess;
      for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
        auto instr = *it;

        auto access = MollyFieldAccess::fromAccessInstruction(instr);
        if (access.isNull())
          continue;

        if (lastAccess.isNull()) {
          lastAccess = access;
          continue;
        }

        auto newStmt = splitScopStmt(stmt, access.getFieldCall()/*or lastAccess.getAccessor()->getNextNode()*/);
        changed = true;
        assert(lastAccess.getAccessor()->getParent() == stmt->getBasicBlock());
        assert(lastAccess.getFieldCall()->getParent() == stmt->getBasicBlock());
        assert(access.getAccessor()->getParent() == newStmt->getBasicBlock());
        assert(access.getFieldCall()->getParent() == newStmt->getBasicBlock());
      

        stmt = newStmt;
        lastAccess = access;
      }
    }

    virtual bool runOnScop(Scop &S) {
      //fields = &getAnalysis<FieldDetectionAnalysis>();
      this->scop = &S;
      LI = &getAnalysis<LoopInfo>();
      //DT = getAnalysisIfAvailable<DominatorTree>();
      auto func = S.getRegion().getEntry()->getParent();
      allocaBlock = &func->getEntryBlock();
      assert(allocaBlock); //TODO: Maybe we need to create a specific allocaBlock?

      // Collect all memory a ScopStmts before modifying the list
      SmallVector<ScopStmt*, 8> stmts;
      collectScopStmts(&S, stmts);

      for (auto itStmt = stmts.begin(), endStmt = stmts.end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        processScopStmt(stmt);
      }
      return changed;
    }

  }; // class ScopStmtSplitPass
} // namespace molly


char ScopStmtSplitPass::ID = 0;
char &molly::ScopStmtSplitPassID = ScopStmtSplitPass::ID;
static RegisterPass<ScopStmtSplitPass> ScopStmtSplitPassRegistration("molly-scopsplit", "Molly - SCoP split");

llvm::Pass *molly::createScopStmtSplitPass() {
  return new ScopStmtSplitPass();
}
