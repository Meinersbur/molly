#define DEBUG_TYPE "molly"
#include "ScopFieldCodeGen.h"

#include <polly/ScopPass.h>
#include "MollyContextPass.h"
#include "FieldDetection.h"
#include <polly/Dependences.h>
//#include <llvm/Analysis/ScalarEvolution.h>
#include <polly/ScopInfo.h>
#include "ScopUtils.h"
#include <llvm/Support/Debug.h>
#include <polly/PollyContextPass.h>
#include <islpp/Map.h>
#include <islpp/UnionMap.h>

using namespace molly;
using namespace llvm;
using namespace polly;


namespace molly {
  class ScopFieldCodeGen LLVM_FINAL : public polly::ScopPass {
  private:
    bool changed;
    FieldDetectionAnalysis *Fields;
    Dependences *Deps;
    MollyContextPass *Ctx;
    isl::Ctx *islctx;
    //ScalarEvolution *SE;

    isl::UnionMap flowDeps;

  protected:
    void modifiedIR() {
    changed = true;
    }

    void modifiedScop() {
    }

  public:
    static char ID;
    ScopFieldCodeGen() : ScopPass(ID) {
    }


    const char *getPassName() const LLVM_OVERRIDE {
      return "ScopFieldCodeGen";
    }


    void getAnalysisUsage(AnalysisUsage &AU) const LLVM_OVERRIDE {
      ScopPass::getAnalysisUsage(AU); // Calls AU.setPreservesAll()
      AU.addRequired<ScopInfo>(); // because it's a ScopPass
      AU.addRequired<MollyContextPass>();
      AU.addRequired<FieldDetectionAnalysis>();
      AU.addRequired<polly::Dependences>();

      AU.addPreserved<PollyContextPass>();
      AU.addPreserved<MollyContextPass>();
      AU.addPreserved<FieldDetectionAnalysis>();
      AU.addPreserved<polly::ScopInfo>(); //FIXME: Which other analyses must be transitively preserved? (ScopDetection,TempScopInfo,RegionInfo,NaturalLoop,...)
      // It actually preserves everything except polly::Dependences (???)
      AU.addPreserved<polly::Dependences>();
    }
    void releaseMemory() LLVM_OVERRIDE {
    }


    void processScopStmt(ScopStmt *stmt) {
      auto where = getWhereMap(stmt);
     
    }


    void processScop(Scop *scop) {
      auto flowDeps = getFlowDependences(Deps);
      flowDeps.foreachMap([] (isl::Map map) {return false;} );

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        processScopStmt(stmt);
      }
    }


    bool runOnScop(Scop &S) LLVM_OVERRIDE {
      auto &F = *S.getRegion().getEntry()->getParent();
      DEBUG(llvm::dbgs() << "run ScopFieldCodeGen on " << S.getNameStr() << " in func " << F.getName() << "\n");
      if (F.getName() == "test") {
        int a = 0;
      }

      changed = false;
      Ctx = &getAnalysis<MollyContextPass>();
      islctx = Ctx->getIslContext();
      Fields = &getAnalysis<FieldDetectionAnalysis>();
      Deps = &getAnalysis<Dependences>();
      
      flowDeps = getFlowDependences(Deps);

      processScop(&S);

      return changed;
    }
  }; // class FieldCodeGen
} // namespace molly



char ScopFieldCodeGen::ID = 0;
char &molly::ScopFieldCodeGenPassID = ScopFieldCodeGen::ID;
static RegisterPass<ScopFieldCodeGen> FieldScopCodeGenRegistration("molly-scopfieldcodegen", "Molly - Modify SCoPs");

polly::ScopPass *molly::createScopFieldCodeGenPass() {
  return new ScopFieldCodeGen();
}
