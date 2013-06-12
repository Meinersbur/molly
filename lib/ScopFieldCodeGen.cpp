#include "ScopFieldCodeGen.h"

#include <polly/ScopPass.h>
#include "MollyContextPass.h"
#include "FieldDetection.h"
#include <polly/Dependences.h>
//#include <llvm/Analysis/ScalarEvolution.h>
#include <polly/ScopInfo.h>
#include "ScopUtils.h"

using namespace molly;
using namespace llvm;
using namespace polly;


namespace molly {
  class ScopFieldCodeGen final : public polly::ScopPass {
  private:
    bool changed;
    FieldDetectionAnalysis *Fields;
    Dependences *Deps;
    MollyContextPass *Ctx;
    isl::Ctx *islctx;
    //ScalarEvolution *SE;

  public:
    static char ID;
    ScopFieldCodeGen() : ScopPass(ID) {
    }


    virtual const char *getPassName() const {
      return "ScopFieldCodeGen";
    }


    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<ScopInfo>(); // because it's a ScopPass
      AU.addRequired<MollyContextPass>();
      AU.addRequired<FieldDetectionAnalysis>();

      AU.addPreserved<MollyContextPass>();
      AU.addPreserved<FieldDetectionAnalysis>();
      AU.addPreserved<polly::ScopInfo>(); //FIXME: Which other analyses must be transitively preserved? (ScopDetection,TempScopInfo,RegionInfo,NaturalLoop,...)
      // It actually preserves everything except polly::Dependences (???)
    }
    virtual void releaseMemory() {
    }

    void processScop(Scop *scop) {
      auto flowDeps = getFlowDependences(Deps);
      flowDeps.foreachMap([] (isl::Map map) {return false;} );

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;

      }

    }

    virtual bool runOnScop(Scop &S) {
      changed = false;
      Ctx = &getAnalysis<MollyContextPass>();
      Fields = &getAnalysis<FieldDetectionAnalysis>();
      Deps = &getAnalysis<Dependences>();
      islctx = Ctx->getIslContext();

      processScop(&S);

      return changed;
    }
  }; // class FieldCodeGen
} // namespace molly



char ScopFieldCodeGen::ID = 0;
char &molly::ScopFieldCodeGenPassID = ScopFieldCodeGen::ID;
static RegisterPass<ScopFieldCodeGen> FieldScopCodeGenRegistration("molly-scopfieldcodegen", "Molly - Modify SCoPs", false, false);

polly::ScopPass *molly::createScopFieldCodeGenPass() {
  return new ScopFieldCodeGen();
}
