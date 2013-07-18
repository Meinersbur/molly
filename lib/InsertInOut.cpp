#define DEBUG_TYPE "molly"
#include "InsertInOut.h"

#include <polly/ScopInfo.h>
#include <polly/ScopPass.h>
#include <polly/PollyContextPass.h>
#include "MollyContextPass.h"
#include "FieldDetection.h"
#include "ScopEditor.h"
#include "MollyFieldAccess.h"
#include "MollyUtils.h"
#include "ScopUtils.h"
#include "islpp/Set.h"
#include "islpp/Map.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/PwAff.h"
#include "islpp/Point.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include "islpp/Expr.h"
#include "islpp/Constraint.h"
#include <polly/LinkAllPasses.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
using namespace molly;
using namespace polly;
using isl::enwrap;


namespace molly {

#define InsertInOutPass InsertInOutPass LLVM_FINAL
  class InsertInOutPass : public ScopPass {
#undef InsertInOutPass
  private:
    bool changedIR;
    bool changedScop;

    isl::Ctx *islctx;
    FieldDetectionAnalysis *fields;

    void modifiedIR() {
      changedIR = true;
    }

    void modifiedScop() {
      changedScop = true;
    }

  public:
    static char ID;
    InsertInOutPass() : ScopPass(ID) {  }

    void getAnalysisUsage(AnalysisUsage &AU) const LLVM_OVERRIDE {
      //ScopPass::getAnalysisUsage(AU);

      AU.addRequired<polly::ScopInfo>();
      AU.addRequired<polly::PollyContextPass>();
      AU.addRequired<molly::MollyContextPass>(); 
      AU.addRequired<molly::FieldDetectionAnalysis>();

      AU.addPreserved<polly::PollyContextPass>();
      AU.addPreserved<molly::MollyContextPass>(); 
      AU.addPreserved<polly::ScopInfo>(); 
      AU.addPreserved<polly::ScopDetection>(); 
      //AU.addPreserved<polly::TempScopInfo>(); 
      AU.addPreserved<llvm::RegionInfo>(); 
      AU.addPreserved<llvm::LoopInfo>(); 
      AU.addPreserved<llvm::ScalarEvolution>(); 
      AU.addPreservedID(polly::IndependentBlocksID);
      AU.addPreserved<molly::FieldDetectionAnalysis>();
      AU.addPreserved<llvm::DominatorTree>(); 
      // Does NOT preserve polly::Dependences (but we could make it do so)
    }


    void processScop(Scop *scop) {
      auto domainId = islctx->createId("logue");
      auto domainSpace = islctx->createSetSpace(0, 0);
      domainSpace = setTupleId(domainSpace, isl_dim_set, domainId);

      auto prologueId = islctx->createId("prologue");
      auto prologueDomainSpace = setTupleId(domainSpace, isl_dim_set, prologueId);
      auto epilogueId = islctx->createId("epilogue");
      auto epilogueDomainSpace = setTupleId(domainSpace, isl_dim_set, epilogueId);

      auto scatterRangeSpace = getScatteringSpace(scop);
      auto scatterId = scatterRangeSpace.getTupleId(isl_dim_set);
      auto scatterDim = scatterRangeSpace.getSetDimCount();
      assert(scatterRangeSpace.isSetSpace());

      DenseMap<FieldVariable *, isl::Set> accessedFields;
      auto scatterRange = scatterRangeSpace.emptySet();
      auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();
      auto scatterSpace = isl::Space::createMapFromDomainAndRange(domainSpace, scatterRangeSpace);
      auto prologueScatterSpace = isl::Space::createMapFromDomainAndRange(prologueDomainSpace, scatterRangeSpace);
      auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);


      // Determine which fields are accessed
      for (auto it = scop->begin(), end = scop->end(); it!=end; ++it) {
        auto stmt = *it;

        for (auto itMemacc = stmt->memacc_begin(), endMemacc = stmt->memacc_end(); itMemacc!=endMemacc; ++itMemacc) {
          auto memacc = *itMemacc;
          auto facc = fields->getFieldAccess(memacc);
          if (facc.isNull()) 
            continue; // Not a field, cannot handle

          auto fvar = facc.getFieldVariable();
          auto indexsetSpace = facc.getIndexsetSpace();
          auto accessRegion = facc.getAccessedRegion();
          auto &accessSet = accessedFields[fvar];
          if (accessSet.isNull()) {
            accessSet = accessRegion;
          } else {
            accessSet.unite_inplace(accessRegion);
          }
        }

        auto domain = getIterationDomain(stmt);
        auto scatter = getScattering(stmt);
        auto stmtScatterRange = apply(domain, scatter);
        scatterRange = unite(scatterRange.move(), stmtScatterRange.move());
      }

