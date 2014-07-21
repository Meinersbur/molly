#define DEBUG_TYPE "molly"
#include "MollyScopProcessor.h"

#include "FieldLayout.h"
#include "MollyPassManager.h"
#include "MollyFieldAccess.h"
#include "MollyUtils.h"
#include "MollyFunctionProcessor.h"
#include "ClusterConfig.h"
#include "FieldType.h"
#include "ScopUtils.h"
#include "ScopEditor.h"
#include "MollyIntrinsics.h"
#include "LLVMfwd.h"
#include "CommunicationBuffer.h"
#include "FieldVariable.h"
#include "Codegen.h"
#include "MollyScopStmtProcessor.h"
#include "MollyRegionProcessor.h"
#include "RectangularMapping.h"
#include "LocalBuffer.h"

#include "islpp/Ctx.h"
#include "islpp/Map.h"
#include "islpp/Set.h"
#include "islpp/UnionMap.h"
#include "islpp/Point.h"
#include "islpp/DimRange.h"
#include "islpp/MultiAff.h"

#include <polly/ScopInfo.h>
#include <polly/ScopPass.h>
#include <polly/Accesses.h>
#include <polly/RegisterPasses.h>
#include <polly/LinkAllPasses.h>
#include <polly/CodeGen/CodeGeneration.h>

