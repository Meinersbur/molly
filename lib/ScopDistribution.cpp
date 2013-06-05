#include "ScopDistribution.h"

#include "molly/FieldDetection.h"
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

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


namespace molly {

  /// Decide for every statement on which node to execute it
  class ScopDistribution : public polly::ScopPass {
  private:
    bool changed;
    FieldDetectionAnalysis *Fields;
    Dependences *Deps;
    MollyContextPass *Ctx;
    isl::Ctx *islctx;
    ScalarEvolution *SE;

  protected:
   void modified() {
      changed = true;
    }

  public:
    static char ID;
        ScopDistribution() : ScopPass(ID) {
    }


    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<molly::FieldDetectionAnalysis>();
      //AU.addRequired<ScopDetection>();
      AU.addRequired<polly::Dependences>();
      AU.addRequired<ScalarEvolution>();

      // This Pass only modifies the ScopInfo
      AU.setPreservesAll();
    }


    void processFieldAccess(MollyFieldAccess &acc, isl::Map &executeWhereWrite, isl::Map &executeWhereRead) {
        auto fieldVar = acc.getFieldVariable();
        auto fieldTy = acc.getFieldType();
        auto stmt = acc.getPollyScopStmt();
        auto scop = stmt->getParent();

        auto rel = acc.getAffineAccess(SE); /* iteration coord -> field coord */
        auto home = fieldTy->getHomeAff(); /* field coord -> cluster coord */
        auto it2rank = rel.toMap().applyRange(home.toMap());

        if (acc.isRead()) {
           executeWhereRead = union_(executeWhereRead.substractDomain(it2rank.getDomain()), it2rank);
        }

        if (acc.isWrite()) {
          executeWhereWrite = union_(executeWhereWrite.substractDomain(it2rank.getDomain()), it2rank);
        }
    }


    void processScopStmt(ScopStmt *stmt) {
      auto itDomain = getIterationDomain(stmt);
      auto itSpace = itDomain.getSpace();

      auto executeWhereWrite = islctx->createEmptyMap(itSpace, Ctx->getClusterSpace());
      auto executeWhereRead = executeWhereWrite.copy();
      auto executeEverywhere = islctx->createAlltoallMap(itDomain, Ctx->getClusterShape());
      auto executeMaster = islctx->createAlltoallMap(itDomain, Ctx->getMasterRank().toMap().getRange());

        for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
           auto memacc = *itAcc;
        MollyFieldAccess acc = MollyFieldAccess::fromMemoryAccess(memacc);
        if (!acc.isValid())
          continue;

        processFieldAccess(acc, executeWhereWrite, executeWhereRead);
        }

        auto result = executeEverywhere;
        result = union_(result.substractDomain(executeWhereRead.getDomain()), executeWhereRead);
        result = union_(result.substractDomain(executeWhereWrite.getDomain()), executeWhereWrite);
        stmt->setWhereMap(result.take());
    }


    void processScop(Scop *scop) {
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        processScopStmt(stmt);
      }
    }


    virtual bool runOnScop(Scop &S) {
      changed = false;
      Fields = &getAnalysis<molly::FieldDetectionAnalysis>();
      Deps = &getAnalysis<polly::Dependences>();
      Ctx = &getAnalysis<molly::MollyContextPass>(); 
      SE = &getAnalysis<ScalarEvolution>(); 
      islctx = Ctx->getIslContext();

      processScop(&S);
      return false;
    }

  }; // class ScopDistribution
} // namespace molly


char ScopDistribution::ID = 0;
char &molly::ScopDistributionPassID = ScopDistribution::ID;
static RegisterPass<ScopDistribution> ScopDistributionRegistration("molly-scopdistr", "Molly - Modify SCoP according value distribution");


 ScopPass *molly::createScopDistributionPass() {
   return new ScopDistribution();
 }
