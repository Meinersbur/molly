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
#include "MollyFieldAccess.h"

using namespace molly;
using namespace llvm;
using namespace polly;
using isl::enwrap;


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
      // Collect information
      // Like polly::Dependences::collectInfo, but finer granularity (MemoryAccess instead ScopStmt)
      DenseMap<isl::Id,MollyFieldAccess> tupleToAccess;

      auto space = enwrap(scop->getParamSpace());
      auto readAccesses = space.createEmptyUnionMap(); /* { stmt[iteration] -> access[indexset] } */
      auto writeAccesses = readAccesses.copy(); /* { stmt[iteration] -> access[indexset] } */
      auto schedule = readAccesses.copy(); /* stmt[iteration] -> scattering[] */

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt ) {
        auto stmt = *itStmt;
        auto domain = getIterationDomain(stmt);
        schedule.addMap_inplace(getScattering(stmt));

        for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) { 
          auto memacc = *itAcc;
          if (!memacc->isFieldAccess())
            continue;

          auto facc = MollyFieldAccess::fromMemoryAccess(memacc);
          assert(facc.isValid());
          auto fvar = facc.getFieldVariable();
          auto fty = facc.getFieldType();

          auto accId = facc.getAccessTupleId();
          assert(!tupleToAccess.count(accId) && "accId must be unique");
          tupleToAccess[accId] = facc;

          auto accessRel = facc.getAccessRelation();
          accessRel.intersectDomain_inplace(domain);
          accessRel.setOutTupleId_inplace(accId);

          //FIXME: access withing the same stmt are without order, what happens if they depend on each other?
          // - Such accesses that do not appear outside must be filtered out
          // - Add another coordinate to give them an order
          if (facc.isRead()) {
            readAccesses.addMap_inplace(accessRel);
          }
          if (facc.isWrite()) {
            writeAccesses.addMap_inplace(accessRel);
          }
        }
      }

      // To find the data that needs to be written back after the scop has been executed, we add an artificial stmt that reads all the data after everything has been executed
      auto epilogueId = islctx->createId("epilogue");
      auto epilogueDomainSpace = islctx->createSetSpace(0,0).setSetTupleId(epilogueId);
      auto scatterRangeSpace = getScatteringSpace(scop);
      assert(scatterRangeSpace.isSetSpace());
      auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);

      auto allIterationDomains = domain(writeAccesses);


    // Find the data flows

isl::UnionMap mustFlow;
isl::UnionMap mayFlow;
isl::UnionMap mustNosrc;
isl::UnionMap mayNosrc;
isl::computeFlow(readAccesses.copy(), writeAccesses.copy(), isl::UnionMap(), schedule.copy(), &mayFlow, &mayFlow, &mustNosrc, &mayNosrc);




      auto flowDeps = getFlowDependences(Deps);
      flowDeps.foreachMap([&] (isl::Map map) -> bool {
        auto src = scop->getScopStmtBySpace(map.getDomainSpace().keep());
        auto dst = scop->getScopStmtBySpace(map.getRangeSpace().keep());

        return false;
        ;});

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