#include <llvm/Transforms/Vectorize.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/GlobalVariable.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace {
  bool isStmtLevel(unsigned i) { return i % 2 == 0; }
  bool isLoopLevel(unsigned i) { return i % 2 == 1; }


  /// Like relativeScatter, but reversed mustBeRelativeTo, such that it matches the order of a potential MultiAff
  /// Returns: { subdomain[domain] -> scattering[scatter] }
  isl::Map relativeScatter2(const isl::Map &mustBeRelativeTo /* { model[domain] -> subdomain[domain] } */, const isl::Map &modelScatter /* { model[domain] -> scattering[scatter] } */, int relative) {
    if (relative == 0)
      return modelScatter;

    auto nScatterDims = modelScatter.getOutDimCount();
    auto subdomain = mustBeRelativeTo.getRange();
    auto mustBeRelativeToScatter = mustBeRelativeTo.applyDomain(modelScatter).reverse(); // { subdomain[domain] -> model[scatter] }
    auto scatterSpace = modelScatter.getRangeSpace();
    auto universeDomain = mustBeRelativeTo.getDomain(); // { [domain] }
    auto reverseModelScatter = modelScatter.reverse().intersectRange(universeDomain); // { scattering[scatter] -> [domain] }

    auto isBefore = scatterSpace.lexLtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    auto isAfter = scatterSpace.lexGtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    //auto naturallyBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain <_scatter model }
    //auto naturallyAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain >_scatter model }


    isl::PwMultiAff extreme; // { subdomain[domain] -> model[scatter]  }
    isl::Map order; // { subdomain[scatter] -> model[scatter] }
    if (relative < 0) {
      extreme = mustBeRelativeToScatter.lexminPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexLtMap(); // { A[scatter] -> B[scatter] | A <_lex B  }
    } else {
      extreme = mustBeRelativeToScatter.lexmaxPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexGtMap(); // { A[scatter] -> B[scatter] | A >_lex B  }
    }

    auto lastDim = nScatterDims - 1;
    auto referenceScatter = extreme.setPwAff(lastDim, extreme.getPwAff(lastDim) + relative); // { subdomain[domain] -> model[scatter] }
    //auto referenceBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());
    //auto referenceAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());

#if 0
    // Try to find another scatter that has the same ordering as referenceScatter
    // This is to simplify output, should not change any semantic
    // This does not consider other dependencies, just self-dependencies
    for (auto i = nScatterDims-nScatterDims; i < nScatterDims-1; i+=2) {
      assert(isStmtLevel(i) && "We only spread stmt levels, not loop levels");

      auto relativeScatter = extreme.setPwAff(i, extreme.getPwAff(i) + relative); // { subdomain[domain] -> model[scatter] }
      auto relativeBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(relativeScatter.reverse());
      auto relativeAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(relativeScatter.reverse());

      if (relative < 0) {
        if (relativeBefore == referenceBefore)
          return relativeScatter;
      } else {
        if (relativeAfter == referenceAfter)
          return relativeScatter;
      }
    }
#endif

    // Not found, return the reference schedule
    return referenceScatter;
  }


  isl::MultiPwAff constantScatter(isl::MultiAff translate/* { result[domain] -> model[domain] } */, isl::MultiPwAff modelScatter /* { model[domain] -> scattering[scatter] } */, int relative) {
    auto result = modelScatter.pullback(translate);
    auto nScatterDims = result.getOutDimCount();
    //result.setPwAff_inplace(nScatterDims - 1, result.getPwAff(nScatterDims - 1) + relative);
    result[nScatterDims - 1] = result[nScatterDims - 1] + relative;
    return result;
  }


  /// Compute a scatter that is relative (before/after) all instances of a group
  /// mustBeRelativeTo
  ///   Defines the group; the domain identifies all elements for which we have to find a scatter, the representative element. It maps to the set of instances that must be scheduled before/after the group's representative element
  /// modelScatter
  ///   Scatter function for model elements
  /// relative 
  ///   +1 for schedule after the group; -1 to schedule before
  isl::Map relativeScatter(const isl::Map &mustBeRelativeTo/* { subdomain[domain] -> model[domain] } */, const isl::Map &modelScatter /* { model[domain] -> scattering[scatter] } */, int relative) {
    if (relative == 0)
      return modelScatter;
    //assert(mustBeRelativeTo.getRange() >= modelScatter.getDomain());
    auto nScatterDims = modelScatter.getOutDimCount();
    auto subdomain = mustBeRelativeTo.getDomain();
    auto mustBeRelativeToScatter = mustBeRelativeTo.applyRange(modelScatter); // { subdomain[domain] -> model[scatter] }
    auto scatterSpace = modelScatter.getRangeSpace();
    auto universeDomain = mustBeRelativeTo.getRange(); // { [domain] }
    auto reverseModelScatter = modelScatter.reverse().intersectRange(universeDomain); // { scattering[scatter] -> [domain] }

    auto isBefore = scatterSpace.lexLtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    auto isAfter = scatterSpace.lexGtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    //auto naturallyBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain <_scatter model }
    //auto naturallyAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain >_scatter model }


    isl::PwMultiAff extreme; // { subdomain[domain] -> model[scatter]  }
    isl::Map order; // { subdomain[scatter] -> model[scatter] }
    if (relative < 0) {
      extreme = mustBeRelativeToScatter.lexminPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexLtMap(); // { A[scatter] -> B[scatter] | A <_lex B  }
    } else {
      extreme = mustBeRelativeToScatter.lexmaxPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexGtMap(); // { A[scatter] -> B[scatter] | A >_lex B  }
    }

    auto lastDim = nScatterDims - 1;
    auto referenceScatter = extreme.setPwAff(lastDim, extreme.getPwAff(lastDim) + relative); // { subdomain[domain] -> model[scatter] }
    //auto referenceBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());
    //auto referenceAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());
    referenceScatter.simplify_inplace();
    //assert(referenceScatter.getDomain() >= subdomain);

#if 0
    // Try to find another scatter that has the same ordering as referenceScatter
    // This is to simplify output, should not change any semantic
    // This does not consider other dependencies, just self-dependencies
    for (auto i = nScatterDims-nScatterDims; i < nScatterDims-1; i+=2) {
      assert(isStmtLevel(i) && "We only spread stmt levels, not loop levels");

      auto relativeScatter = extreme.setPwAff(i, extreme.getPwAff(i) + relative); // { subdomain[domain] -> model[scatter] }
      auto relativeBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(relativeScatter.reverse());
      auto relativeAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(relativeScatter.reverse());

      if (relative < 0) {
        if (relativeBefore == referenceBefore)
          return relativeScatter;
      } else {
        if (relativeAfter == referenceAfter)
          return relativeScatter;
      }
    }
#endif

    // Not found, return the reference schedule
    return referenceScatter;
  }


  class MollyScopContextImpl : public MollyScopProcessor, private ScopPass {
  private:
    MollyPassManager *pm;
    Scop *scop;
    Function *func;
    isl::Ctx *islctx;

    llvm::ScalarEvolution *SE;

    bool changedScop;
    void modifiedScop() {
      changedScop = true;
    }

    void runPass(Pass *pass) {
      switch (pass->getPassKind()) {
      case PT_Region:
        pm->runRegionPass(static_cast<RegionPass*>(pass), &scop->getRegion());
        break;
      case PT_Function:
        pm->runFunctionPass(static_cast<FunctionPass*>(pass), func);
        break;
      case PT_Module:
        pm->runModulePass(static_cast<ModulePass*>(pass));
        break;
      default:
        llvm_unreachable("Unsupport pass type");
      }
    }

    MollyFieldAccess getFieldAccess(ScopStmt *stmt) {
      return pm->getFieldAccess(stmt);
    }

  public:
    static char ID;
    MollyScopContextImpl(MollyPassManager *pm, Scop *scop) : SE(nullptr), ScopPass(ID), pm(pm), scop(scop), changedScop(false), scevCodegen(*pm->findOrRunAnalysis<ScalarEvolution>(nullptr, &scop->getRegion()), "scopprocessor") {
      func = molly::getFunctionOf(scop);
      islctx = pm->getIslContext();

      //FIXME: Shouldn't the resolver assigned by the pass manager?
      this->setResolver(MollyRegionProcessor::createResolver(pm, &scop->getRegion()));
    }


    bool hasFieldAccess() {
      // TODO: Cache result
      for (auto stmt : *scop) {
        auto stmtCtx = getScopStmtContext(stmt);
        if (stmtCtx->isFieldAccess())
          return true;
      }
      return false;
    }


    MollyFunctionProcessor *getFunctionProcessor() {
      return pm->getFuncContext(func);
    }


    const SCEV *getClusterCoordinate(unsigned i) override {
      //TODO: Cache SCEV
      auto funcCtx = pm->getFuncContext(func);
      auto coordVal = funcCtx->getClusterCoordinate(i);
      auto se = pm->findOrRunAnalysis<ScalarEvolution>(func);
      auto coordSe = se->getSCEV(coordVal);
      scop->addParam(coordSe);

      return coordSe;
    }


    std::vector<const SCEV *> getClusterCoordinates() {
      auto nClusterDims = pm->getClusterConfig()->getClusterDims();
      std::vector<const SCEV *> result;
      result.reserve(nClusterDims);
      for (auto i = nClusterDims - nClusterDims; i < nClusterDims; i += 1) {
        result.push_back(getClusterCoordinate(i));
      }
      return result;
    }


  protected:
    template<typename Analysis>
    Analysis *findOrRunAnalysis() {
      return pm->findOrRunAnalysis<Analysis>(func, &scop->getRegion());
    }

    Region *getRegion() override {
      return &scop->getRegion();
    }

    const Region *getRegion() const {
      return &scop->getRegion();
    }

    llvm::Function *getParentFunction() override {
      return getFunctionOf(scop);
    }

  private:
    std::map<isl_id *, llvm::Value *> valueMap;
  public:
    std::map<isl_id *, llvm::Value *> *getValueMap() {
      if (!valueMap.empty())
        return &valueMap;

      SCEVExpander expander(*findOrRunAnalysis<ScalarEvolution>(), "molly");
      auto insertionPoint = getRegion()->getEnteringBlock()->getFirstInsertionPt();
      //expander.set

      auto context = enwrap(scop->getContext());
      auto nContextParams = context.getParamDimCount();
      for (auto i = nContextParams - nContextParams; i < nContextParams; i += 1) {
        auto id = context.getParamDimId(i);
        auto scev = id.getUser<const SCEV *>();
        auto ty = dyn_cast<IntegerType>(scev->getType());

        auto value = expander.expandCodeFor(scev, ty, insertionPoint);
        valueMap[id.keep()] = value;
      }

      return &valueMap;
    }


    MollyScopStmtProcessor *getScopStmtContext(ScopStmt *stmt) const {
      return pm->getScopStmtContext(stmt);
    }




#pragma region Scop CommGen
    static void spreadScatteringsByInsertingDims(Scop *scop) {
      auto nOrigScatterDims = scop->getScatterDim();
      auto origScatterSpace = enwrap(scop->getScatteringSpace());
      auto islctx = enwrap(scop->getIslCtx());
      auto scatterTuple = getScatterTuple(scop);

      auto nScatterDims = nOrigScatterDims * 2 + 1;
      auto scatterSpace = origScatterSpace.addDims(isl_dim_out, nScatterDims - nOrigScatterDims).setSetTupleId(scatterTuple);

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto scatter = enwrap(stmt->getScattering());
        auto nOldDimCount = scatter.getOutDimCount();

        scatter.insertDims_inplace(isl_dim_out, 0, 1);
        scatter.fix_inplace(isl_dim_out, 0, 0);
        for (auto i = nOldDimCount - nOldDimCount; i < nOldDimCount; i += 1) {
          scatter.insertDims_inplace(isl_dim_out, 2 + 2 * i, 1);
          scatter.fix_inplace(isl_dim_out, 2 + 2 * i, 0);
        }
        for (auto i = scatter.getOutDimCount(); i < nScatterDims; i += 1) {
          scatter.insertDims_inplace(isl_dim_out, i, 1);
          scatter.fix_inplace(isl_dim_out, i, 0);
        }
        //assert(scatter.matchesMapSpace(enwrap(stmt->getDomain()).getSpace(), scatterSpace));
        scatter.setOutTupleId_inplace(scatterTuple);
        stmt->setScattering(scatter.take());
      }
    }


    /// Make some space between ever statement such that other statements between them can be inserted
    /// Only multiplies even levels, as those represent the order of statements in a loop body out outside the loop
    /// If this is not the case, one could either multiply all the levels or use spreadScatteringsByInsertingDims
    /// But isl_access_info_compute_flow seems to assume just this scatter layout
    /// multiplier could be 2, but an insertion after on instruction and before the next would overlap
    /// multiplier could be 3
    /// but most commonly we use 4 such that we also can insert something between two statements that result in an even scatter value
    static void spreadScatteringByMultiplying(Scop *scop, int multiplier) {
      auto nScatterDims = scop->getScatterDim();
      auto scatterSpace = enwrap(scop->getScatteringSpace());
      auto islctx = enwrap(scop->getIslCtx());
      auto scatterTuple = scatterSpace.getOutTupleId();

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto scattering = getScattering(stmt);
        auto space = scattering.getSpace();
        auto nParam = space.getParamDimCount();
        auto nIn = space.getInDimCount();
        auto nOut = space.getOutDimCount();
        auto mapSpace = isl::Space::createMapFromDomainAndRange(space.range(), scatterSpace);
        auto map = mapSpace.universeBasicMap();

        for (auto i = 1u; i < nOut; i += 2) {
          map.equate_inplace(isl_dim_in, i, isl_dim_out, i);
        }
        for (auto i = nOut - nOut; i < nOut; i += 2) {
          auto c = mapSpace.createEqualityConstraint();
          c.setCoefficient_inplace(isl_dim_in, i, -multiplier);
          c.setCoefficient_inplace(isl_dim_out, i, 1);
          map.addConstraint_inplace(c);
        }
        for (auto i = nOut; i < nScatterDims; i += 1) {
          map.fix_inplace(isl_dim_out, i, 0);
        }

        auto newScattering = scattering.applyRange(map);
        stmt->setScattering(newScattering.take());
      }
    }

  private:
    DenseMap<const isl_id*, ScopStmt*> tupleToStmt;
    isl::Id epilogueId;  // deprecated
    isl::Set beforeScopScatterRange; // deprecated; use beforeScopScatter
    isl::Set afterBeforeScatterRange;  // deprecated


    MollyScopStmtProcessor *getScopStmtContext(const isl::Id &domainTupleId) {
      auto stmt = tupleToStmt[domainTupleId.keep()]; //TODO: id.getUser() contains this
      assert(stmt);
      return getScopStmtContext(stmt);
    }

    MollyScopStmtProcessor *getScopStmtContext(const isl::Space &space)  {
      return getScopStmtContext(space.getSetTupleId());
    }

    MollyScopStmtProcessor *getScopStmtContext(const isl::Set &set)  {
      return getScopStmtContext(set.getTupleId());
    }


    bool contains(const Instruction *instr) const {
      return getRegion()->contains(instr);
    }


    bool usedOutside(const Value *val) const {
      for (auto use = val->use_begin(), end = val->use_end(); use != end; ++use) {
        auto user = cast<Instruction>(*use);

        if (!this->contains(user))
          return true;
      }

      return false;
    }


    bool usedAfterwards(const Value *val) const {
      auto instr = dyn_cast<Instruction>(val);
      if (!instr)
        return true; // Constant can be used anywhere, including other functions
      assert(instr->getParent()->getParent() == func);

      auto region = getRegion();
      auto entry = region->getEntry();
      auto TD = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

      for (auto use = val->use_begin(), end = val->use_end(); use != end; ++use) {
        auto user = cast<Instruction>(use->getUser());
        auto userBB = user->getParent();

        if (this->contains(user))
          continue; // Uses inside are not afterwards uses

        assert(entry != userBB); // if those are equal, the user is inside the Region!
        if (!TD->dominates(userBB, entry))
          return true; // user not strictly before the region, use is possible
      }

      // FIXME: val could be used by an operation before or inside the SCoP (bitcast etc.) and then use afterwards
      return false;
    }


  public:
    void genCommunication() {
      auto funcName = func->getName();
      DEBUG(llvm::dbgs() << "GenCommunication in " << funcName <<  " on SCoP " << scop->getNameStr() << "\n");
      if (funcName == "test") {
        int a = 0;
      } else if (funcName == "HoppingMatrix" || funcName == "HoppingMatrix_noka" || funcName == "HoppingMatrix_kamul") {
        int b = 0;
      } else if (funcName == "Jacobi") {
        int c = 0;
      } else if (funcName == "reduce") {
        int d = 0;
      } else if (funcName == "init") {
        int e = 0;
      } else if (funcName == "gemm") {
        int f = 0;
      }

      auto clusterConf = pm->getClusterConfig();
      auto &llvmContext = func->getContext();
      auto module = func->getParent();
      auto scatterTuple = getScatterTuple(scop);
      auto clusterTuple = clusterConf->getClusterTuple();
      auto nodeSpace = clusterConf->getClusterSpace();
      auto nodes = clusterConf->getClusterShape();
      auto nClusterDims = nodeSpace.getSetDimCount();

      // Make some space between the stmts in which we can insert our communication //TOOD: adding another dimension would be better; one before (for prologue, epilogue), one at the end (positioning relative to statement)
      spreadScatteringByMultiplying(scop, 4);

      // Collect information
      // Like polly::Dependences::collectInfo, but finer granularity (MemoryAccess instead ScopStmt)
      //DenseMap<const isl_id*, polly::MemoryAccess*> tupleToAccess;
      tupleToStmt.clear();
      for (auto stmt : *scop) {
        auto domainTuple = getDomainTuple(stmt);
        assert(!tupleToStmt.count(domainTuple.keep()) && "tupleId must be unique");
        tupleToStmt[domainTuple.keep()] = stmt;
      }

      DenseMap<const isl_id*, FieldType*> tupleToFty; //TODO: This is not per scop, so should be moved to PassManager
      DenseMap<const isl_id*, FieldVariable*> tupleToFvar;

      auto paramSpace = isl::enwrap(scop->getParamSpace());
      auto bbposTuple = islctx->createId("bbpos");
      auto bbposSpace = islctx->createSetSpace(1).setSetTupleId(bbposTuple);
      auto bbPosIdentity = bbposSpace.selfMap().identityBasicMap(); // { bbpos[] -> bbpos[] }

      auto emptyMap = paramSpace.createEmptyUnionMap();
      auto emptySet = paramSpace.createEmptyUnionSet();
      auto readAccesses = emptyMap; /* { stmt[iteration] -> access[indexset] } */
      auto writeAccesses = emptyMap; /* { stmt[iteration] -> access[indexset] } */
      auto schedule = emptyMap; /* { stmt[iteration] -> scattering[scatter] } */
      auto nonfieldSchedule = emptyMap; /* { stmt[iteration],bbpos[int] -> scattering[scatter],bbpos[int] } */

      //auto fieldReadAccesses = emptyMap; // { stmt[domain] -> field[indexset] }
      //auto fieldWriteAccesses = emptyMap; // { stmt[domain] -> field[indexset] }
      auto nonfieldReadAccesses = emptyMap; // { stmt[domain],bbpos[int] -> variable[] }
      auto nonfieldWriteAccesses = emptyMap; // { stmt[domain],bbpos[int] -> variable[] }
      auto nonfieldUsedOutside = emptySet; // { variable[indexset] }

      auto readFieldDomains = emptySet; // { [domain] }
      auto writeFieldDomain = emptySet; // { [domain] }

      // Collect accesses
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto stmtCtx = getScopStmtContext(stmt);

        auto domainTuple = getDomainTuple(stmt);
        auto domain = getIterationDomain(stmt); /* { stmt[domain] } */
        assert(domain.getSpace().matchesSetSpace(domainTuple));

        auto scattering = getScattering(stmt); /* { stmt[domain] -> scattering[scatter] }  */
        assert(scattering.imageIsBounded());
        assert(scattering.getSpace().matchesMapSpace(domainTuple, scatterTuple));
        scattering.intersectDomain_inplace(domain);
        schedule.addMap_inplace(scattering);

        auto bbposScattering = product(scattering, bbPosIdentity);
        nonfieldSchedule.addMap_inplace(bbposScattering);

        // Non-field accesses
        // nonfield-reuses may occur within a stmt, making it a self-dependency; the problem is that a read access may occur before or after a write access in a scop, with different depedency outcomes
        // Therefore, we refine the schedule to also map the order withing the BasicBlock
        // another solution is to see whether the read dependency is visible from the outside, i.e. overwritten in the same bb before
        for (auto accIt = stmt->begin(), accEnd = stmt->end(); accIt != accEnd; accIt += 1) {
          auto memacc = *accIt;
          auto acc = Access::fromMemoryAccess(memacc);
          auto instr = acc.getInstruction();
          //auto facc = FieldAccess::fromAccessInstruction(instr);
          if (acc.isFieldAccess())
            continue; // Will be handled later

          //llvm::Value *ptr;
          auto bbpos = positionInBasicBlock(instr);
          auto bbposAff = scattering.getDomainSpace().createConstantAff(bbpos); // { scattering[scatter] -> bbpos[bbpos] }
          auto bbposScattering = product(scattering, bbPosIdentity); // { [domain],bbpos[pos] -> scattering[scatter],bbpos[pos] }
          nonfieldSchedule.addMap_inplace(bbposScattering);

          auto rel = enwrap(memacc->getAccessRelation()); // { [domain] -> [indexset] }
          auto bbposMap = rangeProduct(domain.getSpace().selfMap().identityMap(), domain.getSpace().createConstantAff(bbpos).toBasicMap().setOutTupleId(bbposTuple));  // { [domain] -> [domain],bbpos[pos] }
          auto bbposRel = rel.applyDomain(bbposMap); // { [domain],bbpos[pos] -> [indexset] }
          auto indexsetSpace = rel.getRangeSpace();
          auto indexsetTupleId = indexsetSpace.getSetTupleId(); // TODO: Check uniqueness

          auto base = acc.getTypedPtr();

          if (memacc->isRead()) {
            nonfieldReadAccesses.unite_inplace(bbposRel);

            //TODO: Really needed for reads?
            //if (usedOutside(base)) {
            //  nonfieldUsedOutside.unite_inplace(rel.range());
            //}
          }

          if (memacc->isWrite()) {
            nonfieldWriteAccesses.unite_inplace(bbposRel);

            if (usedAfterwards(base)) {
              //assert(usedAfterwards(base));
              nonfieldUsedOutside.unite_inplace(rel.range());
            }
          }

#if 0
          if (auto loadInstr = dyn_cast<LoadInst>(instr)) {
            ptr = loadInstr->getPointerOperand();
            nonfieldReadAccesses.unite_inplace(rel);
          } else if (auto storeInstr = dyn_cast<StoreInst>(instr)) {
            ptr = storeInstr->getPointerOperand();
            nonfieldWriteAccesses.unite_inplace(rel);
          } else if (auto intrCall = dyn_cast<MemTransferInst>(instr)) {
            llvm_unreachable("TODO");
          } else {
            llvm_unreachable("What other type of memory accesses are there?");
          }
#endif
        }

        // Field accesses
        auto facc = getFieldAccess(stmt);
        if (!facc.isValid())
          continue; // Does not contain a field access

        auto fvar = stmtCtx->getFieldVariable();
        auto accessTuple = fvar->getTupleId();
        auto accessSpace = fvar->getAccessSpace();
        assert(!tupleToFvar.count(accessTuple.keep()) || tupleToFvar[accessTuple.keep()] == fvar);
        tupleToFvar[accessTuple.keep()] = fvar;

        auto fty = stmtCtx->getFieldType();
        auto indexsetTuple = fty->getIndexsetTuple();
        assert(!tupleToFty.count(indexsetTuple.keep()) || tupleToFty[indexsetTuple.keep()] == fty);
        tupleToFty[indexsetTuple.keep()] = fty;

        auto accessRel = facc.getAccessRelation(); /*  { stmt[domain] -> fvar[indexset] } */
        assert(accessRel.getSpace().matchesMapSpace(domainTuple, accessSpace));
        accessRel.intersectDomain_inplace(domain);

        // auto scatter = stmtCtx->getScattering();
        // accessRel.applyDomain_inplace(scatter); // Accesses depend on order for flow computation, hence we must apply the scattering
        // assert(scatter.isInjective()); // The instance must still be uniquely identifyable; If this restriction must be lifted, isl::computeFlow must learn how to preserve the domain
        // The scattering also contains the information of order within a loop
        // accessRel.setInTupleId_inplace(domain.getTupleId()); // Still, we need to uniquely identify the statement it came from

        if (facc.isRead()) {
          readAccesses.addMap_inplace(accessRel);
          readFieldDomains.unite_inplace(accessRel.domain());
        }
        if (facc.isWrite()) {
          auto x = getFieldAccess(stmt);
          auto accessRel2 = facc.getAccessRelation();

          writeAccesses.addMap_inplace(accessRel);
          writeFieldDomain.unite_inplace(accessRel.domain());
        }
      }

      // To find the data that needs to be written back after the scop has been executed, we add an artificial stmt that reads all the data after everything has been executed
      epilogueId = islctx->createId("epilogue");
      auto epilogueDomainSpace = islctx->createSetSpace(0, 0).setSetTupleId(epilogueId);
      auto scatterRangeSpace = getScatteringSpace();
      assert(scatterRangeSpace.isSetSpace());
      auto scatterId = scatterRangeSpace.getSetTupleId();
      auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();
      auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);

      auto prologueId = islctx->createId("prologue");
      auto prologueDomainSpace = islctx->createSetSpace(0, 0).setSetTupleId(prologueId);
      auto prologueScatterSpace = isl::Space::createMapFromDomainAndRange(prologueDomainSpace, scatterRangeSpace);

      auto allScatters = range(schedule);
      //assert(allScatters.nSet()==1);
      auto scatterRange = allScatters.extractSet(scatterRangeSpace);
      assert(scatterRange.isValid());
      auto nScatterDims = scatterRange.getDimCount();

      assert(!scatterRange.isEmpty());
      auto max = alltoall(epilogueDomainSpace.universeBasicSet(), scatterRange).dimMax(0) + 1;
      //max.setInTupleId_inplace(epilogueId);
      this->afterScopScatter = epilogueScatterSpace.createZeroMultiAff();
      this->afterScopScatter[0] = max;

      isl::Set epilogueDomain = epilogueDomainSpace.universeSet();
      auto epilogieMapToZero = epilogueScatterSpace.createUniverseBasicMap();
      for (auto d = nScatterRangeDims - nScatterRangeDims + 1; d < nScatterRangeDims; d += 1) {
        epilogieMapToZero.addConstraint_inplace(epilogueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
      }

      assert(!scatterRange.isEmpty());
      auto min = alltoall(prologueDomainSpace.universeBasicSet(), scatterRange).dimMin(0) - 1; /* { [1] } */
      // min.setInTupleId_inplace(prologueId); /* { scattering[nScatterDims] -> [1] } */
      this->beforeScopScatter = prologueScatterSpace.createZeroMultiAff();
      this->beforeScopScatter[0] = min;

      //min.addDims_inplace(isl_dim_in, nScatterDims);
      //auto beforeScopScatter = scatterRangeSpace.mapsTo(scatterRangeSpace).createZeroMultiPwAff(); /* { scattering[nScatterDims] -> scattering[nScatterDims] } */
      //beforeScopScatter.setPwAff_inplace(0, min);
      beforeScopScatterRange = beforeScopScatter.toMap().getRange();


      auto afterBeforeScatter = beforeScopScatter.setPwAff(1, beforeScopScatter.getDomainSpace().createConstantAff(1));
      afterBeforeScatterRange = afterBeforeScatter.toMap().getRange();

      // Insert fake read access after scop
      auto epilogueScatter = max.toMap(); //scatterSpace.createMapFromAff(max);
      epilogueScatter.addDims_inplace(isl_dim_out, nScatterRangeDims - 1);
      //epilogueScatter.setTupleId(isl_dim_out, scatterRangeSpace.getTupleId(isl_dim_set));
      epilogueScatter.setInTupleId_inplace(epilogueId);
      epilogueScatter.setOutTupleId_inplace(scatterId);
      epilogueScatter.intersect_inplace(epilogieMapToZero);
      assert(!epilogueScatter.isEmpty());

      //auto epilogieScatterAsDomain = epilogueScatter.getRange().setTupleId(epilogueId);

      //auto allIterationDomains = domain(writeAccesses);
      auto allIndexsets = range(writeAccesses);
      //auto relateAll = isl::UnionMap::createFromDomainAndRange(
      auto epilogueTouchEverything = isl::UnionMap::createFromDomainAndRange(epilogueDomain, allIndexsets);

      // Find the data flows
      readAccesses.unite_inplace(epilogueTouchEverything);
      schedule.unite_inplace(epilogueScatter);
      nonfieldSchedule.unite_inplace(epilogueScatter);
      isl::UnionMap mustFlow;
      isl::UnionMap mayFlow;
      isl::UnionMap mustNosrc;
      isl::UnionMap mayNosrc;

#if 0
      isl::simpleFlow(readAccesses, writeAccesses, schedule, &mustFlow, &mustNosrc);
#else
      isl::computeFlow(readAccesses, writeAccesses, emptyMap, schedule, /*out*/mustFlow, /*out*/mayFlow, /*out*/mustNosrc, /*out*/mayNosrc);
      assert(mayNosrc.isEmpty());
      assert(mayFlow.isEmpty());
      //TODO: verify that computFlow generates direct dependences
#endif

#if 0
      isl::UnionMap mustFlow2;
      isl::UnionMap mustNosrc2;

      isl::simpleFlow(readAccesses, writeAccesses, schedule, /*out*/mustFlow2, /*out*/mustNosrc2);
      assert(mustFlow == mustFlow2);
      assert(mustNosrc == mustNosrc2);

      // isl::simpleFlow seems to be more correct
      // FIXME: Find bug in isl::computeFlow
      //mustFlow == mustFlow2;
      //mustNosrc == mustNosrc2;
#endif



      auto inputFlow = emptySet; //TODO: Make set
      auto dataFlow = emptyMap;
      auto outputFlow = emptySet;

      //inputFlow = mustNosrc;
      for (auto inpMap : mustNosrc.getMaps()) { /* mustNosrc: { stmtRead[domain] -> field[indexset] } */
        inputFlow.addSet_inplace(inpMap.domain().coalesce());
      }
      // TODO: The information which data is accessed is useful in case that a stmt accesses multiple elements; unfortunately ISL does not deliver this information
      for (auto dep : mustFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        auto readTuple = dep.getOutTupleId();
        if (readTuple == epilogueId) {
          auto out = dep.getDomain();
          outputFlow.addSet_inplace(out.coalesce());
        } else {
          dataFlow.addMap_inplace(dep.coalesce());
        }
      }
      isl::Approximation approx2;
      auto dataFlowClosure = dataFlow.transitiveClosure(approx2);
      //viewRelation(dataFlow);

#pragma region Flow for nonfield accesses
      //auto universeAccessEverything = emptyMap; // { epilogue[] -> [indexset] }
      //for (auto access : nonfieldWriteAccesses.getMaps()) {//TODO: Only add those that are read outside the SCoP
      //  auto indexsetSpace = access.getRangeSpace();
      //  auto universe = indexsetSpace.universeBasicSet();
      //  auto accessEverything = alltoall(epilogueDomain, universe); // { epilogue[] -> nonfield[indexset] }
      //  universeAccessEverything.unite_inplace(accessEverything);
      //}
      auto universeAccessEverything = alltoall(epilogueDomain, nonfieldUsedOutside);
      nonfieldReadAccesses.unite_inplace(universeAccessEverything);

      isl::UnionMap nonfieldMustFlow;
      isl::UnionMap nonfieldMayFlow;
      isl::UnionMap nonfieldMustNosrc;
      isl::UnionMap nonfieldMayNosrc;
      isl::computeFlow(nonfieldReadAccesses, nonfieldWriteAccesses, emptyMap, nonfieldSchedule, /*out*/nonfieldMustFlow, /*out*/nonfieldMayFlow, /*out*/nonfieldMustNosrc, /*out*/nonfieldMayNosrc);

      auto nonfieldInputFlow = emptySet; //nonfieldMustNosrc.domain();
      auto nonfieldDataFlow = emptyMap;
      auto nonfieldOutputFlow = emptySet;

      //nonfieldMustNosrc.foreachMap([&](isl::Map map) -> bool {
      for (auto map : nonfieldMustNosrc.getMaps()) {
        if (map.getInTupleIdOrNull() == epilogueId)
          continue;

        auto stmtbbposSpace = map.getDomainSpace();
        auto stmtSpace = stmtbbposSpace.unwrap().getDomainSpace();
        auto removeBbposMap = domainProduct(stmtSpace.identityToSelfBasicMap(), alltoall(bbposSpace, stmtSpace));
        nonfieldInputFlow.addSet_inplace(map.domain().apply(removeBbposMap));
      }
      // Split into flow and output dependencies, and remove the bbpos annotation
      isl::UnionMap nonfieldMustFlowBtwStmt;
      //nonfieldMustFlow.foreachMap([&](isl::Map map) -> bool {
      for (auto map : nonfieldMustFlow.getMaps()) {//TODO: Remove self-dependencies
        auto stmtbbposSpace = map.getDomainSpace();
        auto stmtSpace = stmtbbposSpace.unwrap().getDomainSpace();
        auto removeBbposMap = domainProduct(stmtSpace.identityToSelfBasicMap(), alltoall(bbposSpace, stmtSpace));
        if (map.getOutTupleIdOrNull() == epilogueId) {
          nonfieldOutputFlow.addSet_inplace(map.getDomain().apply(removeBbposMap));
        } else {
          auto dststmtbbposSpace = map.getRangeSpace();
          auto dststmtSpace = dststmtbbposSpace.unwrap().getDomainSpace();
          auto dstRemoveBbposMap = domainProduct(dststmtSpace.identityToSelfBasicMap(), alltoall(bbposSpace, dststmtSpace));
          auto stmtToStmt = map.applyDomain(removeBbposMap).applyRange(dstRemoveBbposMap);
          if (stmtToStmt.getDomainSpace() == stmtToStmt.getRangeSpace()) {
            stmtToStmt.subtract_inplace(stmtToStmt.getSpace().identityBasicMap()); // Remove self-dependences
          }
          nonfieldDataFlow.addMap_inplace(stmtToStmt);
        }
      }

#if 0
      for (auto dep : nonfieldMustFlowBtwStmt.getMaps()) {
        if (dep.getOutTupleId() == epilogueId) {
          nonfieldOutputFlow.addSet_inplace(dep.getDomain());
        } else {
          nonfieldDataFlow.addMap_inplace(dep);
        }
      }
#endif

      isl::Approximation approx;
      auto nonfieldDataFlowClosure = nonfieldDataFlow.transitiveClosure(approx); // { [domain] -> [domain] }
      assert(!possiblyFalseNegatives(approx));
      auto nonfieldDataFlowClosureRev = nonfieldDataFlowClosure.reverse();
      //viewRelation(nonfieldDataFlow);
#pragma endregion

      /////////////////////////////////////////////////
      // Decide where to execute the statements

      /// List of statement instances that are not yet executed on at least one node
      auto notyetExecuted = emptySet;
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmtCtx = getScopStmtContext(*itStmt);

        auto domain = stmtCtx->getDomain();
        stmtCtx->setWhere(alltoall(domain.getSpace().universeBasicSet(), nodeSpace.emptySet())); // Reset to execute nowhere
        notyetExecuted.unite_inplace(domain);
      }


      // Apply #pragma molly where metadata if it exists
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmtCtx = getScopStmtContext(*itStmt);

        auto bb = stmtCtx->getBasicBlock();
        StringRef islWhere;
        for (auto itInstr = bb->begin(), endInstr = bb->end(); itInstr != endInstr; ++itInstr) {
          auto instr = &*itInstr;
          auto where = itInstr->getMetadata("where");
          if (!where)
            continue;

          clang::CodeGen::InstructionWhereMetadata mdReader;
          mdReader.readMetadata(getModuleOf(bb), where);

          //FIXME: A lot can go wrong here
          // - islstr cannot be parsed
          // - result map is incompatible
          auto whereMap = islctx->readMap(mdReader.islstr);
          whereMap.setInTupleId_inplace(stmtCtx->getDomainSpace().getSetTupleId());
          whereMap.setOutTupleId_inplace(clusterTuple);
          stmtCtx->addWhere(whereMap);
          notyetExecuted.substract_inplace(whereMap.domain());
        }
      }

      // Stmts producing scalars that are used after the SCoP must be executed everywhere, since every node requires the up-to-date data; 
      //TODO: Broadcasting this value to the other nodes instead might be better, but what if it is a pointer? Might come for free with automatic promotion of arrays to fields
      for (auto outSet : nonfieldOutputFlow.getSets()) { // { [domain] }
        auto stmt = getScopStmtContext(outSet);

        stmt->addWhere(alltoall(outSet, nodes));
        notyetExecuted.substract_inplace(outSet);
      }
      localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);


      // "owner computes": execute statements that are valid after the exit of the SCoP at the value's home locations; but just fits at this moment
      for (auto output : outputFlow.getSets()) {
        //auto  outputnotyet =  intersect(output, notyetExecuted);
        auto stmtCtx = getScopStmtContext(output);
        auto accessed = stmtCtx->getAccessRelation().intersectDomain(output); // { writeStmt[domain] -> field[indexset] }
        auto accessedButNotyetExecuted = accessed.intersectDomain(notyetExecuted); // { stmt[domain] -> field[indexset] }
        auto fvar = stmtCtx->getFieldVariable();
        //auto flayout = fvar->getLayout();
        auto fieldHome = fvar->getPhysicalNode(); // { [domain] -> [cluster] }
        auto homeAcc = accessedButNotyetExecuted.applyRange(fieldHome); // { stmt[domain] -> [cluster] }
        stmtCtx->addWhere(homeAcc); // homeAcc is not a function; TODO: Execute on ALL these nodes?

        notyetExecuted.substract_inplace(homeAcc.domain());
        localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
      }

      //auto tmp = overviewWhere();

