#include "ScopDistribution.h"

#include "molly/FieldDetection.h"
#include "MollyFieldAccess.h"

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
  private:
    FieldDetectionAnalysis *Fields;
    Dependences *Deps;
  public:
    static char ID;
        ScopDistribution() : ScopPass(ID) {
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<FieldDetectionAnalysis>();
      //AU.addRequired<ScopDetection>();
      AU.addRequired<Dependences>();

      // This Pass only modifies the ScopInfo
      AU.setPreservesAll();
    }


    void processScopStmt(ScopStmt *stmt) {

    }


    void processMemoryAccess(MemoryAccess *memacc) {

    }


    void executeIfLocal(MollyFieldAccess *acc) {
    }


    void processScop(Scop *scop) {
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
          auto memacc = *itAcc;

          //MollyFieldAccess acc = MollyFieldAccess::fromMem


        }
      }

    }

    virtual bool runOnScop(Scop &S) {
      Fields = &getAnalysis<FieldDetectionAnalysis>();
      Deps = &getAnalysis<Dependences>();

      processScop(&S);
      return false;
    }


  }; // class ScopDistribution
} // namespace molly


char ScopDistribution::ID = 0;
char &molly::ScopDistributionPassID = ScopDistribution::ID;
static RegisterPass<ScopDistribution> ScopDistributionRegistration("molly-scopdistr", "Molly - Modify SCoP according value distribution");