      // Only interested in most significant dimension
      //scatterRange = isl::projectOut(scatterRange.move(), isl_dim_set, 1, scatterDim-1);
      auto min = scatterRange.dimMin(0) - 1;
      min.setTupleId_inplace(isl_dim_in, domainId);
      auto max = scatterRange.dimMax(0) + 1;
      max.setTupleId_inplace(isl_dim_in, domainId);

      //isl::BasicSet::createFromPoint( setCoordinate(domainSpace.createZeroPoint(), isl_dim_set, 0, 0));

      isl::Set domain = domainSpace.universeSet();
      isl::Set prologueDomain = prologueDomainSpace.universeSet();
      isl::Set epilogueDomain = epilogueDomainSpace.universeSet();
      auto region = &scop->getRegion();

      // isl_basic_map_set_tuple_id does not exist
      auto prologieMapToZero = prologueScatterSpace.createUniverseBasicMap();
      auto epilogieMapToZero = epilogueScatterSpace.createUniverseBasicMap();
      for (auto d = 1; d < nScatterRangeDims; d+=1) {
        prologieMapToZero.addConstraint_inplace(prologueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
        epilogieMapToZero.addConstraint_inplace(epilogueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
      }


      // Insert fake write access before scop
      auto prologueScatter = min.toMap();// scatterSpace.createMapFromAff(min);
      prologueScatter.addDims_inplace(isl_dim_out, nScatterRangeDims - 1);
      //prologueScatter.setTupleId(isl_dim_out, scatterRangeSpace.getTupleId(isl_dim_set));
      prologueScatter.setInTupleId_inplace(prologueId);
      prologueScatter.setOutTupleId_inplace(scatterId);
      prologueScatter = intersect(prologueScatter.move(), prologieMapToZero.move());

      prologueScatter.setTupleId(isl_dim_in, prologueId.copy());
      auto prologueStmt = new ScopStmt(scop,  nullptr/*Maybe need to create a dummy BB*/, "scop.prologue", region, ArrayRef<Loop*>(), prologueDomain.take(), prologueScatter.take()); 
      scop->addScopStmt(prologueStmt);


      // Insert fake read access after scop
      auto epilogueScatter = max.toMap(); //scatterSpace.createMapFromAff(max);
      epilogueScatter.addDims_inplace(isl_dim_out, nScatterRangeDims - 1);
      //epilogueScatter.setTupleId(isl_dim_out, scatterRangeSpace.getTupleId(isl_dim_set));
      epilogueScatter.setInTupleId_inplace(epilogueId);
      epilogueScatter.setOutTupleId_inplace(scatterId);
      epilogueScatter = intersect(epilogueScatter.move(), epilogieMapToZero.move());

      //epilogueScatter.setTupleId(isl_dim_in, epilogueId.copy());
      auto epilogueStmt = new ScopStmt(scop, nullptr/*Maybe need to create a dummy BB*/, "scop.epilogie", region, ArrayRef<Loop*>(), epilogueDomain.take(), epilogueScatter.take()); 
      scop->addScopStmt(epilogueStmt);

      // Insert dummy memory accesses for all fields
      for (auto it = accessedFields.begin(), end = accessedFields.end(); it!=end; ++it) {
        auto fvar = it->first;
        auto fty = fvar->getFieldType();
        auto region = it->second.move(); 

        auto accessSpace = isl::Space::createMapFromDomainAndRange(domainSpace, region.getSpace());
        isl::Map allacc /* { domain () -> shape } */ = islctx->createAlltoallMap(domain, region.move());

        auto proAcc = prologueStmt->addAccess(MemoryAccess::MUST_WRITE, fvar->getVariable(), allacc.setTupleId(isl_dim_in, prologueId).take(), nullptr);
        proAcc->setFieldVariable(fvar);
        auto epiAcc = epilogueStmt->addAccess(MemoryAccess::READ, fvar->getVariable(), allacc.setTupleId(isl_dim_in, epilogueId).take(), nullptr);
        epiAcc->setFieldVariable(fvar);
      }
    }


    bool runOnScop(Scop &S) LLVM_OVERRIDE {
      changedIR = false;
      changedScop = false;
      this->fields = &getAnalysis<FieldDetectionAnalysis>();
      auto pollyCtx = &getAnalysis<polly::PollyContextPass>();
      this->islctx = enwrap(pollyCtx->getIslCtx());

      processScop(&S);

      return changedIR;
    }
  }; // class InsertInOutPass
} // namespace molly


char InsertInOutPass::ID;
char &molly::InsertInOutPassID = InsertInOutPass::ID;
static RegisterPass<InsertInOutPass> ScopStmtSplitPassRegistration("molly-insertinout", "molly::InsertInOutPass");
polly::ScopPass *molly::creatInsertInOutPass() {
  return new InsertInOutPass();
}