#if 0
      // "source computes": execute statements that read data from before the SCoP at the home location of that data
      for (auto input : inputFlow.getSets()) {
        auto stmtCtx = getScopStmtContext(input);
        auto accessed = stmtCtx->getAccessRelation();
        auto accessedButNotyetExecuted = accessed.intersectDomain(notyetExecuted);
        auto fvar = stmtCtx->getFieldVariable();
        //auto fieldHome = fvar->getHomeAff();
        auto fieldHome = fvar->getPrimaryPhysicalNode(); // fvar->getPhysicalNode();
        auto homeAcc = accessedButNotyetExecuted.applyRange(fieldHome); // { stmt[domain] -> [cluster] }
        stmtCtx->addWhere(homeAcc);

        notyetExecuted.substract_inplace(accessedButNotyetExecuted.domain());
        localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
      }
#endif

      //tmp = overviewWhere();

      while (true) {
        if (notyetExecuted.isEmpty()) break;
        bool outerChanged = false;

        while (true) {
          bool changed = false;
          // "Dependence computes": Execute statements that computes value preferably on the node where the value is needed again
          for (auto flow : dataFlow.getMaps()) {
            auto producerSpace = flow.getDomainSpace();
            auto consumerSpace = flow.getRangeSpace();

            auto producerStmt = getScopStmtContext(producerSpace);
            auto consumerStmt = getScopStmtContext(consumerSpace);

            auto flowNotyetExecuted = flow.intersectDomain(notyetExecuted); // { producer[domain] -> consumer[domain] }
            //auto flowConsumerWhere = flowNotyetExecuted.wrap().chainSubspace(consumerStmt->getWhere()); // { (producer[domain] -> consumer[domain]) -> consumer[cluster] }
            auto producerSuggestedWhere = flowNotyetExecuted.applyRange(consumerStmt->getWhere()); // { producer[domain] -> [cluster] }

            if (!producerSuggestedWhere.isEmpty()) {
              producerStmt->addWhere(producerSuggestedWhere);
              notyetExecuted.substract_inplace(producerSuggestedWhere.domain());
              localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
              changed = true;
              outerChanged = true;
            }
          }

          //tmp = overviewWhere();

          //TODO: This fixpoint computation should be done on a higher level; on unbounded domains, this method might not even finish
          // See UnionMap::transitiveClosure for how it should work
          if (!changed)
            break;
        }

        if (notyetExecuted.isEmpty())
          break;


        for (auto nonexec : notyetExecuted.getSets()) {
          auto nonexecSpace = nonexec.getSpace();
          auto nonexecStmt = getScopStmtContext(nonexecSpace);
          if (nonexecStmt->isFieldAccess())
            continue; // Field access does need to be executed for its own sake

          // What does it depend on?
          auto depends = nonfieldDataFlowClosure.intersectRange(nonexec); // { producer[domain] -> nonexec[domain] }
          for (auto dep : depends.getMaps()) {
            dep = dep.intersectRange(notyetExecuted); // Remove lately added instances
            auto producerSpace = dep.getDomainSpace();
            auto producerStmt = getScopStmtContext(producerSpace);

            auto producerWhere = producerStmt->getWhere();
            auto producerSuggestedWhere = dep.applyDomain(producerWhere).reverse(); // { nonexec[domain] -> rank[cluster] }
            if (!producerSuggestedWhere.isEmpty()) {
              auto producerSuggestedOneWhere = producerSuggestedWhere.anyElement(); // { nonexec[domain] -> rank[cluster] }
              nonexecStmt->addWhere(producerSuggestedOneWhere);
              notyetExecuted.substract_inplace(producerSuggestedOneWhere.domain());
              localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
              outerChanged = true;
            }
          }
          // Stmts that are still not executed anywhere, execute on the home locations of the accesses
          depends.intersectRange_inplace(notyetExecuted);
          for (auto dep : depends.getMaps()) {
            dep = dep.intersectRange(notyetExecuted);
            auto producerSpace = dep.getDomainSpace();
            auto producerStmt = getScopStmtContext(producerSpace);

            if (!producerStmt->isFieldAccess())
              continue;
            auto fvar = producerStmt->getFieldVariable();
            auto home = fvar->getPrimaryPhysicalNode(); // { producer[domain] -> rank[cluster] }
            auto suggested = home.applyDomain(producerStmt->getAccessed().reverse()).applyDomain(dep);
            if (!suggested.isEmpty()) {
              auto suggestOne = suggested.anyElement();
              nonexecStmt->addWhere(suggestOne);
              notyetExecuted.substract_inplace(suggestOne.domain());
              localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
              outerChanged = true;
            }
          }
        }

#if 0
        // Do the reverse; for SCoPStmts that call a function find at least one node where it is executed depending on its data input 
        // TOOD: What if sink does not read anything?
        for (auto flow : dataFlow.getMaps()) {
          auto producerSpace = flow.getDomainSpace();
          auto consumerSpace = flow.getRangeSpace();
          auto producerStmt = getScopStmtContext(producerSpace);
          auto consumerStmt = getScopStmtContext(consumerSpace);

          auto flowNotyetExecuted = flow.intersectRange(notyetExecuted); // { producer[domain] -> consumer[domain] }
          auto consumerSuggestedWhere = flowNotyetExecuted.applyDomain(producerStmt->getWhere()).reverse();  // { consumer[domain] -> [cluster] }

          if (!consumerSuggestedWhere.isEmpty()) {
            // Chose just one
            auto consumerSuggestedOneWhere = consumerSuggestedWhere.lexmin();
            consumerStmt->addWhere(consumerSuggestedOneWhere);
            notyetExecuted.substract_inplace(consumerSuggestedWhere.domain());
            localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
            changed = true;
          }
        }
#endif

        if (!outerChanged)
          break;

      }

      if (!notyetExecuted.isEmpty()) {
        // Usually, for every statement at least one node should be found where it is executed
        // Stmts without node do not produce any value used by others
        // This should only happen if a field element is overwritten by another value
        // In experiments, it also happens when some reg2mem-generated stores (jacobi.cpp) why?
        // TODO: Such nodes might be sinks calling some function (printf); add some mechanism that executes them somewhere
        int a = 0;
      }

#if 0
      for (auto it = scop->begin(), end = scop->end();it!=end;++it) {
        auto stmt = *it;
        auto stmtCtx = getScopStmtContext(stmt);
        auto where = stmtCtx->getWhere();
        where.coalesce_inplace();
        stmtCtx->setWhere(where);
      }
#endif

      /////////////////////////////////////////////////

      int inpCnt = 1;
      for (auto inp : inputFlow.getSets()) {
        // Value source is outside this scop 
        // Value must be read from home location
        DEBUG(llvm::dbgs() << "InputCommunication " << inpCnt << " of " << inputFlow.nSet() << ": "; inp.print(llvm::dbgs()); llvm::dbgs() << "\n");
        genInputCommunication(inp);
        inpCnt+=1;
      }

      int flowCnt = 1;
      for (auto dep : dataFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        DEBUG(llvm::dbgs() << "FlowCommunication " << flowCnt << " of " << dataFlow.getNumMaps() << ": "; dep.print(llvm::dbgs()); llvm::dbgs() << "\n");
        genFlowCommunication(dep);
        flowCnt+=1;
      }

      int outCnt = 1;
      for (auto out : outputFlow.getSets()) {
        DEBUG(llvm::dbgs() << "OutputCommunication " << outCnt << " of " << outputFlow.nSet() << ": "; out.print(llvm::dbgs()); llvm::dbgs() << "\n");
        // This means the data is visible outside the scop and must be written to its home location
        // There is no explicit read access
        genOutputCommunication(out);
        outCnt+=1;
      }


      // Disable all write accesses; all of them have been replaced by communication-writes (or even by multiple if there are multiple statement that read them)
      for (auto const & writeAcc : writeAccesses.getMaps()) {
        auto id = writeAcc.getInTupleId();
        if (id == prologueId)
          continue;
        auto writeStmt = getScopStmtContext(id);
        auto emptyWhere = writeStmt->getDomainSpace().mapsTo(nodeSpace).emptyMap();
        writeStmt->setWhere(emptyWhere);
      }

#ifndef NDEBUG
      for (const auto &readAcc : readAccesses.getMaps()) {
        auto id = readAcc.getInTupleId();
        if (id == epilogueId) 
          continue;
        auto readStmt = getScopStmtContext(id);
        auto where = readStmt->getWhere();
        assert(where.isEmpty()); // Any of those should have been replaced
      }
