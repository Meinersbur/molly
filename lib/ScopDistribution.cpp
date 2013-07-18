#define DEBUG_TYPE "molly"
#include "ScopDistribution.h"

#include "FieldDetection.h"
#include "MollyFieldAccess.h"
#include "islpp/Map.h"
#include "MollyContextPass.h"
#include "ScopUtils.h"
#include "FieldType.h"

#include <llvm/Pass.h>
#include <polly/ScopPass.h>
#include <polly/ScopDetection.h>
#include <polly/ScopInfo.h>
#include <polly/Dependences.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Support/Debug.h>
#include <polly/PollyContextPass.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


namespace molly {

  /// Decide for every statement on which node to execute it
  class ScopDistribution : public polly::ScopPass {
  private:
    bool changed;
    Scop *scop;
    FieldDetectionAnalysis *Fields;
    Dependences *Deps;
    MollyContextPass *Ctx;
    isl::Ctx *islctx;
    ScalarEvolution *SE;

  protected:
    void modifiedIR() {
      changed = true;
    }
    void modifiedScop() {
      scop->setCodegenPending(true);
    }

  public:
    static char ID;
    ScopDistribution() : ScopPass(ID) {
    }


    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
      ScopPass::getAnalysisUsage(AU); // Calls AU.setPreservesAll();
      AU.addRequired<polly::PollyContextPass>();
      AU.addRequired<molly::MollyContextPass>();
      AU.addRequired<molly::FieldDetectionAnalysis>();
      AU.addRequired<polly::Dependences>();
      AU.addRequired<polly::ScopInfo>();
      AU.addRequired<llvm::ScalarEvolution>();

      //AU.addPreserved<polly::Dependences>();
      //AU.addPreserved<ScalarEvolution>();
      //AU.addPreserved<polly::ScopInfo>();
      //AU.addPreserved<polly::PollyContextPass>();
      //AU.setPreservesAll();
    }

#if 0
    MollyFieldAccess getTheFieldAccess(ScopStmt *stmt) {
      // Currently we support just one field access per ScopStmt
      // This function searches it or returns an invalid MollyFieldAccess
      MollyFieldAccess result;
      for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto memacc = *itAcc;
        MollyFieldAccess acc = MollyFieldAccess::fromMemoryAccess(memacc);
        if (!acc.isValid())
          continue;

        assert(result.isNull() && "Just one FieldAccess allowed per ScopStmt");
        result = acc;
#ifdef NDEBUG
        break;
#endif
      }
      return result;
    }
#endif


#if 0
    isl::PwMultiAff/* {iteration coord -> node coord} */ computeWhereToExecuteStmt(ScopStmt *stmt) {
      //auto result = islctx->createEmptyMap(Ctx->getClusterShape(), ScopUtils::getIterationDomain(stmt));

      //auto resultSpace = islctx->createMapSpace(Ctx->getClusterSpace(), ScopUtils::getIterationDomain(stmt).getSpace());
      //auto fieldWriteMap = resultSpace.emptyMap();
      //auto fieldReadMap = resultSpace.emptyMap();
      //auto tilingMap = resultSpace.emptyMap();

      // Use the following rules to determine where to execute it

      for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto memacc = *itAcc;
        MollyFieldAccess acc = MollyFieldAccess::fromMemoryAccess(memacc);
        if (!acc.isValid())
          continue;

        auto fieldTy = acc.getFieldType();
        auto fieldVar = acc.getFieldVariable();

        if (acc.isRead()) {
        }

        if (acc.isWrite()) {
          auto rel = acc.getAffineAccess(); /*iteration coord -> field coord*/
          //auto when = reverse(rel.move()); /*field coord -> iteration coords*/

          auto nodecoord2fieldcoordset = fieldTy->getLocalIndexset(Ctx->getClusterShape()); // { nodecoord -> its local gcoords }
          auto fieldcoord2nodecoord = nodecoord2fieldcoordset.reverse(); // { fieldcoord -> nodecoord where it is local }
          auto itercoord2nodecoord = rel.applyRange(fieldcoord2nodecoord);
        }
      }


      return result;
    }
#endif


    void processFieldAccess(MollyFieldAccess &acc, isl::Map &executeWhereWrite, isl::Map &executeWhereRead) {
      if (acc.isPrologue() || acc.isEpilogue()) {
        return; // These are not computed anywhere
      }

      auto fieldVar = acc.getFieldVariable();
      auto fieldTy = acc.getFieldType();
      auto stmt = acc.getPollyScopStmt();
      auto scop = stmt->getParent();

      auto rel = acc.getAffineAccess(SE); /* iteration coord -> field coord */
      auto home = fieldTy->getHomeAff(); /* field coord -> cluster coord */
      auto it2rank = rel.toMap().applyRange(home.toMap());

      if (acc.isRead()) {
        executeWhereRead = unite(executeWhereRead.substractDomain(it2rank.getDomain()), it2rank);
      }

      if (acc.isWrite()) {
        executeWhereWrite = unite(executeWhereWrite.substractDomain(it2rank.getDomain()), it2rank);
      }
    }


    void processScopStmt(ScopStmt *stmt) {
      if (stmt->isPrologue() || stmt->isEpilogue())
        return;

      auto itDomain = getIterationDomain(stmt);
      auto itSpace = itDomain.getSpace();
      
      auto executeWhereWrite = islctx->createEmptyMap(itSpace, Ctx->getClusterSpace());
      auto executeWhereRead = executeWhereWrite.copy();
      auto executeEverywhere = islctx->createAlltoallMap(itDomain, Ctx->getClusterShape());
      auto executeMaster = islctx->createAlltoallMap(itDomain, Ctx->getMasterRank().toMap().getRange());

      for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto memacc = *itAcc;
        auto acc = Fields->getFieldAccess(memacc);
        if (!acc.isValid())
          continue;

        processFieldAccess(acc, executeWhereWrite, executeWhereRead);
      }

      auto result = executeEverywhere;
      result = unite(result.substractDomain(executeWhereRead.getDomain()), executeWhereRead);
      result = unite(result.substractDomain(executeWhereWrite.getDomain()), executeWhereWrite);
      stmt->setWhereMap(result.take());

      modifiedScop();
    }


    void processScop(Scop *scop) {
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        processScopStmt(stmt);
      }
    }


    virtual bool runOnScop(Scop &S) {
      DEBUG(llvm::dbgs() << "run Scopdistribution\n");

      changed = false;
      this->scop = &S;
      Fields = &getAnalysis<molly::FieldDetectionAnalysis>();
      Deps = &getAnalysis<polly::Dependences>();
      Ctx = &getAnalysis<molly::MollyContextPass>(); 
      SE = &getAnalysis<ScalarEvolution>(); 
      islctx = Ctx->getIslContext();

      processScop(&S);

      this->scop = nullptr;
      return changed;
    }

  }; // class ScopDistribution
} // namespace molly


char ScopDistribution::ID = 0;
char &molly::ScopDistributionPassID = ScopDistribution::ID;
static RegisterPass<ScopDistribution> ScopDistributionRegistration("molly-scopdistr", "Molly - Modify SCoP according value distribution");


ScopPass *molly::createScopDistributionPass() {
  return new ScopDistribution();
}
