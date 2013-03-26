#include "ScopDistribution.h"

#include "molly/FieldDetection.h"

#include <llvm/Pass.h>
#include <polly/ScopPass.h>
#include <polly/ScopDetection.h>
#include <polly/ScopInfo.h>
#include <polly/Dependences.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


namespace molly {
  class ScopDistribution : public polly::ScopPass {
  public:
    static char ID;
        ScopDistribution() : ScopPass(ID) {
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<FieldDetectionAnalysis>();
      //AU.addRequiredTransitive<ScopDetection>();
      AU.addRequired<Dependences>();
      AU.setPreservesAll();
    }


    void processScopStmt(ScopStmt *stmt) {
#if 0
      stmt->getIslCtx();
      stmt->getDomain();
      stmt->getDomainSpace();
      stmt->getDomainId();
      stmt->getDomainStr();

      stmt->getScattering(); // isl_map
      stmt->getScatteringStr();
      stmt->getBasicBlock();
      stmt->lookupAccessFor();
      stmt->memacc_begin(); stmt->memacc_end();

      stmt->getNumParams();
      stmt->getNumIterators();
      stmt->getNumScattering(); // dim

      stmt->getParent(); // Scop
      stmt->getBaseName();

      stmt->getInductionVariableForDimension(); // PHINode
      stmt->getLoopForDimension();
#endif
    }


    void processMemoryAccess(MemoryAccess *memacc) {
#if 0
      memacc->isRead();
      memacc->isWrite();
      memacc->getAccessRelation(); // isl_map
      memacc->getAccessRelationStr();
      memacc->getBaseAddr();
      memacc->getBaseName();
      memacc->getAccessInstruction();

      memacc->getStride(); // isl_set

      memacc->getStatement(); // ScopStmt
#endif
    }


    void executeIfLocal(MollyFieldAccess *acc) {
    }


    virtual bool runOnScop(Scop &S) {
      auto &fields = getAnalysis<FieldDetectionAnalysis>();
      auto &deps = getAnalysis<Dependences>();
      //auto &scops = getAnalysis<ScopDetection>();


      for (auto itStmt = S.begin(), endStmt = S.end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
          auto memacc = *itAcc;

          if (memacc->isWrite()) {
            // Only execute writes for those points that are local
            // Find the inputs it depends on and insert a send/recv for those that are remote
          }
        }
      }

#if 0
      S.getSE();

      S.getParams();
      S.getIdForParam();
      S.getRegion();
      S.getMaxLoopDepth();
      S.getScatterDim();
      S.getNameStr();

      S.getIslCtx();
      S.getContext(); // return isl_set
      S.getParamSpace();

      S.begin(); S.end(); // iterate ScopStmt
      S.getDomains(); // isl_union_set
      
      deps.isValidScattering();
      deps.isParallelDimension();
      deps.getDependences();
#endif
      return false;
    }


  }; // casss ScopDistribution
} // namespace molly


char ScopDistribution::ID = 0;
char &molly::ScopDistributionPassID = ScopDistribution::ID;
static RegisterPass<ScopDistribution> ScopDistributionRegistration("molly-scopdistr", "Molly - Modify SCoP according value distribution");