#endif
      //TODO: For every may-write, copy the original value into that the target memory (unless they are the same), so we copy the correct values into the buffers
      // i.e. treat ever may write as, write original value if not written
    }

    isl::UnionMap overviewWhere();


    /// Make the data that is required to compute a ScopStmt (also) execute the value-computing ScopStmt
    void localizeNonfieldFlowDeps(isl::UnionSet &notyetExecuted, const isl::UnionMap &nonfieldDataFlowClosure)    {
      // When a stmt reads a value from a nonfield, the producing stmt must also be executed on the same node; communication between nodes is possible with fields only
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
        auto stmtCtx = getScopStmtContext(*itStmt);
        auto where = stmtCtx->getWhere();// { [domain] -> [cluster] }
        auto whereTrans = nonfieldDataFlowClosure.applyRange(where); // Require all non-field value producers to execute on the same node

        for (auto prodWhere : whereTrans.getMaps()) { // { [domain] -> [cluster] }
          auto prodTuple = prodWhere.getInTupleId();
          //auto prodStmt = tupleToStmt[prodTuple.keep()];
          auto prodStmtCtx = getScopStmtContext(prodTuple);
          prodStmtCtx->addWhere(prodWhere);
          notyetExecuted.substract_inplace(prodWhere.getDomain());
        }
      }

      // By construction, addWhere adds redundant instances again and again; remove them here
      //for (auto map : nonfieldDataFlowClosure.getMaps()) {
     //   auto source = map.getInTupleId();
      //  auto stmtCtx = getScopStmtContext(source);
      //  stmtCtx->setWhere(stmtCtx->getWhere().removeRedundancies_consume().coalesce_consume());
      //}
    }


  private:
    isl::Space getStmtNodeSpace(MollyScopStmtProcessor *stmtCtx) {
      auto domainSpace = stmtCtx->getDomainSpace();
      auto domainTupleId = domainSpace.getSetTupleId();

      auto clusterSpace = getClusterConfig()->getClusterSpace();
      auto clusterTupleId = clusterSpace.getSetTupleId();

      auto nodeId = islctx->createId(domainTupleId.getName() + Twine("_") + clusterTupleId.getName(), domainTupleId.getUser());
      return clusterSpace.setSetTupleId(nodeId);
    }



  private:
    isl::MultiPwAff beforeScopScatter; // { prologue[] -> scattering[scatter] }
    isl::MultiPwAff afterScopScatter; // { epilogue[] -> scattering[scatter] }


    void genInputCommunication(isl::Set inp/* { readStmt[domain] } */) {
      auto scatterTupleId = getScatterTuple(scop);
      auto clusterConf = getClusterConfig();
      auto clusterShape = clusterConf->getClusterShape();
      auto clusterSpace = clusterConf->getClusterSpace();
      auto clusterTupleId = clusterShape.getTupleId();
      auto srcNodeId = islctx->createId("srcNode");
      auto srcNodeSpace = clusterSpace.setSetTupleId(srcNodeId);
      auto dstNodeId = islctx->createId("dstNode");
      auto dstNodeSpace = clusterSpace.setSetTupleId(dstNodeId);

      auto readStmt = getScopStmtContext(inp);
      assert(readStmt->isReadAccess());
      auto readDomain = readStmt->getDomain(); // { readStmt[domain] }
      auto readDomainSpace = readDomain.getSpace();
      auto readTupleId = readDomain.getTupleId();
      assert(readStmt->isFieldAccess());
      auto readFvar = readStmt->getFieldVariable();
      auto readAccRel = readStmt->getAccessRelation().intersectDomain(readDomain); // { readStmt[domain] -> fvar[index] }
      assert(readAccRel.matchesMapSpace(readDomain.getSpace(), readFvar->getAccessSpace()));
      auto readScatter = readStmt->getScattering(); //  { readStmt[domain] -> scattering[scatter] }
      assert(readScatter.matchesMapSpace(readDomain.getSpace(), scatterTupleId));
      auto readWhere = readStmt->getWhere().intersectDomain(readDomain).setOutTupleId(dstNodeId); // { stmtRead[domain] -> dstNode[cluster] }
      auto readEditor = readStmt->getEditor();
      auto readVal = readStmt->getLoadAccessor();
      auto readPtr = readStmt->getAccessPtr();
      auto readAccess = readStmt->getAccess();
      auto readAccessedAff = readStmt->getAccessed();

      auto fvar = readFvar;
      auto fty = fvar->getFieldType();
      //auto homeAff = fvar->getHomeAff().setOutTupleId(srcNodeId); // { fvar[indexset] -> srcNode[cluster] }
      auto homeAff = fvar->getPhysicalNode().setOutTupleId(srcNodeId); // { fvar[indexset] -> srcNode[cluster] }
      auto indexsetSpace = homeAff.getDomainSpace(); // { fvar[indexset] }

      auto transfer = readAccRel.intersectDomain(inp).chain(homeAff).wrap_consume().chainSubspace(readWhere).wrap_consume(); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
      auto local = intersect(transfer, transfer.getSpace().equalBasicSet(dstNodeSpace, srcNodeSpace)); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }

      auto locallyHandled = local.reorderSubspaces(dstNodeSpace >> indexsetSpace); // { dstNode[cluster], field[index] }
      //auto remoteTransfer = (transfer - local).coalesce(); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
      auto localTransfer = local.reorganizeSubspaces(indexsetSpace >> (readDomainSpace >> srcNodeSpace)).cast((indexsetSpace >> (readDomainSpace >> clusterSpace)).wrap_consume()).coalesce_consume(); // { fvar[indexset], readStmt[domain], [cluster] }

      auto remotes = transfer - locallyHandled.reorganizeSubspaces(transfer.getSpace()); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
      //remoteTransfer.coalesce_inplace();
      auto primaryHome = fvar->getPrimaryPhysicalNode(); // { fvar[domain] -> node[cluster] }
      auto remoteTransferSrc = remotes.reorderSubspaces(readDomainSpace >> indexsetSpace >> dstNodeSpace).chainSubspace(primaryHome.setOutTupleId(srcNodeId)); // { readStmt[domain], field[index], dstNode[cluster] -> srcNode[cluster] } 
      remoteTransferSrc.simplify_inplace();
      auto fvarname = fvar->getVariable()->getName();

      DEBUG(llvm::dbgs() << "Complexity local=" << (localTransfer.getBasicSetCount()) << " remote=" << (remoteTransferSrc.getComplexity()>>32) << "\n");

      ScopEditor editor(scop, asPass());

      if (!localTransfer.isEmpty()) { // TODO: For this part, we may just reuse the original ScopStmt
        auto readlocalWhere = localTransfer.reorderSubspaces(readDomainSpace, clusterSpace);
        auto readlocalScatter = readScatter;
        auto readlocalEditor = readEditor.createStmt(readlocalWhere.domain(), readlocalScatter, readlocalWhere, "readinput_local");
        readEditor.removeInstances(readlocalWhere);
        auto readlocalStmt = readlocalEditor.getStmt();
        auto readlocalStmtCtx = getScopStmtContext(readlocalStmt);

        auto readlocalCodegen = readlocalStmtCtx->makeCodegen();
        auto readlocalCurrent = readlocalEditor.getCurrentIteration(); // { readinput_local[domain] -> readStmt[domain] }
        auto readlocalCurrentDomain = readlocalCurrent.castRange(readDomainSpace);
        auto readlocalCurrentIndex = readlocalCurrentDomain.applyRange(readAccessedAff); // { readinput_local[domain] -> field[indexset] }
        auto readlocalCurrentNode = readlocalStmtCtx->getClusterMultiAff(); // { readlocalStmt[domain] -> rank[cluster] }
        //auto readlocalCurrentAccessed = readAccRel.castDomain(readlocalStmtCtx->getDomainSpace()); // { readlocalStmt[domain] -> field[indexset] }
        //auto readlocalCurrentAccessed = readAccessedAff.castDomain(readlocalStmtCtx->getDomainSpace()); // { readlocalStmt[domain] -> field[indexset] }

        //auto readStorage = readStmt->getAccessStackStoragePtr();
        //readlocalCodegen.codegenAssignScalarFromLocal(readStorage, fvar, readlocalCurrentNode, readlocalCurrentAccessed);
        //        auto readlocalVal = readlocalCodegen.codegenLoadLocal(fvar, readlocalCurrentNode, readlocalCurrentAccessed);
        //        readlocalCodegen.updateScalar(readPtr, readlocalVal);

        auto fieldPtr = readlocalCodegen.codegenFieldLocalPtr(fvar, readlocalCurrentNode, readlocalCurrentIndex);
        auto stackPtr = readStmt->getAccessStackStorageAnnPtr().pullbackDomain(readlocalCurrentDomain); //AnnotatedPtr::createScalarPtr(readStorage, readlocalWhere.getDomainSpace());
        readlocalCodegen.codegenAssign(stackPtr, fieldPtr);

        //auto marker = string() + "input local '" + readTupleId.getName() + "'";
        readlocalCodegen.markBlock((Twine("input local '") + readTupleId.getName() + "'" + fvarname).str());
      }


      if (!remoteTransferSrc.isEmpty()) {
        auto prologueDomain = beforeScopScatter.getDomain(); // { prologue[] }
        auto epilogueDomain = afterScopScatter.getDomainSpace().createUniverseBasicSet(); // { epilogue[] }

        //auto primaryHome = fvar->getPrimaryPhysicalNode(); // { fvar[domain] -> node[cluster] }
        //auto primaryHomeReorganzied = primaryHome.toMap().castRange(srcNodeSpace).wrap().reorganizeSubspaces(remoteTransfer.getSpace());
        //auto uniqueRemoteTransfer = remoteTransfer.intersect(primaryHomeReorganzied); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
        //uniqueRemoteTransfer.dumpExplicit(6*3*3*3,true,true);
        auto uniqueRemoteTransfer = remoteTransferSrc.wrap();  // { readStmt[domain], field[index], dstNode[cluster], srcNode[cluster] } 

        auto comrel = alltoall(prologueDomain, uniqueRemoteTransfer.reorderSubspaces((srcNodeSpace >> dstNodeSpace) >> indexsetSpace));  // { prologue[] -> (src[cluster], dst[cluster], fvar[indexset]) }
        auto combuf = pm->newCommunicationBuffer(fty, comrel);
        combuf->doLayoutMapping();


        // { dst[cluster] }: send_wait
        {
          auto sendwaitWhere = uniqueRemoteTransfer.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { src[cluster] -> node[cluster] }
          auto sendwaitScatter = constantScatter(dstNodeSpace.mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -3); // { srcNode[cluster] -> scattering[scatter] }
          auto sendwaitEditor = editor.createStmt(sendwaitWhere.domain(), sendwaitScatter, sendwaitWhere, "sendwait");
          auto sendwaitStmt = sendwaitEditor.getStmt();
          auto sendwaitStmtCtx = getScopStmtContext(sendwaitStmt);

          auto sendwaitCodegen = sendwaitStmtCtx->makeCodegen();
          auto sendwaitChunk = sendwaitStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { sendwaitStmt[domain] -> prologue[] }
          auto sendwaitSrc = sendwaitStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { sendwaitStmt[domain] -> src[cluster] }
          auto sendwaitDst = sendwaitEditor.getCurrentIteration().castRange(dstNodeSpace); // { sendwaitStmt[domain] -> dst[cluster] }

          //combuf->codegenSendWait(sendwaitCodegen, sendwaitChunk, sendwaitSrc, sendwaitDst);

          auto sendbufPtrPtr = combuf->codegenSendbufPtrPtr(sendwaitCodegen, sendwaitChunk, sendwaitSrc, sendwaitDst);
          auto bufptr = combuf->codegenSendWait(sendwaitCodegen, sendwaitChunk, sendwaitSrc, sendwaitDst);
          sendwaitCodegen.getIRBuilder().CreateStore(bufptr, sendbufPtrPtr);

          sendwaitCodegen.markBlock((Twine("input remote send_wait '") + readTupleId.getName() + "'" + fvarname).str());
        }

        // { dst[cluster], readStmt[domain] }: get_input
        {
          auto getinputDomain = uniqueRemoteTransfer.reorderSubspaces(dstNodeSpace >> readDomainSpace);
          auto getinputWhere = uniqueRemoteTransfer.reorderSubspaces(dstNodeSpace >> readDomainSpace, srcNodeSpace).castRange(clusterSpace); // { (dst[cluster], readStmt[domain]) -> src[cluster] }
          auto getinputScatter = constantScatter(getinputWhere.getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -2); // { (dst[cluster], readStmt[domain]) -> scattering[scatter] }
          auto getinputEditor = editor.createStmt(getinputWhere.domain(), getinputScatter, getinputWhere, "getinput");
          auto getinputStmt = getinputEditor.getStmt();
          auto getinputStmtCtx = getScopStmtContext(getinputStmt);

          auto getinputCodegen = getinputStmtCtx->makeCodegen();
          auto getInputChunk = getinputStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { getinputStmt[domain] -> prologue[] }
          auto getinputSrc = getinputStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { getinputStmt[domain] -> src[cluster] }
          auto getinputReadStmt = getinputEditor.getCurrentIteration().sublist(readDomainSpace); // { getinputStmt[domain] -> readStmt[domain] }
          auto getinputIndex = getinputReadStmt.applyRange(readAccessedAff);  // { getinputStmt[domain] -> fvar[indexset] }
          auto getinputDst = getinputEditor.getCurrentIteration().sublist(dstNodeSpace);  // { getinputStmt[domain] -> dst[cluster] }

          //auto getinputVal = getinputCodegen.codegenLoadLocal(fvar, getinputSrc.castRange(clusterSpace), getinputIndex);
          //auto getinputBufptr = combuf->codegenPtrToSendBuf(getinputCodegen, getInputChunk, getinputSrc, getinputDst, getinputIndex);
          //getinputCodegen.createArrayStore(getinputVal, )
          //getinputCodegen.getIRBuilder().CreateStore(getinputVal, getinputBufptr);
          //combuf->codegenStoreInSendbuf(getinputCodegen, getInputChunk, getinputSrc, getinputDst, getinputIndex, getinputVal);

          auto fieldPtr = getinputCodegen.codegenFieldLocalPtr(fvar, getinputSrc.castRange(clusterSpace), getinputIndex);
          auto bufPtr = combuf->codegenSendbufPtr(getinputCodegen, getInputChunk, getinputSrc, getinputDst, getinputIndex);
          getinputCodegen.codegenAssign(bufPtr, fieldPtr);

          getinputCodegen.markBlock((Twine("input remote get_input '") + readTupleId.getName() + "'" + fvarname).str());
        }


        // { dst[cluster] }: send
        {
          auto sendWhere = uniqueRemoteTransfer.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto sendScatter = constantScatter(dstNodeSpace.mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -1); // { srcNode[cluster] -> scattering[scatter] }
          auto sendEditor = editor.createStmt(sendWhere.domain(), sendScatter, sendWhere, "send");
          auto sendStmt = sendEditor.getStmt();
          auto sendStmtCtx = getScopStmtContext(sendStmt);

          auto sendCodegen = sendStmtCtx->makeCodegen();
          auto sendChunk = sendStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { sendwaitStmt[domain] -> prologue[] }
          auto sendSrc = sendStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { sendwaitStmt[domain] -> src[cluster] }
          auto sendDst = sendEditor.getCurrentIteration().castRange(dstNodeSpace); // { sendwaitStmt[domain] -> dst[cluster] }

          combuf->codegenSend(sendCodegen, sendChunk, sendSrc, sendDst);

          sendCodegen.markBlock((Twine("input remote send '") + readTupleId.getName() + "'" + fvarname).str());
        }


        // { src[cluster] } recv_wait
        {
          auto recvwaitWhere = uniqueRemoteTransfer.reorderSubspaces(srcNodeSpace, dstNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto recvwaitScatter = relativeScatter2(uniqueRemoteTransfer.reorderSubspaces(readDomainSpace, srcNodeSpace) /* { readStmt[domain] -> src[cluster] } */, readScatter, -1); // { src[cluster] -> scattering[] }
          auto recvwaitEditor = editor.createStmt(recvwaitWhere.domain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recvwait");
          auto recvwaitStmt = recvwaitEditor.getStmt();
          auto recvwaitStmtCtx = getScopStmtContext(recvwaitStmt);

          auto recvwaitCodegen = recvwaitStmtCtx->makeCodegen();
          auto recvwaitChunk = recvwaitStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { recvwaitStmt[domain] -> prologue[] }
          auto recvwaitSrc = recvwaitEditor.getCurrentIteration().castRange(srcNodeSpace); // { recvwaitStmt[domain] -> src[cluster] }
          auto recvwaitDst = recvwaitStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { recvwaitStmt[domain] -> dst[cluster] }

          //combuf->codegenRecvWait(recvwaitCodegen, recvwaitChunk, recvwaitSrc, recvwaitDst);

          auto bufPtr = combuf->codegenRecvWait(recvwaitCodegen, recvwaitChunk, recvwaitSrc, recvwaitDst);
          auto sendbufPtrPtr = combuf->codegenRecvbufPtrPtr(recvwaitCodegen, recvwaitChunk, recvwaitSrc, recvwaitDst);
          recvwaitCodegen.getIRBuilder().CreateStore(bufPtr, sendbufPtrPtr);

          recvwaitCodegen.markBlock((Twine("input remote recv_wait '") + readTupleId.getName() + "'" + fvarname).str());
        }


        // { readStmt[domain] }: readflow
        {
          auto readflowWhere = uniqueRemoteTransfer.reorderSubspaces(readDomainSpace, dstNodeSpace).castRange(clusterSpace); // { readStmt[domain] -> dst[cluster] }
          auto readflowScatter = readScatter; // { readStmt[domain] -> scattering[] }
          auto readflowEditor = readEditor.createStmt(readflowWhere.domain(), readflowScatter, readflowWhere, "readflow");
          readEditor.removeInstances(readflowWhere.castRange(clusterSpace));
          auto readflowStmt = readflowEditor.getStmt();
          auto readflowStmtCtx = getScopStmtContext(readflowStmt);

          auto readflowCodegen = readflowStmtCtx->makeCodegen();
          auto readflowChunk = readflowStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { readflowStmt[domain] -> prologue[] }
          auto readflowDst = readflowStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { readflowStmt[domain] -> dst[cluster] }
          auto readflowOrig = readflowEditor.getCurrentIteration().castRange(readDomainSpace);  // { readflowStmt[domain] -> readStmt[domain] }
          auto readflowIndex = readflowOrig.applyRange(readAccessedAff); // { readflowStmt[domain] -> fvar[indexset] }
          auto readflowSrc = readflowIndex.applyRange(primaryHome).castRange(srcNodeSpace); // { readflowStmt[domain] -> src[indexset] }

          //auto readflowBufptr = combuf->codegenPtrToRecvBuf(readflowCodegen, readflowChunk, readflowSrc, readflowDst, readflowIndex);
          //auto readflowVal = readflowCodegen.getIRBuilder().CreateLoad(readflowBufptr, "readflowval");
          //auto readflowVal = combuf->codegenLoadFromRecvBuf(readflowCodegen, readflowChunk, readflowSrc, readflowDst, readflowIndex);
          //readflowCodegen.updateScalar(readPtr, readflowVal);

          auto bufPtr = combuf->codegenRecvbufPtr(readflowCodegen, readflowChunk, readflowSrc, readflowDst, readflowIndex);
          auto stackPtr = AnnotatedPtr::createScalarPtr(readStmt->getAccessStackStoragePtr(), readflowWhere.getDomainSpace());
          readflowCodegen.codegenAssign(stackPtr, bufPtr);

          readflowCodegen.markBlock((Twine("input remote readflow '") + readTupleId.getName() + "'" + fvarname).str());
        }


        // { src[cluster] } recv
        {
          auto recvWhere = uniqueRemoteTransfer.reorderSubspaces(srcNodeSpace, dstNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto recvScatter = relativeScatter2(uniqueRemoteTransfer.reorderSubspaces(readDomainSpace, srcNodeSpace) /* { readStmt[domain] -> src[cluster] } */, readScatter, +1); // { src[cluster] -> scattering[] }
          auto recvEditor = editor.createStmt(recvWhere.domain(), recvScatter.copy(), recvWhere.copy(), "recv");
          auto recvStmt = recvEditor.getStmt();
          auto recvStmtCtx = getScopStmtContext(recvStmt);

          auto recvCodegen = recvStmtCtx->makeCodegen();
          auto recvChunk = recvStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { recvwaitStmt[domain] -> prologue[] }
          auto recvSrc = recvEditor.getCurrentIteration().castRange(srcNodeSpace); // { recvwaitStmt[domain] -> src[cluster] }
          auto recvDst = recvStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { recvwaitStmt[domain] -> dst[cluster] }

          combuf->codegenRecv(recvCodegen, recvChunk, recvSrc, recvDst);

          recvCodegen.markBlock((Twine("input remote recv '") + readTupleId.getName() + "'" + fvarname).str());
        }
      }
    } // genInputCommunication



  private:
    /// return the first dimension from which on all dependencies are always lexicographically positive
    /// examples:
    /// { (1,0,0) -> (0,1,0) } returns ?
    /// { (0,i,0) -> (0,i+1,-1) } returns ?
    /// { (0,0) -> (0,0) } returns ?
    unsigned computeLevelOfDependence(isl::Map depScatter) {  /* dep: { scattering[scatter] -> scattering[scatter] } */
      auto depMap = depScatter.getSpace();
      auto dependsVector = depMap.emptyMap(); /* { read[scatter] -> (read[scatter] - write[scatter]) } */
      auto nParam = depScatter.getParamDimCount();
      auto nIn = depScatter.getInDimCount();
      auto nOut = depScatter.getOutDimCount();
      assert(nIn == nOut);
      auto scatterSpace = depScatter.getDomainSpace(); /* { scattering[scatter] } */
      auto nCompareDims = std::min(nIn, nOut);

      // get the first always-positive coordinate such that the dependence is satisfied
      // Everything after that doesn't matter anymore to meet that dependence
      for (auto i = nCompareDims - nCompareDims; i < nCompareDims; i += 1) {
        auto orderMap = depMap.universeBasicMap().orderLt(isl_dim_in, i, isl_dim_out, i);
        // auto fulfilledDeps = depScatter.intersect(orderMap);
        if (orderMap >= depScatter) {
          // All dependences are fulfilled from here
          // i.e. we can shuffle stuff as we want in lower dimensions
          return i + 1;
        }

        // Remaining deps to be fulfilled
        depScatter -= orderMap;
      }

      // No always-positive dependence, therefore return the first out-of-range
      return std::max(nIn, nOut) + 1;
    }


    ClusterConfig *getClusterConfig() {
      return pm->getClusterConfig();
    }


    isl::Space getScatteringSpace() {
      return enwrap(scop->getScatteringSpace());
    }


    isl::Id getScatterDimId(unsigned i) {
      return islctx->createId("scatter" + Twine(i));
    }


    /// Create chunks: map the first (or last) element of a chunk to the elements of the chunk
    /// returns: { root[domain] -> [domain] }
    isl::Map chunkDomain(const isl::Map &scatter, unsigned levelOfDep, bool last) { // scatter: { [domain] -> scattering[scatter] }
      auto levelOfIndep = scatter.getOutDimCount() - levelOfDep;
      auto nDomainDims = scatter.getInDimCount();
      auto scatterSpace = scatter.getRangeSpace();

      auto depSubscatter = scatter.projectOut(isl_dim_out, levelOfDep, levelOfIndep);  // { [domain] -> dep[scatter] }
      auto domainBySubscatter = depSubscatter.reverse(); // { dep[scatter] -> [domain] }

      auto representiveElts = domainBySubscatter.lexoptPwMultiAff(last); // { dep[scatter] -> root[domain] }
      auto rootElts = representiveElts.toMap().getRange();
      auto result = representiveElts.toMap().applyDomain(domainBySubscatter).reverse(); // { root[domain] -> [domain] }
      return result;
    }


    /// returs: { [domain] -> root[domain] } 
    isl::PwMultiAff chunkDomainPwAff(const isl::PwMultiAff &scatter, unsigned levelOfDep, bool last) { // scatter: { [domain] -> scattering[scatter] }
      auto levelOfIndep = scatter.getOutDimCount() - levelOfDep;
      auto nDomainDims = scatter.getInDimCount();
      auto scatterSpace = scatter.getRangeSpace();

      auto depSubscatter = scatter.sublist(0, levelOfDep); // { [domain] -> dep[scatter] }
      auto domainBySubscatter = depSubscatter.reverse(); // { dep[scatter] -> [domain] }
      auto representiveElts = domainBySubscatter.lexoptPwMultiAff(last); // { dep[scatter] -> root[domain] }
      auto result = depSubscatter.applyRange(representiveElts); // { [domain] -> root[domain] } 
      return result; // NRVO
    }


    isl::Map partitionDomain(const isl::Map &scatter, unsigned levelOfDep) {
      auto levelOfIndep = scatter.getOutDimCount() - levelOfDep;
      auto nDomainDims = scatter.getInDimCount();
      auto scatterSpace = scatter.getRangeSpace();

      //scatter; // { [domain] -> (dep[scatter], indep[scatter]) }

      auto mapSpace = scatter.getParamsSpace().createMapSpace(levelOfDep + levelOfIndep, levelOfDep); // { scattering[scatter] -> dep[scatter] }
      auto map = mapSpace.equalBasicMap(isl_dim_in, 0, levelOfDep, isl_dim_out, 0);



      auto depSubscatter = scatter.projectOut(isl_dim_out, levelOfDep, levelOfIndep); // { [domain] -> dep[scatter] }
      auto indepSubscatter = scatter.projectOut(isl_dim_out, 0, levelOfDep); // { [domain] -> indep[scatter] }
      auto subscatter = scatter.moveDims(isl_dim_in, nDomainDims, isl_dim_out, 0, levelOfDep);  // { ([domain], dep[scatter]) -> indep[scatter] }
      auto domainBySubscatter = depSubscatter.reverse(); // { dep[scatter] -> [domain] }

      // partition depSubscatter by those domain elements that map to the same elements
      auto representiveElts = domainBySubscatter.lexmaxPwMultiAff(); // { dep[scatter] -> root[domain] }

      auto result = representiveElts.toMap().applyDomain(domainBySubscatter); // { [domain] -> root[domain] }
      return result;

      //return representiveElts.toMap().applyDomain(map.reverse().applyRange(scatter)); // { dep[scatter] -> [domain] }

      // Find a representive elt for every part
      auto canonicalElt = indepSubscatter.reverse().lexmaxPwMultiAff(); // { dep[scatter] -> [domain] }
      auto canonicalSubdomain = canonicalElt.toMap().getRange(); // { dep[domain] }
      auto canonicalMap = canonicalElt.toMap().reverse();


      return canonicalMap; // { dep[domain] -> indep[domain] }
    }


    static isl::Set simplify(isl::Set set) {
      auto result = set.move();
      //result.makeDisjoint_inplace();
      //result.detectEqualities_inplace();
      //result.removeRedundancies_inplace();
      result.coalesce_inplace();
      return result;
    }


    static isl::Map simplify(isl::Map map) {
      auto result = map.move();
      //result.makeDisjoint_inplace();
      //result.detectEqualities_inplace();
      //result.removeRedundancies_inplace();
      result.coalesce_inplace();
      return result;
    }


    void genFlowCommunication(const isl::Map &dep) { /* dep: { writeStmt[domain] -> readStmt[domain] } */
      auto scatterTupleId = getScatterTuple(scop);
      auto clusterSpace = getClusterConfig()->getClusterSpace();
      auto clusterShape = getClusterConfig()->getClusterShape();
      auto clusterTupleId = clusterShape.getTupleId();


      auto readStmt = getScopStmtContext(dep.getOutTupleId());
      assert(readStmt->isReadAccess());
      auto readDomain = readStmt->getDomain();
      auto readTupleId = readDomain.getTupleId();
      assert(readStmt->isFieldAccess());
      auto readFvar = readStmt->getFieldVariable();
      auto readAccRel = readStmt->getAccessRelation().intersectDomain(readDomain); /* { readStmt[domain] -> field[index] } */
      assert(readAccRel.matchesMapSpace(readDomain.getSpace(), readFvar->getAccessSpace()));
      auto readScatter = readStmt->getScattering().intersectDomain(readDomain); //  { readStmt[domain] -> scattering[scatter] }
      assert(readScatter.matchesMapSpace(readDomain.getSpace(), scatterTupleId));
      auto readWhere = readStmt->getWhere().intersectDomain(readDomain); // { stmtRead[domain] -> node[cluster] }
      assert(!readWhere.isEmpty());
      auto readEditor = readStmt->getEditor();
      auto readFvarname = readFvar->getVariable()->getName();

      auto writeStmt = getScopStmtContext(dep.getInTupleId());
      assert(writeStmt->isWriteAccess());
      auto writeDomain = writeStmt->getDomain();
      auto writeTupleId = writeDomain.getTupleId();
      assert(writeStmt->isFieldAccess());
      auto writeFVar = writeStmt->getFieldVariable();
      auto writeAccRel = writeStmt->getAccessRelation().intersectDomain(writeDomain); /* { writeStmt[domain] -> field[index] } */
      assert(writeAccRel.matchesMapSpace(writeDomain.getSpace(), writeFVar->getAccessSpace()));
      auto writeScatter = writeStmt->getScattering().intersectDomain(writeDomain); // { writeStmt[domain] -> scattering[scatter] }
      assert(writeScatter.matchesMapSpace(writeDomain.getSpace(), scatterTupleId));
      auto writeWhere = writeStmt->getWhere().intersectDomain(writeDomain); // { writeRead[domain] -> node[cluster] }
      assert(!writeWhere.isEmpty());
      auto writeEditor = writeStmt->getEditor();

      // A statement is either reading or writing, but not both
      assert(readStmt != writeStmt);

      // flow is data transfer from a writing statement to a reading statement of the same location, i.e. also same field
      assert(readFvar == writeFVar);
      auto fvar = readFvar;
      auto fty = fvar->getFieldType();
      auto indexsetSpace = fvar->getAccessSpace();

      // Actually, where the home of an index in that field is is irrelevant; we do not buffer the data in there but directly to the communication buffer
      //auto fieldDistribution = fty->getDistributionMapping(); // { field[index] -> node[cluster] }

      // Scatter spaces must be the same
      auto scatterSpace = readScatter.getRangeSpace();
      assert(writeScatter.getRangeSpace() == scatterSpace);

      // Which elements are transfered?
      auto transferedMap = dep.applyDomain(writeAccRel).applyRange(readAccRel);
      // Logically, this should be the identity map, but the requirement is only that at least the data that is read be existing in the communication buffer
      assert(transferedMap <= transferedMap.getSpace().identityMap());

      // Find where the communication can be inserted without violating any dependencies
      auto scatterDep = dep.applyDomain(writeScatter); /* { scatteringWrite[scatter] -> scatteringRead[scatter] } */
      scatterDep.applyRange_inplace(readScatter);
      auto levelOfDep = computeLevelOfDependence(scatterDep); //TODO: Also check for tansitive dependency violation; e.g. within a chunk one statement requires data from an instance executed before
      auto levelOfIndep = scatterSpace.getSetDimCount() - levelOfDep;

      // partition into subdomains such that all elements of a subdomain can be arbitrarily ordered
      // TODO: It would be better to derive the partitioning from dep (the data flow), so we derive a send and recv matching
      auto recvId = islctx->createId("recv");
      auto readPartitioning = partitionDomain(readScatter, levelOfDep).setOutTupleId(recvId); // { readStmt[domain] -> recv[domain] }
      auto sendId = islctx->createId("send");
      //auto writePartitioning = partitionDomain(writeScatter, levelOfDep).setOutTupleId(sendId); // { writeStmt[domain] -> send[domain] }

      auto depPartitioning = readPartitioning.reverse().chainNested(isl_dim_out, dep.reverse()); // { recv[domain] -> (readStmt[domain], writeStmt[domain]) }
      auto readChunks_ = chunkDomain(readScatter, levelOfDep, false).setInTupleId(recvId); // { recv[domain] -> readStmt[domain] }
      auto writeChunks_ = chunkDomain(writeScatter, levelOfDep, true); // { lastWrite[domain] -> writeStmt[domain] }
      auto readChunkAff = chunkDomainPwAff(readScatter.toPwMultiAff(), levelOfDep, false).setOutTupleId(recvId); // { readStmt[domain] -> recv[domain] }
      auto readChunks = readChunkAff.reverse();

      //auto sendDomain = writePartitioning.getRange();
      auto recvDomain = readChunks.getDomain();
      auto chunkSpace = recvDomain.getSpace();

      // Stmts might be executed multiple times on different nodes, hence we need to add the information on which node to the instance
      auto readNodeId = islctx->createId(readTupleId.getName() + Twine("_") + clusterTupleId.getName(), readTupleId.getUser());
      auto readNodeShape = clusterShape.setTupleId(readNodeId);
      auto readInstDomain = readWhere.setOutTupleId(readNodeId).wrap(); // { (readStmt[domain], node[cluster]) }
      auto readInstMap = readInstDomain.unwrap().domainMap().reverse(); // { readStmt[domain] -> (readStmt[domain], node[cluster]) }
      auto writeNodeId = islctx->createId(writeTupleId.getName() + Twine("_") + clusterTupleId.getName(), writeTupleId.getUser());
      auto writeNodeShape = clusterShape.setTupleId(writeNodeId);
      auto writeInstDomain = writeWhere.setOutTupleId(writeNodeId).wrap(); // { (writeStmt[domain], node[cluster]) }
      auto writeInstDomainSpace = writeInstDomain.getSpace();
      auto writeInstDomainSpaceUnwrapped = writeInstDomainSpace.unwrap();
      auto writeInstMap = writeInstDomain.unwrap().domainMap().reverse(); // { readStmt[domain] -> (readStmt[domain], node[cluster]) }
      auto writeFvarname = writeFVar->getVariable()->getName();

      // Adapt the other maps to this
      //auto readInstSubDomain = readWhere.intersectDomain(readSubDomain).wrap();
      //auto writeInstSubDomain = writeWhere.intersectDomain(writeSubDomain).wrap();

      auto readInstScatter = readInstDomain.chainSubspace(readScatter);          // { (readStmt[domain], node[cluster]) -> scattering[scatter] }
      auto writeInstScatter = writeInstDomain.chainSubspace(writeScatter);       // { (writeStmt[domain], node[cluster]) -> scattering[scatter] }
      //auto readInstSubScatter = readInstDomain.chainSubspace(readSubScatter);    // { (readStmt[domain], node[cluster]) -> subscattering[scatter] }
      //auto writeInstSubScatter = writeInstDomain.chainSubspace(writeSubScatter); // { (writeStmt[domain], node[cluster]) -> subscattering[scatter] }
      auto readInstAccess = readInstDomain.chainSubspace(readAccRel);            // { (readStmt[domain], node[cluster]) -> field[index] }
      auto writeInstAccess = writeInstDomain.chainSubspace(writeAccRel);         // { (writeStmt[domain], node[cluster]) -> field[index] }

      auto depInst = dep.applyDomain(writeInstMap).applyRange(readInstMap);       // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster]) }
      assert(!depInst.isEmpty());
      //auto depSubInst = depSub.applyDomain(writeInstMap).applyRange(readInstMap); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster]) }


      //auto depSubInstLoc = depSubInst.chainNested(isl_dim_out, readInstAccess); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster], field[index]) }
      auto depInstLoc = depInst.chainNested(isl_dim_in, writeAccRel).chainNested(isl_dim_out, readAccRel); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster], field[index]) }
      auto eqField = depInstLoc.getSpace().equalSubspaceBasicMap(indexsetSpace);
      assert(depInstLoc <= eqField);
      depInstLoc.intersect_inplace(eqField);
      depInstLoc.projectOutSubspace_inplace(isl_dim_in, indexsetSpace);

      //auto depInstLocChunks = depInstLoc.reverse().wrap().chainSubspace(readChunkAff).reverse(); // { recv[domain] -> (readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }

      // For every read, we need to select one write instance. It will decide from which node we will receive the data
      // TODO: This selection is arbitrary; preferable select data from local node and nodes we have to send data from anyways
      // { (readStmt[domain], node[cluster], field[index]) -> (writeStmt[domain], node[cluster]) }
      auto producers = depInstLoc.wrap().reorganizeSubspaces(readInstDomain.getSpace() >> indexsetSpace, writeInstDomain.getSpace());
      auto sourceOfRead = producers.anyElement();
      assert(sourceOfRead.range() <= writeInstDomain);

      // Organize in chunks
      // { recv[domain] -> (readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }
      auto sourceOfReadChunks = sourceOfRead.wrap().chainSubspace(readChunkAff).reverse();

      sourceOfReadChunks = simplify(sourceOfReadChunks.move());

      // { (recv[domain], node[cluster]) -> (readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }
      auto recvChunks = sourceOfReadChunks.wrap().reorganizeSubspaces(recvDomain.getSpace() >> readNodeShape.getSpace(), readInstDomain.getSpace() >> indexsetSpace >> writeInstDomain.getSpace());
      auto recvInstDomain = recvChunks.getDomain(); // { (recv[domain], node[cluster]) }


      // Select a source write for each read
      // { (recv[domain], node[cluster]) -> (writeStmt[domain], node[cluster]) }
      auto firstWritePerChunk = sourceOfReadChunks.wrap().reorganizeSubspaces(recvDomain.getSpace() >> readNodeShape.getSpace(), writeInstDomain.getSpace()).lexmaxPwMultiAff();
      // { (send[domain], node[cluster]) }
      //auto sendInstDomain = firstWritePerChunk.getRange().unwrap().setInTupleId(sendId).wrap();
      // Insert complete information again
      auto recvChunksWrapped = sourceOfReadChunks.wrap(); // { (recv[domain], readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }


      // { (recv[domain], readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }
      //auto chunks = recvChunksWrapped.intersect(firstWritePerChunk.wrap().reorganizeSubspaces(recvChunksWrapped.getSpace()));
      auto allChunks = recvChunksWrapped;

      auto localChunks = allChunks.intersect(isl::Space::createMapFromDomainAndRange(readNodeShape.getSpace(), writeNodeShape.getSpace()).equalBasicMap().wrap().toSet().reorganizeSubspaces(allChunks.getSpace()));
      auto remoteChunks = (allChunks - localChunks);

      //writeInstDomain = chunks.reorganizeSubspaces(writeInstDomain.getSpace()); // { (writeStmt[domain], node[cluster]) }
      //writeInstScatter = writeInstDomain.chainSubspace(writeScatter);  // { (writeStmt[domain], node[cluster]) -> scattering[scatter] }
      //auto writeInstWhere = writeInstDomain.reorganizeSubspaces(writeInstDomain.getSpace(), writeInstDomain.getSpace().unwrap().getRangeSpace()); // { (writeStmt[domain], node[cluster]) -> scattering[scatter] }

      // Order write is chunks as defined by already selected read chunks
      //auto sendByWrite = chunks.reorganizeSubspaces(recvInstDomain.getSpace(), writeInstDomain.getSpace()); // { (recv[domain], node[cluster]) -> writeStmt[domain] } 
      // { (recv[domain] -> node[cluster]) -> sendScattering[scatter] }
      //auto sendScatter = relativeScatter(sendByWrite, writeScatter.wrap().reorganizeSubspaces(writeInstDomain.getSpace(), writeScatter.getRangeSpace()), -1);


      //auto writeFlowWhere = chunks.reorganizeSubspaces(writeDomain.getSpace() >> readNodeShape.getSpace() >> readChunkAff.getRangeSpace(), writeNodeShape.getSpace()); // { (writeStmt[domain], dstNode[cluster], recv[domain]) -> node[cluster] }
      //auto writeFlowDomain = writeFlowWhere.getDomain(); // { (writeStmt[domain], dstNode[cluster], recv[domain]) }
      //auto writeFlowScatter = writeFlowDomain.chainSubspace(writeScatter);

      //auto readFlowWhere = chunks.reorganizeSubspaces(readDomain.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { readStmt[domain] -> node[cluster] }
      //auto readFlowDomain = readFlowWhere.getDomain();
      //auto readFlowScatter = readWhere;

      //auto communicationSet = depInstLoc;
      //communicationSet.projectOutSubspace_inplace(isl_dim_in, writeDomain.getSpace()); /* { (nodeSrc[cluster], nodeDst[cluster]) -> field[indexset] } */
      //communicationSet.projectOutSubspace_inplace(isl_dim_in, indexsetSpace);
      //communicationSet.projectOutSubspace_inplace(isl_dim_out, readDomain.getSpace());
      // { writeNode[cluster] -> (readNode[cluster], field[index]) }
      //communicationSet.uncurry_inplace();
      // { (writeNode[cluster], readNode[cluster]) -> field[index] }
      //auto communicationSet = sourceOfRead.wrap().reorganizeSubspaces(writeNodeShape.getSpace() >> readNodeShape.getSpace(), indexsetSpace);

      // We may need to send to/recv from any node in the cluster
      //auto sendDstDomain = depInst.getDomain().unwrap().setInTupleId(sendDomain.getTupleId()).intersectDomain(sendDomain).wrap(); // { (writeStmt[domain], nodeDst[cluster]) }
      //auto recvSrcDomain = depInst.getRange().unwrap().setInTupleId(recvDomain.getTupleId()).intersectDomain(recvDomain).wrap(); // { (readStmt[domain], nodeDst[cluster]) }

      // Execute all send/recv statement in parallel, i.e. same scatter point
      //auto sendDstScatter = sendDstDomain.chainSubspace(writeScatter.setInTupleId(sendDomain.getTupleId())); // { (writeStmt[domain], nodeDst[cluster]) -> scattering[scatter] }
      //auto recvSrcScatter = recvSrcDomain.chainSubspace(readScatter.setInTupleId(recvDomain.getTupleId())); // { (writeStmt[domain], nodeDst[cluster]) -> scattering[scatter] }

      //auto sendDstWhere = sendDstDomain.unwrap().rangeMap(); // { (writeStmt[domain], nodeDst[cluster]) -> nodeDst[cluster] }
      //auto recvSrcWhere = recvSrcDomain.unwrap().rangeMap(); // { (readStmt[domain], nodeSrc[cluster]) -> nodeSrc[cluster] }
      localChunks.coalesce_inplace();
      remoteChunks.coalesce_inplace();
     
      DEBUG(llvm::dbgs() << "Complexity local=" << (localChunks.getBasicSetCount()) << " remote=" << (remoteChunks.getBasicSetCount()) << "\n");



      ScopEditor editor(scop, asPass());


      if (!localChunks.isEmpty()) {
        // Source and target node being the same does not mean that the field element has a home on that node, we therefore cannot use the field as storage

        auto sameNodeChunks = localChunks.reorganizeSubspaces(readDomain.getSpace() >> writeDomain.getSpace() >> indexsetSpace, writeNodeShape.getSpace()).setOutTupleId(clusterTupleId).wrap(); // { readStmt[domain], writeStmt[domain], field[indexset], node[cluster] }
        auto layout = fvar->getLayout();
        auto storageRequired = sameNodeChunks.reorderSubspaces(clusterSpace, indexsetSpace); // { node[cluster] -> fty[indexset] }
#if 0
        auto storageAvailable = layout->getIndexableIndices(); // { node[cluster] -> fty[phys_indexset] }

        if (storageAvailable >= storageRequired) {
          int a = 0;
          // No need to allocate more memory, everything can be put into the field's local storage
          // TODO: Implement; also genOutputCommunication does not need to write those again if the latest value has already been written here
          // TODO: In future, we may overapproximate field-local storage such that these elements can always be stored in the field's local storage
#if 0
          // write: { writeStmt[domain] }
          {
            auto localwriteWhere = localChunks.reorganizeSubspaces(writeDomain.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { write[domain] -> rank[cluster] }
            auto localwriteScatter = writeScatter;
            auto localwriteEditor = writeEditor.createStmt(localwriteWhere.domain(), localwriteScatter, localwriteWhere, "localwrite");
            auto localwriteStmt = getScopStmtContext(localwriteEditor.getStmt());
            auto localwriteCodegen = localwriteStmt->makeCodegen();

            auto localwriteCurrentIteration = localwriteEditor.getCurrentIteration(); // { ??? -> localwrite[domain] }
            auto localwriteCurrentDomain = localwriteCurrentIteration.castRange(writeDomain.getSpace()); // { ??? -> writeStmt[domain] }
            auto localwriteCurrentNode = localwriteStmt->getClusterMultiAff(); // { ??? -> rank[cluster] }
            auto localwriteIndex = localwriteCurrentDomain.applyRange(writeAccRel.toPwMultiAff());

            auto localwriteValPtr = writeStmt->getAccessStackStorageAnnPtr().pullbackDomain(localwriteCurrentDomain);
            auto localwriteFieldPtr = localwriteCodegen.codegenFieldLocalPtr(writeFVar, localwriteCurrentNode, localwriteIndex);
            localwriteCodegen.codegenAssign(localwriteBufPtr, localwriteValPtr);

            localwriteCodegen.markBlock((Twine("flow local write '") + writeDomain.getSetTupleName() + "'" + writeFvarname).str(), localwriteCurrentIteration);
          }

          // read: { readStmt[domain] }
          {
            auto localreadWhere = localChunks.reorganizeSubspaces(readDomain.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId);
            auto localreadScatter = readScatter;
            auto localreadEditor = readEditor.createStmt(localreadWhere.domain(), localreadScatter, localreadWhere, "localread");
            auto localreadStmt = getScopStmtContext(localreadEditor.getStmt());
            readEditor.removeInstances(localreadWhere);
            auto localreadCodegen = localreadStmt->makeCodegen();

            auto localreadCurrentIteration = localreadEditor.getCurrentIteration(); // { ??? -> localread[domain] }
            auto localreadCurrentDomain = localreadCurrentIteration.castRange(readDomain.getSpace()); // { ??? -> readStmt[domain] }
            auto localreadCurrentNode = localreadStmt->getClusterMultiAff(); // { ??? -> rank[cluster] }
            auto localreadIndex = localreadCurrentDomain.applyRange(readAccRel.toPwMultiAff());

            auto localreadFieldPtr = localreadCodegen.codegenFieldLocalPtr(readFvar, localreadCurrentNode, localreadIndex);
            auto localreatStackPtr = readStmt->getAccessStackStorageAnnPtr().pullbackDomain(localreadCurrentDomain);
            localreadCodegen.codegenAssign(localreatStackPtr, localreadFieldPtr);

            localreadCodegen.markBlock((Twine("flow local read '") + readDomain.getSetTupleName() + "'" + readFvarname).str(), localreadCurrentIteration);
          }
#endif
        }
#endif

        // Allocate stack space for these
        auto mapper = RectangularMapping::createRectangualarHullMapping(storageRequired);

        auto funcCtx = getFunctionProcessor();
        auto currentNode = funcCtx->getCurrentNodeCoordinate(); // { [] -> rank[cluster] }
        auto funcCodegen = funcCtx->makeEntryCodegen();
        auto size = mapper->codegenSize(funcCodegen, currentNode);

        Value *localbuf = nullptr;
        if (auto c = dyn_cast<ConstantInt>(size)) {
          auto sizeConst = c->getZExtValue();
          if (sizeConst <= 128) { // Max elements we put on the stack
            localbuf = funcCodegen.allocStackSpace(fty->getEltType(), size, "localflowbuf");
          }
        }
        if (!localbuf) {
          auto lbuf = pm->newLocalBuffer(fvar->getEltType(), mapper);
          //auto localbufptr = funcCodegen.getIRBuilder().CreateLoad(lbuf->getGlobalVariable(), "localflowbufptr");
          //localbuf = funcCodegen.getIRBuilder().CreateLoad(localbufptr, "localflowbuf");
          //localbuf = funcCodegen.getIRBuilder().CreateLoad(lbuf->getGlobalVariable(), "localflowbuf");

          auto localbufobj = funcCodegen.getIRBuilder().CreateLoad(lbuf->getGlobalVariable(), "localbufobj");
          localbuf = funcCodegen.callCombufLocalDataPtr(localbufobj);
        }
        localbuf = funcCodegen.getIRBuilder().CreatePointerCast(localbuf, fvar->getEltPtrType());

        // write: { writeStmt[domain] }
        {
          auto localwriteWhere = localChunks.reorganizeSubspaces(writeDomain.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { write[domain] -> rank[cluster] }
          auto localwriteScatter = writeScatter;
          auto localwriteEditor = writeEditor.createStmt(localwriteWhere.domain(), localwriteScatter, localwriteWhere, "localwrite");
          auto localwriteStmt = getScopStmtContext(localwriteEditor.getStmt());

          auto localwriteCurrentIteration = localwriteEditor.getCurrentIteration(); // { ??? -> localwrite[domain] }
          auto localwriteCurrentDomain = localwriteCurrentIteration.castRange(writeDomain.getSpace()); // { ??? -> writeStmt[domain] }
          auto localwriteCurrentNode = localwriteStmt->getClusterMultiAff(); // { ??? -> rank[cluster] }
          auto localwriteIndex = localwriteCurrentDomain.applyRange(writeAccRel.toPwMultiAff());

          auto localwriteCodegen = localwriteStmt->makeCodegen();
          auto localwriteValPtr = writeStmt->getAccessStackStorageAnnPtr().pullbackDomain(localwriteCurrentDomain);
          auto idx = mapper->codegenIndex(localwriteCodegen, localwriteCurrentNode, localwriteIndex);
          auto ptr = localwriteCodegen.getIRBuilder().CreateGEP(localbuf, idx);
          auto localwriteBufPtr = AnnotatedPtr::createArrayPtr(ptr, localbuf, localwriteIndex);
          localwriteCodegen.codegenAssign(localwriteBufPtr, localwriteValPtr);

          localwriteCodegen.markBlock((Twine("flow local write '") + writeDomain.getSetTupleName() + "'" + writeFvarname).str(), localwriteCurrentIteration);
        }

        // read: { readStmt[domain] }
        {
          auto localreadWhere = localChunks.reorganizeSubspaces(readDomain.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId);
          auto localreadScatter = readScatter;
          auto localreadEditor = readEditor.createStmt(localreadWhere.domain(), localreadScatter, localreadWhere, "localread");
          auto localreadStmt = getScopStmtContext(localreadEditor.getStmt());
          readEditor.removeInstances(localreadWhere);
          auto localreadCodegen = localreadStmt->makeCodegen();

          auto localreadCurrentIteration = localreadEditor.getCurrentIteration(); // { ??? -> localread[domain] }
          auto localreadCurrentDomain = localreadCurrentIteration.castRange(readDomain.getSpace()); // { ??? -> readStmt[domain] }
          auto localreadCurrentNode = localreadStmt->getClusterMultiAff(); // { ??? -> rank[cluster] }
          auto localreadIndex = localreadCurrentDomain.applyRange(readAccRel.toPwMultiAff());

          auto idx = mapper->codegenIndex(localreadCodegen, localreadCurrentNode, localreadIndex);
          auto ptr = localreadCodegen.getIRBuilder().CreateGEP(localbuf, idx);
          auto localreadBufPtr = AnnotatedPtr::createArrayPtr(ptr, localbuf, localreadIndex);
          auto localreatStackPtr = readStmt->getAccessStackStorageAnnPtr().pullbackDomain(localreadCurrentDomain);
          localreadCodegen.codegenAssign(localreatStackPtr, localreadBufPtr);

          localreadCodegen.markBlock((Twine("flow local read '") + readDomain.getSetTupleName() + "'" + readFvarname).str(), localreadCurrentIteration);
        }
      }


      if (!remoteChunks.isEmpty()) {
        auto comRelation = remoteChunks.reorganizeSubspaces(chunkSpace, (writeNodeShape.getSpace() >> readNodeShape.getSpace()) >> indexsetSpace); // { recv[] -> (writeNode[cluster], readNode[cluster], field[indexset]) }

        // Codegen
        //TODO: Handle local communication separately, do not use combuf
        auto combuf = pm->newCommunicationBuffer(fty, comRelation);
        combuf->doLayoutMapping();

        // send_wait { [chunk], dstNode[cluster] }
        {
          auto sendwaitWhere = remoteChunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> readNodeShape.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId);  // { (recv[domain], dstNode[cluster]) -> srcNode[cluster] }
          auto sendwaitScatter = relativeScatter(remoteChunks.reorganizeSubspaces(sendwaitWhere.getDomainSpace(), writeDomain.getSpace()), writeScatter, -1); // { (recv[domain], dstNode[cluster]) -> scatter[scattering] }
          auto sendwaitEditor = editor.createStmt(sendwaitWhere.getDomain() /* { recv[domain], dstNode[cluster] } */, sendwaitScatter.copy(), sendwaitWhere.copy(), "send_wait");
          auto sendwaitStmt = getScopStmtContext(sendwaitEditor.getStmt());

          auto sendwaitCurrentIteration = sendwaitEditor.getCurrentIteration(); // { [] -> (recv[domain], dstNode[cluster]) }
          auto sendwaitCurrentChunk = sendwaitCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
          auto sendwaitCurrentDst = sendwaitCurrentIteration.sublist(readNodeShape.getSpace()); // { [] -> dstNode[cluster] }
          auto sendwaitCurrentNode = sendwaitStmt->getClusterMultiAff().setOutTupleId(writeNodeId); // { [] -> srcNode[cluster] }
          //combuf->codegenSendWait(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);

          auto sendwaitCodegen = sendwaitStmt->makeCodegen();
          auto sendbufPtrPtr = combuf->codegenSendbufPtrPtr(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);
          auto bufptr = combuf->codegenSendWait(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);
          sendwaitCodegen.getIRBuilder().CreateStore(bufptr, sendbufPtrPtr);

          sendwaitCodegen.markBlock((Twine("flow remote send_wait '") + writeDomain.getSetTupleName() + "'" + writeFvarname).str());
        }

        // write { writeStmt[domain], dstNode[cluster], recv[domain] }
        {
          auto writeflowWhere = remoteChunks.reorganizeSubspaces(writeDomain.getSpace() >> readNodeShape.getSpace() >> readChunkAff.getRangeSpace(), writeNodeShape.getSpace()).castRange(clusterSpace); // { (writeStmt[domain], dstNode[cluster], recv[domain]) -> rank[cluster] } 
          auto writeflowDomain = writeflowWhere.domain();
          auto writeflowScatter = writeflowDomain.chainSubspace(writeScatter);
          auto writeFlowEditor = writeEditor.createStmt(writeflowDomain, writeflowScatter, writeflowWhere, "writeflow");
          //writeEditor.removeInstances(writeFlowWhere.wrap().reorganizeSubspaces(writeDomain.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId));
          auto writeFlowStmt = getScopStmtContext(writeFlowEditor.getStmt());

          //auto writeStore = writeStmt->getStoreAccessor();
          //auto writeAcc = writeStmt->getAccess();
          auto writeAccessed = writeStmt->getAccessed().toPwMultiAff(); // { writeStmt[domain] -> field[indexset] }
          //auto writeVal = writeStore->getValueOperand();
          //auto writeflowVal = writeFlowCodegen.materialize(writeVal);
          //auto writeflowVal = writeFlowCodegen.materialize(writeAcc.getWrittenValueRegister());

          auto writeFlowCurrentIteration = writeFlowEditor.getCurrentIteration();
          auto writeFlowCurrentDst = writeFlowCurrentIteration.sublist(readNodeShape.getSpace());
          auto writeFlowCurrentChunk = writeFlowCurrentIteration.sublist(readChunkAff.getRangeSpace());
          auto writeFlowCurrentDomain = writeFlowCurrentIteration.sublist(writeDomain.getSpace());
          auto writeFlowCurrentWriteAccessed = writeFlowCurrentDomain.applyRange(writeAccessed);
          auto writeFlowCurrentNode = writeFlowStmt->getClusterMultiAff().setOutTupleId(writeNodeId);
          //combuf->codegenStoreInSendbuf(writeFlowCodegen, writeFlowCurrentChunk, writeFlowCurrentNode, writeFlowCurrentDst, writeFlowCurrentWriteAccessed, writeflowVal);

          auto writeFlowCodegen = writeFlowStmt->makeCodegen();
          auto writeflowBufPtr = combuf->codegenSendbufPtr(writeFlowCodegen, writeFlowCurrentChunk, writeFlowCurrentNode, writeFlowCurrentDst, writeFlowCurrentWriteAccessed);
          auto writeflowStackPtr = writeStmt->getAccessStackStorageAnnPtr().pullbackDomain(writeFlowCurrentDomain);
          writeFlowCodegen.codegenAssign(writeflowBufPtr, writeflowStackPtr);

          writeFlowCodegen.markBlock((Twine("flow remote write '") + writeDomain.getSetTupleName() + "'" + writeFvarname).str());
        }

        // send
        {
          auto sendWhere = remoteChunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> readNodeShape.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { (recv[domain], dstNode[cluster]) -> srcNode[cluster] }
          auto sendScatter = relativeScatter(remoteChunks.reorganizeSubspaces(sendWhere.getDomainSpace(), writeDomain.getSpace()), writeScatter, +1); // { (recv[domain], dstNode[cluster]) -> scatter[scattering] }
          auto sendEditor = editor.createStmt(sendWhere.getDomain() /* { recv[domain], dstNode[cluster] } */, sendScatter.copy(), sendWhere.copy(), "send");
          auto sendStmt = getScopStmtContext(sendEditor.getStmt());

          auto sendCurrentIteration = sendEditor.getCurrentIteration(); // { [] -> (recv[domain], dstNode[cluster]) }
          auto sendCurrentChunk = sendCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
          auto sendCurrentDst = sendCurrentIteration.sublist(readNodeShape.getSpace()); // { [] -> dstNode[cluster] }
          auto sendCurrentNode = sendStmt->getClusterMultiAff().setOutTupleId(writeNodeId); // { [] -> srcNode[cluster] }
          
          auto sendCodegen = sendStmt->makeCodegen();
          combuf->codegenSend(sendCodegen, sendCurrentChunk, sendCurrentNode, sendCurrentDst);
         
          sendCodegen.markBlock((Twine("flow remote send '") + writeDomain.getSetTupleName() + "'" + writeFvarname).str());
        }

        // recv_wait
        {
          auto recvwaitWhere = remoteChunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> writeNodeShape.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { (readStmt[domain], srcNode[cluster]) -> dstNode[cluster] } 
          auto recvwaitScatter = relativeScatter(remoteChunks.reorganizeSubspaces(recvwaitWhere.getDomainSpace(), readDomain.getSpace()), readScatter, -1);
          auto recvwaitEditor = editor.createStmt(recvwaitWhere.getDomain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recv");
          auto recvwaitStmt = getScopStmtContext(recvwaitEditor.getStmt());

          auto recvwaitCurrentIteration = recvwaitEditor.getCurrentIteration(); // { [] -> (recv[domain], srcNode[cluster]) }
          auto recvwaitCurrentChunk = recvwaitCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
          auto recvwaitCurrentSrc = recvwaitCurrentIteration.sublist(writeNodeShape.getSpace()); // { [] -> srcNode[cluster] }
          auto recvwaitCurrentNode = recvwaitStmt->getClusterMultiAff().setOutTupleId(readNodeId); // { [] -> dstNode[cluster] }

          auto recvwaitCodegen = recvwaitStmt->makeCodegen();
          auto bufPtr = combuf->codegenRecvWait(recvwaitCodegen, recvwaitCurrentChunk, recvwaitCurrentSrc, recvwaitCurrentNode);
          auto sendbufPtrPtr = combuf->codegenRecvbufPtrPtr(recvwaitCodegen, recvwaitCurrentChunk, recvwaitCurrentSrc, recvwaitCurrentNode);
          recvwaitCodegen.getIRBuilder().CreateStore(bufPtr, sendbufPtrPtr);

          recvwaitCodegen.markBlock((Twine("flow remote recv_wait '") + readDomain.getSetTupleName() + "'" + readFvarname).str());
        }


        // read
        {
          // Modify read such that it reads from the combuf instead
          auto readflowWhere = remoteChunks.reorganizeSubspaces(readDomain.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { readStmt[domain] -> node[cluster] }
          auto readflowScatter = readScatter;
          auto readFlowEditor = readEditor.createStmt(readflowWhere.domain(), readflowScatter, readflowWhere, "readFlow");
          readEditor.removeInstances(readflowWhere);
          auto readFlowStmt = getScopStmtContext(readFlowEditor.getStmt());

          //auto readLoad = readStmt->getLoadAccessor();
          auto readAccessed = readStmt->getAccessed().toPwMultiAff(); // { readStmt[domain] -> field[indexset] }

          auto readFlowCurrentDomain = readFlowEditor.getCurrentIteration().setOutTupleId(readDomain.getTupleId());
          auto readFlowCurrentChunk = readFlowCurrentDomain.applyRange(readChunkAff);
          auto readFlowCurrentReadAccessed = readFlowCurrentDomain.applyRange(readAccessed);
          auto readFlowCurrentNode = readFlowStmt->getClusterMultiAff().setOutTupleId(readNodeId);
          auto whatsthesourceInfo = rangeProduct(rangeProduct(readFlowCurrentDomain, readFlowCurrentNode), readFlowCurrentReadAccessed.toMultiPwAff()).toPwMultiAff();
          auto readFlowCurrentSrc = sourceOfRead.sublist(writeNodeShape.getSpace()).pullback(whatsthesourceInfo);
          //auto readflowVal = combuf->codegenLoadFromRecvBuf(readFlowCodegen, readFlowCurrentChunk, readFlowCurrentSrc, readFlowCurrentNode, readFlowCurrentReadAccessed);
          //readFlowCodegen.updateScalar(readLoad, readflowVal);

          auto readFlowCodegen = readFlowStmt->makeCodegen();
          auto readflowStackPtr = readStmt->getAccessStackStorageAnnPtr().pullbackDomain(readFlowCurrentDomain);
         static int count = 0;
         count +=1;
         if (count==8) {
         int a = 0;
         }
          auto readflowBufPtr = combuf->codegenRecvbufPtr(readFlowCodegen, readFlowCurrentChunk, readFlowCurrentSrc, readFlowCurrentNode, readFlowCurrentReadAccessed);
          readFlowCodegen.codegenAssign(readflowStackPtr, readflowBufPtr);

          readFlowCodegen.markBlock((Twine("flow remote read '") + readDomain.getSetTupleName() + "'" + readFvarname).str());
        }

        // recv
        {
          auto recvWhere = remoteChunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> writeNodeShape.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { (readStmt[domain], srcNode[cluster]) -> dstNode[cluster] } 
          auto recvScatter = relativeScatter(remoteChunks.reorganizeSubspaces(recvWhere.getDomainSpace(), readDomain.getSpace()), readScatter, +1);
          auto recvEditor = editor.createStmt(recvWhere.getDomain(), recvScatter, recvWhere, "recv");
          auto recvStmt = getScopStmtContext(recvEditor.getStmt());

          auto recvCurrentIteration = recvEditor.getCurrentIteration(); // { [] -> (recv[domain], srcNode[cluster]) }
          auto recvCurrentChunk = recvCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
          auto recvCurrentSrc = recvCurrentIteration.sublist(writeNodeShape.getSpace()); // { [] -> srcNode[cluster] }
          auto recvCurrentNode = recvStmt->getClusterMultiAff().setOutTupleId(readNodeId); // { [] -> dstNode[cluster] }
          
          auto recvCodegen = recvStmt->makeCodegen();
          combuf->codegenRecv(recvCodegen, recvCurrentChunk, recvCurrentSrc, recvCurrentNode);

          recvCodegen.markBlock((Twine("flow remote recv '") + readDomain.getSetTupleName() + "'" + readFvarname).str());
        }
      } // if (!remoteChunks.isempty())
    } // void genFlowCommunication(const isl::Map &dep) 


    void genOutputCommunication(const isl::Set &outSet/* { writeStmt[domain] } */) {
      //DEBUG(llvm::dbgs() << "OutputCommunication: "; outSet.print(llvm::dbgs()); llvm::dbgs() << "\n");

      auto writeDomainTuple = outSet.getTupleId();
      //auto writeStmt = tupleToStmt[writeDomainTuple.keep()];
      auto writeStmtCtx = getScopStmtContext(outSet);
      auto writeEditor = writeStmtCtx->getEditor();
      auto srcNodeId = islctx->createId("srcNode");
      auto dstNodeId = islctx->createId("dstNode");
      auto fvar = writeStmtCtx->getFieldVariable();
      auto fvarId = fvar->getTupleId();
      auto fvarname = fvar->getVariable()->getName();
      auto clusterSpace = getClusterConfig()->getClusterSpace();
      auto clusterNodeId = clusterSpace.getSetTupleId();
      auto dstNodeSpace = clusterSpace.setSetTupleId(dstNodeId);

      auto writeAccessed = writeStmtCtx->getAccessRelation(); // { writeStmt[domain] -> field[indexset] }
      auto out = writeAccessed.intersectDomain(outSet); // { writeStmt[domain] -> field[indexset] }
      auto fty = writeStmtCtx->getFieldType();
      //auto fvar = writeStmtCtx->getFieldVariable();
      auto layout = fvar->getDefaultLayout();
      auto fieldHome = layout->getPhysicalNode(); // { field[indexset] -> node[cluster] }
      auto readHome = fieldHome.setOutTupleId(dstNodeId).setInTupleId(fvarId);
      auto writeHome = fieldHome.setOutTupleId(srcNodeId).setInTupleId(fvarId);
      auto indexsetSpace = writeAccessed.getRangeSpace();
      auto indexsetTupleId = indexsetSpace.getSetTupleId();

      auto writeWhere = writeStmtCtx->getWhere().setOutTupleId(srcNodeId);
      auto srcNodeSpace = writeWhere.getRangeSpace();
      auto writeOutInstances = writeWhere.wrap().chainSubspace(out); // { (writeStmt[domain], writeNode[cluster]) -> field[indexset] }


      //auto selectedWrite = writeOutInstances.reverse(); // { field[indexset] -> (writeStmt[domain], writeNode[cluster]) }
#if 0
      // For each index, select one write instance that will be stored into the array
      auto selectedWrite = writeOutInstances.reverse().lexminPwMultiAff(); // { field[indexset] -> (writeStmt[domain], writeNode[cluster]) }
      auto selectedWriteMap1 = selectedWrite.toMap();
      auto selectedWriteMap2 = writeOutInstances.reverse().lexmin();
      assert(selectedWriteMap1 == selectedWriteMap2);
      selectedWriteMap1.coalesce_inplace();
      auto selectedWriteMap3 = writeOutInstances.reverse();
      if (selectedWriteMap3 == selectedWriteMap1) {
      // Unique anyway; use selectedWriteMap3
        auto x = selectedWriteMap3.toPwMultiAff();
        int a = 0;
      }
#endif

      auto writeInstDomain = writeOutInstances.getDomain(); // { (writeStmt[domain], writeNode[cluster]) }
      auto writeInstDomainSpace = writeInstDomain.getSpace();
      auto writeDomain = writeInstDomain.unwrap().getDomain(); // { writeStmt[domain] }
      auto writeDomainSpace = writeDomain.getSpace();
      auto writeScatter = writeStmtCtx->getScattering().intersectDomain(writeDomain); // { writeStmt[domain] -> scattering[scatter] }
      auto epilogueDomain = afterScopScatter.getDomain(); // { epilogue[] }
      auto epilogueDomainSpace = epilogueDomain.getSpace();

      auto transfer = writeOutInstances.wrap().chainSubspace(readHome).wrap(); // { (field[indexset], writeStmt[domain], srcNode[cluster], dstNode[cluster]) }
      auto local = transfer.intersect(transfer.getSpace().equalBasicSet(dstNodeSpace, srcNodeSpace)); // { (field[indexset], writeStmt[domain], srcNode[cluster], dstNode[cluster]) }

      // for each local element, what are the possible source nodes?
      auto transferSrcNode = transfer.reorderSubspaces(indexsetSpace >> dstNodeSpace, srcNodeSpace >> writeDomainSpace); // { (field[indexset],  dstNode[cluster]) -> (srcNode[cluster], writeStmt[domain]) }
      auto localSrcNode = local.reorderSubspaces(indexsetSpace >> dstNodeSpace, srcNodeSpace >> writeDomainSpace);

      auto remoteSrcNode = transferSrcNode.subtractDomain(localSrcNode.domain()); // { (field[indexset],  dstNode[cluster]) -> (srcNode[cluster], writeStmt[domain]) }

      // Find unique src nodes for remote transfers
      auto remoteSrcNodeUnique = remoteSrcNode.anyElement(); // { (field[indexset], dstNode[cluster]) -> (srcNode[cluster], writeStmt[domain]) }
      //if (remoteSrcNode == remoteSrcNodeUnique.toMap()) {
        // Use less complex version
      //  remoteSrcNodeUnique = remoteSrcNode.toPwMultiAff();
      //}
      //remoteSrcNodeUnique.coalesce_inplace();

      //auto remoteTransfers = transfer - local;
     // auto localTransfers = local.reorderSubspaces(indexsetSpace >> (writeDomainSpace >> srcNodeSpace)).cast((indexsetSpace >> (writeDomainSpace >> clusterSpace)).normalizeWrapped()); // { (field[indexset], writeStmt[domain], node[cluster]) } 

      auto remoteTransfers = remoteSrcNodeUnique.wrap();
      auto localTransfers = localSrcNode.wrap().reorderSubspaces(indexsetSpace >> writeDomainSpace >> dstNodeSpace).cast((indexsetSpace >> writeDomainSpace >> clusterSpace).normalizeWrapped());  // { (field[indexset], writeStmt[domain], node[cluster]) } 
      remoteTransfers.coalesce_inplace(); localTransfers.coalesce_inplace();
      DEBUG(llvm::dbgs() << "Complexity local=" << (localTransfers.getBasicSetCount()) << " remote=" << (remoteTransfers.getBasicSetCount()) << "\n");

      ScopEditor editor(scop, asPass());


      // { writeStmt[domain] }: writeback (local)
      if (!localTransfers.isEmpty()) {
        auto writelocalWhere = localTransfers.reorderSubspaces(writeDomainSpace, clusterSpace);
        auto writelocalScatter = writeScatter;
        auto writelocalEditor = writeEditor.createStmt(writelocalWhere.domain(), writelocalScatter, writelocalWhere, "writeback_local");
        //writeEditor.removeInstances(writelocalWhere);
        auto writelocalStmt = writelocalEditor.getStmt();
        auto writelocalStmtCtx = getScopStmtContext(writelocalStmt);
        auto writelocalCodegen = writelocalStmtCtx->makeCodegen();

        //auto writeStore = writeStmtCtx->getStoreAccessor();
        //auto writeVal = writeStore->getValueOperand();
        auto writeAcc = writeStmtCtx->getAccess();
        //auto writeVal = writeAcc.getWrittenValueRegister();
        //auto writeAccessRel = writeStmtCtx->getAccessRelation();
        auto writeAccessed = writeStmtCtx->getAccessed();

        auto writelocalCurrent = writelocalEditor.getCurrentIteration().setOutTupleId(writeDomainTuple);
        auto writelocalCurrentNode = writelocalStmtCtx->getClusterMultiAff(); // { writelocalStmt[domain] -> node[cluster] }
        auto writelocalCurrentAccessed = writeAccessed.setInTupleId(writelocalCurrent.getInTupleId()); // { writelocalStmt[domain] -> field[indexset] }

        auto localbufPtr = writelocalCodegen.codegenFieldLocalPtr(fvar, writelocalCurrentNode, writelocalCurrentAccessed);
        auto stackPtr = writeStmtCtx->getAccessStackStorageAnnPtr().pullbackDomain(writelocalCurrent);
        writelocalCodegen.codegenAssign(localbufPtr, stackPtr);

        writelocalCodegen.markBlock((Twine("output local '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
      }


      if (!remoteTransfers.isEmpty()) {
        auto comrel = alltoall(epilogueDomain, remoteTransfers.reorderSubspaces((srcNodeSpace >> dstNodeSpace) >> indexsetSpace)); // { recv[] -> (writeNode[cluster], readNode[cluster], field[indexset]) }
        auto combuf = pm->newCommunicationBuffer(fty, comrel);
        combuf->doLayoutMapping();

        // { readNode[cluster] }: send_wait
        {
          auto sendwaitWhere = remoteTransfers.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { readNode[cluster] -> writeNode[cluster] }
          auto sendwaitScatter = relativeScatter2(remoteTransfers.reorderSubspaces(writeDomainSpace, dstNodeSpace) /* { writeStmt[domain] -> readNode[cluster] } */, writeScatter, -1); // { readNode[cluster] -> scattering[scatter] }
          auto sendwaitEditor = editor.createStmt(sendwaitWhere.getDomain(), sendwaitScatter.copy(), sendwaitWhere.copy(), "sendwait");
          auto sendwaitStmt = sendwaitEditor.getStmt();
          auto sendwaitStmtCtx = getScopStmtContext(sendwaitStmt);
          auto sendwaitCodegen = sendwaitStmtCtx->makeCodegen();

          auto sendwaitCurrentChunk = dstNodeSpace.mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { readNode[cluster] -> epilogue[] }
          auto sendwaitCurrentNode = sendwaitStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);
          auto sendwaitCurrentDst = sendwaitEditor.getCurrentIteration().setOutTupleId(dstNodeId);

          auto sendbufPtrPtr = combuf->codegenSendbufPtrPtr(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);
          auto bufptr = combuf->codegenSendWait(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);
          sendwaitCodegen.getIRBuilder().CreateStore(bufptr, sendbufPtrPtr);

          sendwaitCodegen.markBlock((Twine("output remote send_wait '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }

        // { writeStmt[domain], dstNode[cluster] }: writeflow
        {
          auto writeflowWhere = remoteTransfers.reorderSubspaces(writeDomainSpace >> dstNodeSpace, srcNodeSpace);
          auto writeflowScatter = writeScatter.wrap().reorganizeSubspaces(writeDomainSpace >> dstNodeSpace, writeScatter.getRangeSpace());
          auto writeflowEditor = writeEditor.createStmt(writeflowWhere.getDomain(), writeflowScatter.copy(), writeflowWhere, "writeflow");
          //writeEditor.removeInstances(writeflowWhere);
          auto writeflowStmt = writeflowEditor.getStmt();
          auto writeflowStmtCtx = getScopStmtContext(writeflowStmt);
          auto writeflowCodegen = writeflowStmtCtx->makeCodegen();

          auto writeStore = writeStmtCtx->getStoreAccessor();
          auto writeAcc = writeStmtCtx->getAccess();
          auto writeAccessedAff = writeStmtCtx->getAccessed(); // { writeStmt[domain] -> field[indexset] } // TODO: // { writeStmt[domain],dstNode[cluster] -> field[indexset] }

          auto writeflowCurrent = writeflowEditor.getCurrentIteration(); // { writeStmt[domain], dstNode[cluster] -> writeStmt[domain], dstNode[cluster] }
          auto writeflowCurrentDomain = writeflowCurrent.sublist(writeDomainSpace).setOutTupleId(writeDomainTuple);
          auto writeflowCurrentNode = writeflowStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);
          auto writeflowCurrentIndex = writeAccessedAff.pullback(writeflowCurrentDomain).toPwMultiAff();
          //auto writeflowCurrentDst = fieldHome.pullback(writeflowCurrentIndex);
          auto writeflowCurrentDst = writeflowCurrent.sublist(dstNodeSpace);
          auto writeflowCurrentChunk = writeDomainSpace.mapsTo(epilogueDomainSpace).createZeroMultiAff(); // {  writeStmt[domain] -> prologue[] }

          //auto writeflowPtr = combuf->codegenPtrToSendBuf(writeflowCodegen, writeflowCurrentChunk, writeflowCurrentNode, writeflowCurrentDst, writeflowCurrentIndex);
          //auto writeflowStore = writeflowCodegen.getIRBuilder().CreateStore(writeAcc.getWrittenValueRegister(), writeflowPtr);

          auto sendbufPtr = combuf->codegenSendbufPtr(writeflowCodegen, writeflowCurrentChunk, writeflowCurrentNode, writeflowCurrentDst, writeflowCurrentIndex);
          auto stackPtr = writeStmtCtx->getAccessStackStorageAnnPtr().pullbackDomain(writeflowCurrentDomain);
          writeflowCodegen.codegenAssign(sendbufPtr, stackPtr);

          writeflowCodegen.markBlock((Twine("output remote write '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }

        // { dstNode[cluster] }: send
        {
          auto sendWhere = remoteTransfers.reorderSubspaces(dstNodeSpace, srcNodeSpace); // { dstNode[cluster] -> srcNode[cluster] }
          auto sendScatter = relativeScatter2(remoteTransfers.reorderSubspaces(writeDomainSpace, dstNodeSpace), writeScatter, +1);
          auto sendEditor = editor.createStmt(sendWhere.getDomain(), sendScatter.copy(), sendWhere.copy(), "send");
          auto sendStmt = sendEditor.getStmt();
          auto sendStmtCtx = getScopStmtContext(sendStmt);
          auto sendCodegen = sendStmtCtx->makeCodegen();

          auto sendCurrent = sendEditor.getCurrentIteration();
          auto sendCurrentChunk = sendCurrent.getDomainSpace().mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { send[domain] -> epilogue[] }
          auto sendCurrentDst = sendCurrent.setOutTupleId(dstNodeId);
          auto sendCurrentNode = sendStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);

          combuf->codegenSend(sendCodegen, sendCurrentChunk, sendCurrentNode, sendCurrentDst);

          sendCodegen.markBlock((Twine("output remote send '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }


        // { srcNode[cluster] }: recv_wait
        {
          auto recvwaitWhere = remoteTransfers.reorderSubspaces(srcNodeSpace, dstNodeSpace); // { src[cluster] -> dst[cluster] }
          auto recvwaitScatter = constantScatter(srcNodeSpace.mapsTo(afterScopScatter.getDomainSpace()).createZeroMultiAff(), afterScopScatter, 0); // { srcNode[cluster] -> scattering[scatter] }
          auto recvwaitEditor = editor.createStmt(recvwaitWhere.getDomain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recvwait");
          auto recvwaitStmt = recvwaitEditor.getStmt();
          auto recvwaitStmtCtx = getScopStmtContext(recvwaitStmt);
          auto recvwaitCodegen = recvwaitStmtCtx->makeCodegen();

          auto recvwaitCurrent = recvwaitEditor.getCurrentIteration(); // { srcNode[cluster] -> [...] }
          auto recvwaitCurrentDomain = recvwaitCurrent.castRange(srcNodeSpace);
          auto recvwaitCurrentSrc = recvwaitCurrent.setOutTupleId(srcNodeId);
          auto recvwaitCurrentNode = recvwaitStmtCtx->getClusterMultiAff().setOutTupleId(dstNodeId);
          auto recvwaitCurrentChunk = recvwaitCurrent.getDomainSpace().mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { send[domain] -> epilogue[] }

          //combuf->codegenRecvWait(recvwaitCodegen, recvwaitCurrentSrc, recvwaitCurrentSrc, recvwaitCurrentNode);

          auto bufPtr = combuf->codegenRecvWait(recvwaitCodegen, recvwaitCurrentChunk, recvwaitCurrentSrc, recvwaitCurrentNode);
          auto sendbufPtrPtr = combuf->codegenRecvbufPtrPtr(recvwaitCodegen, recvwaitCurrentChunk, recvwaitCurrentSrc, recvwaitCurrentNode);
          recvwaitCodegen.getIRBuilder().CreateStore(bufPtr, sendbufPtrPtr);

          recvwaitCodegen.markBlock((Twine("output remote recv_wait '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }

        // { field[indexset] }: writeback/read
        {
          auto writebackWhere = remoteTransfers.reorderSubspaces(indexsetSpace, dstNodeSpace); // { field[indexset] -> dstNode[cluster] }
          auto writebackDomain = writebackWhere.getDomain(); // { field[indexset] }
          auto writebackScatter = constantScatter(indexsetSpace.mapsTo(afterScopScatter.getDomainSpace()).createZeroMultiAff(), afterScopScatter, +1); // { field[indexset] -> scattering[scatter] }
          auto writebackEditor = editor.createStmt(writebackDomain.copy(), writebackScatter.copy(), writebackWhere.copy(), "writeback");
          auto writebackStmt = writebackEditor.getStmt();
          auto writebackStmtCtx = getScopStmtContext(writebackStmt);
          auto writebackCodegen = writebackStmtCtx->makeCodegen();

          auto writebackCurrentIndex = writebackEditor.getCurrentIteration().setOutTupleId(indexsetTupleId);
          auto writebackCurrentChunk = indexsetSpace.mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { field[indexset] -> epilogue[] }
          auto writebackCurrentNode = writebackStmtCtx->getClusterMultiAff().setOutTupleId(dstNodeId); // { field[indexset] -> dstNode[cluster] }
          //auto writebackCurrentSrc = selectedWrite.sublist(srcNodeSpace); // { field[indexset] -> srcNode[cluster] }

          //remoteSrcNodeUnique; // { (field[indexset], dstNode[cluster]) -> (srcNode[cluster], writeStmt[domain]) }
          auto writebackSrcNode = remoteSrcNodeUnique.sublist(srcNodeSpace); // { (field[indexset], dstNode[cluster]) -> srcNode[cluster] }
          auto translateexpand = rangeProduct(indexsetSpace.mapsToItself().createIdentityMultiAff(), writebackCurrentNode); // { field[indexset] -> (field[indexset], dstNode[cluster]) }
          auto writebackCurrentSrc = writebackSrcNode.pullback(translateexpand); // { field[indexset] -> srcNode[cluster] }
          //auto tmp = writebackCurrentNode.embedIntoDomain(writebackSrcNode.getDomainSpace()); // { (field[indexset], dstNode[cluster]) -> (field[indexset], dstNode[cluster])  }


          //auto tmp = writebackCurrentNode.embedIntoDomain(writebackCurrentWriteDomainSrcNode.getDomainSpace()); // { field[indexset] -> srcNode[cluster] }
          //auto writebackCurrentWriteDomainSrcNode = 
           // writebackCurrentNode.pullback(remoteSrcNodeUnique );
            
          //auto writebackLocalPtr = combuf->codegenPtrToRecvBuf(writebackCodegen, writebackCurrentChunk, writebackCurrentNode, writebackCurrentSrc, writebackCurrentIndex);
          //auto writebackVal = writebackCodegen.getIRBuilder().CreateLoad(writebackLocalPtr, "loadfromcombuf");
          //writebackCodegen.codegenStoreLocal(writebackVal, fvar, writebackCurrentNode, writebackCurrentIndex);

          auto localbufptr = writebackCodegen.codegenFieldLocalPtr(fvar, writebackCurrentNode, writebackCurrentIndex);
          auto recvbufPtr = combuf->codegenRecvbufPtr(writebackCodegen, writebackCurrentChunk, writebackCurrentSrc, writebackCurrentNode, writebackCurrentIndex);
          writebackCodegen.codegenAssign(localbufptr, recvbufPtr);

          writebackCodegen.markBlock((Twine("output remote writeback '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }

        // { srcNode[cluster] }: recv
        {
          auto recvWhere = remoteTransfers.reorderSubspaces(srcNodeSpace, dstNodeSpace); // { src[cluster] -> dst[cluster] }
          auto recvScatter = constantScatter(srcNodeSpace.mapsTo(afterScopScatter.getDomainSpace()).createZeroMultiAff(), afterScopScatter, +2);
          auto recvEditor = editor.createStmt(recvWhere.getDomain(), recvScatter.copy(), recvWhere.copy(), "recv");
          auto recvStmt = recvEditor.getStmt();
          auto recvStmtCtx = getScopStmtContext(recvStmt);
          auto recvCodegen = recvStmtCtx->makeCodegen();

          auto recvCurrent = recvEditor.getCurrentIteration(); // { srcNode[cluster] -> [...] }
          auto recvCurrentSrc = recvCurrent.setOutTupleId(srcNodeId);
          auto recvCurrentNode = recvStmtCtx->getClusterMultiAff().setOutTupleId(dstNodeId);
          auto recvCurrentChunk = recvCurrent.getDomainSpace().mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { field[indexset] -> epilogue[] }

          combuf->codegenRecv(recvCodegen, recvCurrentChunk, recvCurrentSrc, recvCurrentNode);

          recvCodegen.markBlock((Twine("output remote recv '") + writeDomain.getSetTupleName() + "'" + fvarname).str());
        }
      } //  if (!remoteTransfers.isEmpty())
    } //  void genOutputCommunication(const isl::Set &outSet/*)


#pragma endregion


#pragma region Polly
    void pollyOptimize() {

      switch (Optimizer) {
#ifdef SCOPLIB_FOUND
      case OPTIMIZER_POCC:
        runPass(polly::createPoccPass());
        break;
#endif
#ifdef PLUTO_FOUND
      case OPTIMIZER_PLUTO:
        runPass(polly::createPlutoOptimizerPass());
        break;
#endif
      case OPTIMIZER_ISL:
        runPass(polly::createIslScheduleOptimizerPass());
        break;
      case OPTIMIZER_NONE:
        break;
      }
    }

    void pollyCodegen() {
      switch (CodeGenerator) {
#ifdef CLOOG_FOUND
      case CODEGEN_CLOOG:
        runPass(polly::createCodeGenerationPass());
        if (PollyVectorizerChoice == VECTORIZER_BB) {
          VectorizeConfig C;
          C.FastDep = true;
          runPass(createBBVectorizePass(C));
        }
        break;
#endif
      case CODEGEN_ISL:
        runPass(polly::createIslCodeGenerationPass());
        break;
      case CODEGEN_NONE:
        break;
      }
    }
#pragma endregion

    llvm::Pass *asPass() override {
      return this;
    }

    bool runOnScop(Scop &S) override {
      return false;
    }


  private:
    SCEVExpander scevCodegen;
    //std::map<const isl_id *, Value *> paramToValue;

    isl::Space getParamsSpace() override {
      //TODO: All parameters should be collected in an initialization phase, thereafter nothing needs to be realigned
      scop->realignParams();
      return enwrap(scop->getParamSpace()).getParamsSpace();
    }

    const SmallVector<const SCEV *, 8> &getParamSCEVs() override {
      return scop->getParams();
    }

    /// SCEVExpander remembers which SCEVs it already expanded in order to reuse them; this is why it is here
    llvm::Value *codegenScev(const llvm::SCEV *scev, llvm::Instruction *insertBefore) override {
      return scevCodegen.expandCodeFor(scev, nullptr, insertBefore);
    }


    /// ScalarEvolution remembers its SCEVs
    const llvm::SCEV *scevForValue(llvm::Value *value) override {
      if (!SE) {
        SE = pm->findOrRunAnalysis<ScalarEvolution>(&scop->getRegion());
      }
      assert(SE);
      auto result = SE->getSCEV(value);
      return result;
    }


    // We need to always return the same id for same loop since we use that Id as key in maps
    // Usually ISL would remember itself always created ids, but we use the key as weak reference so ISL generates a new id after refcount drops to 0 
    llvm::DenseMap<const Loop *, isl::Id> storedDomIds;
    isl::Id getIdForLoop(const Loop *loop) override {
      auto &id = storedDomIds[loop];
      if (id.isValid())
        return id;

      auto depth = loop->getLoopDepth();
      loop->getCanonicalInductionVariable();
      auto indvar = loop->getCanonicalInductionVariable();
      id = islctx->createId("dom" + Twine(depth - 1), loop);
      return id;
    }


    /// Here, polly::Scop is supposed to remember (or create) them
    isl::Id idForSCEV(const llvm::SCEV *scev) override {
      auto id = scop->getIdForParam(scev);
      if (!id) {
        // id is not yet known to polly::Scop
        // We can assume it is a loop variable and therefore representing a domain dimension. But which one?
        auto recScev = cast<SCEVAddRecExpr>(scev);
        auto loop = recScev->getLoop();

        // We create an id for each loop induction variable
        return getIdForLoop(loop);

        //scop->addParams(std::vector<const SCEV *>(1, scev));
        //id = scop->getIdForParam(scev);
      }
      assert(id);
      return enwrap(id);
    }


  public:
    void dump() const override {
      dbgs() << "ScopProcessor:\n";
      if (scop)
        scop->dump();
    }


    void validate() const override {
#ifdef NDEBUG
      return;
#endif

      for (auto stmt : *scop) {
        auto stmtCtx = getScopStmtContext(stmt);
        stmtCtx->validate();
      }
    }


  private:
    isl::MultiAff currentNodeCoord;

    void prepareCurrentNodeCoordinate() {
      //TODO: Use the one in MollyFunctionProcessor
      if (currentNodeCoord.isValid())
        return;

      auto clusterSpace = pm->getClusterConfig()->getClusterSpace();
      auto nDims = clusterSpace.getSetDimCount();
      std::vector<const SCEV *> NewParameters;
      NewParameters.reserve(nDims);
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto scev = getClusterCoordinate(i);
        NewParameters.push_back(scev);
      }
      scop->addParams(NewParameters);

      auto currentNodeCoordSpace = isl::Space::createMapFromDomainAndRange(0, clusterSpace);
      currentNodeCoordSpace.alignParams_inplace(enwrap(scop->getParamSpace()));
      currentNodeCoord = currentNodeCoordSpace.createZeroMultiAff();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto scev = NewParameters[i];
        auto id = enwrap(scop->getIdForParam(scev));
        currentNodeCoord.setAff_inplace(i, currentNodeCoordSpace.getDomainSpace().createAffOnParam(id));
      }
    }


    isl::MultiAff getCurrentNodeCoordinate() override {
      prepareCurrentNodeCoordinate();
      return currentNodeCoord;
    }


    polly::ScopStmt *getStmtForBlock(llvm::BasicBlock *bb) override {
      return scop->getScopStmtFor(bb);
    }


    DenseMap<FieldVariable *, AllocaInst *> localPtrs;
    llvm::AllocaInst *codegenLocalBufferPtrOf(FieldVariable *fvar) override {
      auto &result = localPtrs[fvar];
      if (result)
        return result;

      auto entry = &func->getEntryBlock();
      MollyCodeGenerator codegen(entry, entry->getTerminator()/*must be after llvm.molly.cluster.current.coordinate*/, this);
      auto &builder = codegen.getIRBuilder();
      result = builder.CreateAlloca(fvar->getEltPtrType(), nullptr, "scoplocalptr");
      auto localPtr = codegen.callLocalPtr(fvar);
      builder.CreateStore(localPtr, result);
      return result;
    }


    DenseMap<CommunicationBuffer *, AllocaInst *> sendbufPtrs;
    llvm::AllocaInst *codegenSendbufPtrsOf(CommunicationBuffer *combuf) override { // Returns a jagged array result[bufidx][eltidx]; result[bufidx] are uninitialized, to be filled with the return value of llvm.molly.sendbuf.send_wait
      auto &result = sendbufPtrs[combuf];
      if (result)
        return result;

      auto entry = &func->getEntryBlock();
      //MollyCodeGenerator codegen(entry, entry->getTerminator()/*must be after llvm.molly.cluster.current.coordinate*/, this, getParams());
      auto codegen = getFunctionProcessor()->makeCodegen(entry->getTerminator()/*must be after llvm.molly.cluster.current.coordinate*/);
      auto self = getFunctionProcessor()->getCurrentNodeCoordinate(); //  getCurrentNodeCoordinate();
      auto numDests = combuf->codegenNumDests(codegen, self);

      result = codegen.getIRBuilder().CreateAlloca(combuf->getEltPtrType(), numDests, "scopsendbufptrs");
      return result;
    }


    DenseMap<CommunicationBuffer *, AllocaInst *> recvbufPtrs;
    llvm::AllocaInst *codegenRecvbufPtrsOf(CommunicationBuffer *combuf) override {
      auto &result = recvbufPtrs[combuf];
      if (result)
        return result;

      auto entry = &func->getEntryBlock();
      //MollyCodeGenerator codegen(entry, entry->getTerminator()/*must be after llvm.molly.cluster.current.coordinate*/, this);
      auto codegen = getFunctionProcessor()->makeCodegen(entry->getTerminator()/*must be after llvm.molly.cluster.current.coordinate*/);
      auto self = getFunctionProcessor()->getCurrentNodeCoordinate();
      auto numDests = combuf->codegenNumSrcs(codegen, self);

      result = codegen.getIRBuilder().CreateAlloca(combuf->getEltPtrType(), numDests, "scoprecvbufptrs");
      return result;
    }
  }; // class MollyScopContextImpl


  isl::UnionMap MollyScopContextImpl::overviewWhere() {
    isl::UnionMap result = islctx->createEmptyUnionMap();

    for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt != endStmt; ++itStmt) {
      auto stmtCtx = getScopStmtContext(*itStmt);
      auto where = stmtCtx->getWhere();
      result.unite_inplace(where);
    }

    return result;
  }

} // namespace


char MollyScopContextImpl::ID = '\0';
MollyScopProcessor *MollyScopProcessor::create(MollyPassManager *pm, polly::Scop *scop) {
  return new MollyScopContextImpl(pm, scop);
}
