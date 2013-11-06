#include "MollyScopProcessor.h"

#include <polly/ScopInfo.h>
#include "islpp/Ctx.h"
#include "MollyPassManager.h"
#include "MollyFieldAccess.h"
#include "MollyUtils.h"
#include "MollyFunctionProcessor.h"
#include <llvm/Analysis/ScalarEvolution.h>
#include "ClusterConfig.h"
#include "FieldType.h"
#include "islpp/Map.h"
#include "islpp/Set.h"
#include "ScopUtils.h"
#include "islpp/UnionMap.h"
#include "ScopEditor.h"
#include "MollyIntrinsics.h"
#include "LLVMfwd.h"
#include <llvm/IR/IRBuilder.h>
#include "CommunicationBuffer.h"
#include "FieldVariable.h"
#include <llvm/IR/Intrinsics.h>
#include "islpp/Point.h"
#include "polly/RegisterPasses.h"
#include "polly/LinkAllPasses.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "Codegen.h"
#include "MollyScopStmtProcessor.h"
#include "polly/ScopPass.h"
#include "MollyRegionProcessor.h"
#include <llvm/Analysis/LoopInfo.h>
#include "islpp/DimRange.h"
#include "islpp/MultiAff.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace {
  bool isStmtLevel(unsigned i) { return i%2==0; }
  bool isLoopLevel(unsigned i) { return i%2==1; }


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
    auto naturallyBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain <_scatter model }
    auto naturallyAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain >_scatter model }


    isl::PwMultiAff extreme; // { subdomain[domain] -> model[scatter]  }
    isl::Map order; // { subdomain[scatter] -> model[scatter] }
    if (relative < 0) {
      extreme = mustBeRelativeToScatter.lexminPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexLtMap(); // { A[scatter] -> B[scatter] | A <_lex B  }
    } else {
      extreme = mustBeRelativeToScatter.lexmaxPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexGtMap(); // { A[scatter] -> B[scatter] | A >_lex B  }
    }

    auto lastDim = nScatterDims-1;
    auto referenceScatter = extreme.setPwAff(lastDim, extreme.getPwAff(lastDim) + relative); // { subdomain[domain] -> model[scatter] }
    auto referenceBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());
    auto referenceAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());

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


  isl::MultiAff constantScatter(isl::MultiAff translate/* { result[domain] -> model[domain] } */, isl::MultiAff modelScatter /* { model[domain] -> scattering[scatter] } */, int relative) {
    auto result = modelScatter.pullback(translate);
    auto nScatterDims = result.getOutDimCount();
    result.setAff(nScatterDims-1, result.getAff(nScatterDims-1)+relative);
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

    auto nScatterDims = modelScatter.getOutDimCount();
    auto subdomain = mustBeRelativeTo.getDomain();
    auto mustBeRelativeToScatter = mustBeRelativeTo.applyRange(modelScatter); // { subdomain[domain] -> model[scatter] }
    auto scatterSpace = modelScatter.getRangeSpace();
    auto universeDomain = mustBeRelativeTo.getRange(); // { [domain] }
    auto reverseModelScatter = modelScatter.reverse().intersectRange(universeDomain); // { scattering[scatter] -> [domain] }

    auto isBefore = scatterSpace.lexLtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    auto isAfter = scatterSpace.lexGtMap().applyDomain(modelScatter.reverse()).applyRange(modelScatter.reverse()); // { [domain] -> [domain] } 
    auto naturallyBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain <_scatter model }
    auto naturallyAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(reverseModelScatter); // { subdomain[domain] -> [domain] | subdomain >_scatter model }


    isl::PwMultiAff extreme; // { subdomain[domain] -> model[scatter]  }
    isl::Map order; // { subdomain[scatter] -> model[scatter] }
    if (relative < 0) {
      extreme = mustBeRelativeToScatter.lexminPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexLtMap(); // { A[scatter] -> B[scatter] | A <_lex B  }
    } else {
      extreme = mustBeRelativeToScatter.lexmaxPwMultiAff(); // { subdomain[domain] -> model[scatter] }
      order = scatterSpace.lexGtMap(); // { A[scatter] -> B[scatter] | A >_lex B  }
    }

    auto lastDim = nScatterDims-1;
    auto referenceScatter = extreme.setPwAff(lastDim, extreme.getPwAff(lastDim) + relative); // { subdomain[domain] -> model[scatter] }
    auto referenceBefore = scatterSpace.lexLtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());
    auto referenceAfter = scatterSpace.lexGtMap().applyDomain(reverseModelScatter).applyRange(referenceScatter.reverse());

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

    bool changedScop ;
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


    const SCEV *getClusterCoordinate(unsigned i) LLVM_OVERRIDE {
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
      for (auto i = nClusterDims-nClusterDims; i < nClusterDims; i+=1) {
        result.push_back(getClusterCoordinate(i));
      }
      return result;
    }


  protected:
    template<typename Analysis> 
    Analysis *findOrRunAnalysis() {
      return pm->findOrRunAnalysis<Analysis>(func, &scop->getRegion());
    }

    Region *getRegion() LLVM_OVERRIDE {
      return &scop->getRegion();
    }

    llvm::Function *getParentFunction() LLVM_OVERRIDE {
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
      for (auto i = nContextParams-nContextParams; i < nContextParams; i+=1) {
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


#pragma region Scop Distribution
    void processFieldAccess(MollyFieldAccess &acc, isl::Map &executeWhereWrite, isl::Map &executeWhereRead) {
      auto fieldVar = acc.getFieldVariable();
      auto fieldTy = acc.getFieldType();
      auto stmt = acc.getPollyScopStmt();
      auto scop = stmt->getParent();

      auto rel = acc.getAffineAccess(SE); /* iteration coord -> field coord */
      auto home = fieldTy->getHomeAff();  /* field coord -> cluster coord */
      auto relMap = rel.toMap().setOutTupleId(fieldTy->getIndexsetTuple());
      auto it2rank = relMap.applyRange(home.toMap());

      if (acc.isRead()) {
        executeWhereRead = unite(executeWhereRead.subtractDomain(it2rank.getDomain()), it2rank);
      }

      if (acc.isWrite()) {
        executeWhereWrite = unite(executeWhereWrite.subtractDomain(it2rank.getDomain()), it2rank);
      }
    }


  protected:
    void distributeScopStmt(ScopStmt *stmt) {
      auto itDomain = getIterationDomain(stmt);
      auto itSpace = itDomain.getSpace();

      auto clusterConf =  pm->getClusterConfig();
      auto executeWhereWrite = islctx->createEmptyMap(itSpace, clusterConf->getClusterSpace());
      auto executeWhereRead = executeWhereWrite.copy();
      auto executeEverywhere = islctx->createAlltoallMap(itDomain, clusterConf->getClusterShape());
      auto executeMaster = islctx->createAlltoallMap(itDomain, clusterConf->getMasterRank().toMap().getRange());

      for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) {
        auto memacc = *itAcc;
        auto acc = pm->getFieldAccess(memacc);
        if (!acc.isValid())
          continue;

        processFieldAccess(acc, executeWhereWrite, executeWhereRead);
      }

      auto result = executeEverywhere;
      result = unite(result.subtractDomain(executeWhereRead.getDomain()), executeWhereRead);
      result = unite(result.subtractDomain(executeWhereWrite.getDomain()), executeWhereWrite);

      result.setOutTupleId_inplace(pm->getClusterConfig()->getClusterTuple());
      stmt->setWhereMap(result.take());
      // FIXME: We must ensure that depended instructions are executed on the same node. Execute on multiple nodes if necessary

      modifiedScop();
    }


  public:
    void computeScopDistibution() {
      SE = pm->findOrRunAnalysis<ScalarEvolution>(&scop->getRegion());

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        distributeScopStmt(stmt);
      }
    }
#pragma endregion


#pragma region Scop CommGen
    static void spreadScatteringsByInsertingDims(Scop *scop) {
      auto nOrigScatterDims = scop->getScatterDim();
      auto origScatterSpace = enwrap(scop->getScatteringSpace());
      auto islctx = enwrap(scop->getIslCtx());
      auto scatterTuple = getScatterTuple(scop);

      auto nScatterDims = nOrigScatterDims*2 + 1;
      auto scatterSpace = origScatterSpace.addDims(isl_dim_out, nScatterDims - nOrigScatterDims).setSetTupleId(scatterTuple);

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto scatter = enwrap(stmt->getScattering());
        auto nOldDimCount = scatter.getOutDimCount();

        scatter.insertDims_inplace(isl_dim_out, 0, 1);
        scatter.fix_inplace(isl_dim_out, 0, 0);
        for (auto i = nOldDimCount-nOldDimCount; i < nOldDimCount; i+=1) {
          scatter.insertDims_inplace(isl_dim_out, 2+2*i, 1);
          scatter.fix_inplace(isl_dim_out, 2+2*i, 0);
        }
        for (auto i = scatter.getOutDimCount(); i < nScatterDims; i+=1) {
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

      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto scattering = getScattering(stmt);
        auto space = scattering.getSpace();
        auto nParam = space.getParamDimCount();
        auto nIn = space.getInDimCount();
        auto nOut = space.getOutDimCount();
        auto mapSpace = isl::Space::createMapFromDomainAndRange(space.range(), scatterSpace);
        auto map = mapSpace.universeBasicMap();

        for (auto i = 1u; i < nOut; i+=2) {
          map.equate_inplace(isl_dim_in, i, isl_dim_out, i);
        }
        for (auto i = nOut-nOut; i < nOut; i+=2) {
          auto c = mapSpace.createEqualityConstraint();
          c.setCoefficient_inplace(isl_dim_in, i, -multiplier);
          c.setCoefficient_inplace(isl_dim_out, i, 1);
          map.addConstraint_inplace(c);
        }
        for (auto i = nOut; i < nScatterDims; i+=1) {
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

    bool contains(Instruction *instr) {
      return getRegion()->contains(instr);
    }

  public:
    void genCommunication() {
      auto funcName = func->getName();
      DEBUG(llvm::dbgs() << "run ScopFieldCodeGen on " << scop->getNameStr() << " in func " << funcName << "\n");
      if (funcName == "sink") {
        int a = 0;
      }

      //auto func = scop->getRegion().getEntry()->getParent();
      auto clusterConf = pm->getClusterConfig();
      auto &llvmContext = func->getContext();
      auto module = func->getParent();
      auto scatterTuple = getScatterTuple(scop);
      auto clusterTuple = clusterConf->getClusterTuple();
      auto nodeSpace = clusterConf->getClusterSpace();
      auto nodes = clusterConf->getClusterShape();
      auto nClusterDims = nodeSpace.getSetDimCount();

      // Make some space between the stmts in which we can insert our communication
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

      DenseMap<const isl_id*,FieldType*> tupleToFty; //TODO: This is not per scop, so should be moved to PassManager
      DenseMap<const isl_id*,FieldVariable*> tupleToFvar;

      auto paramSpace = isl::enwrap(scop->getParamSpace());

      auto emptyMap = paramSpace.createEmptyUnionMap();
      auto emptySet = paramSpace.createEmptyUnionSet();
      auto readAccesses = emptyMap; /* { stmt[iteration] -> access[indexset] } */
      auto writeAccesses = emptyMap; /* { stmt[iteration] -> access[indexset] } */
      auto schedule = emptyMap; /* { stmt[iteration] -> scattering[scatter] } */

      //auto fieldReadAccesses = emptyMap; // { stmt[domain] -> field[indexset] }
      //auto fieldWriteAccesses = emptyMap; // { stmt[domain] -> field[indexset] }
      auto nonfieldReadAccesses = emptyMap; // { stmt[domain] -> variable[] }
      auto nonfieldWriteAccesses = emptyMap; // { stmt[domain] -> variable[] }
      auto nonfieldUsedOutside = emptySet; // { variable[indexset] }

      auto readFieldDomains = emptySet; // { [domain] }
      auto writeFieldDomain = emptySet; // { [domain] }

      // Collect accesses
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto stmtCtx = getScopStmtContext(stmt);

        auto domainTuple = getDomainTuple(stmt);
        auto domain = getIterationDomain(stmt); /* { stmt[domain] } */
        assert(domain.getSpace().matchesSetSpace(domainTuple));

        auto scattering = getScattering(stmt); /* { stmt[domain] -> scattering[scatter] }  */
        assert(scattering.getSpace().matchesMapSpace(domainTuple, scatterTuple));
        scattering.intersectDomain_inplace(domain);
        schedule.addMap_inplace(scattering);

        // Non-field accesses
        for (auto accIt = stmt->memacc_begin(), accEnd = stmt->memacc_end(); accIt!=accEnd; accIt+=1) {
          auto memacc = *accIt;
          auto instr = const_cast<Instruction *>(memacc->getAccessInstruction());
          auto facc = FieldAccess::fromAccessInstruction(instr);
          if (facc.isValid())
            continue; // Will be handled later

          llvm::Value *ptr;
          auto rel = enwrap(memacc->getAccessRelation()); // { [domain] -> [indexset] }
          auto indexsetSpace = rel.getRangeSpace();
          auto indexsetTupleId = indexsetSpace.getSetTupleId(); // TODO: Check uniqueness
          if (auto loadInstr = dyn_cast<LoadInst>(instr)) {
            ptr = loadInstr->getPointerOperand();
            nonfieldReadAccesses.unite_inplace(rel);
          } else if (auto storeInstr = dyn_cast<StoreInst>(instr)) {
            ptr = storeInstr->getPointerOperand();
            nonfieldWriteAccesses.unite_inplace(rel);
          } else {
            llvm_unreachable("What other type of memory access are there?");
          }

          // See if it is still used after leaving the scop
          bool usedOutside = false;
          for (auto use = ptr->use_begin(), end=ptr->use_end(); use!=end; ++use) {
            auto user =  cast<Instruction>(*use);
            auto userBB = user->getParent();

            if (this->contains(user))
              continue;

            // TODO: Used before or after the scop?
            // TOOD: LoadInst or StoreInst or pointer arithmetic? TODO: Check if is actually read outside the SCoP
            usedOutside = true;
            break;
          }

          if (usedOutside) {
            nonfieldUsedOutside.unite_inplace(rel.range());
          }
        }

        // Field accesses
        auto facc = getFieldAccess(stmt);
        if (!facc.isValid())
          continue; // Does not contain a field access

        auto fvar = stmtCtx->getFieldVariable();
        auto accessTuple = fvar->getTupleId();
        auto accessSpace = fvar->getAccessSpace();
        assert(!tupleToFvar.count(accessTuple.keep()) || tupleToFvar[accessTuple.keep()]==fvar);
        tupleToFvar[accessTuple.keep()] = fvar;

        auto fty = stmtCtx->getFieldType();
        auto indexsetTuple = fty->getIndexsetTuple();
        assert(!tupleToFty.count(indexsetTuple.keep()) || tupleToFty[indexsetTuple.keep()]==fty);
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
          writeAccesses.addMap_inplace(accessRel);
          writeFieldDomain.unite_inplace(accessRel.domain());
        }
      }

      // To find the data that needs to be written back after the scop has been executed, we add an artificial stmt that reads all the data after everything has been executed
      epilogueId = islctx->createId("epilogue");
      auto epilogueDomainSpace = islctx->createSetSpace(0,0).setSetTupleId(epilogueId);
      auto scatterRangeSpace = getScatteringSpace();
      assert(scatterRangeSpace.isSetSpace());
      auto scatterId = scatterRangeSpace.getSetTupleId();
      auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();
      auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);

      auto prologueId = islctx->createId("prologue");
      auto prologueDomainSpace = islctx->createSetSpace(0,0).setSetTupleId(prologueId);
      auto prologueScatterSpace = isl::Space::createMapFromDomainAndRange(prologueDomainSpace, scatterRangeSpace);

      auto allScatters = range(schedule);
      //assert(allScatters.nSet()==1);
      auto scatterRange = allScatters.extractSet(scatterRangeSpace); 
      assert(scatterRange.isValid());
      auto nScatterDims = scatterRange.getDimCount();

      assert(!scatterRange.isEmpty());
      auto max = affineHull(alltoall(epilogueDomainSpace.universeBasicSet(), scatterRange)).dimMax(0) + 1;
      //max.setInTupleId_inplace(epilogueId);
      this->afterScopScatter = epilogueScatterSpace.createZeroMultiAff();
      this->afterScopScatter.setAff_inplace(0, max);

      isl::Set epilogueDomain = epilogueDomainSpace.universeSet();
      auto epilogieMapToZero = epilogueScatterSpace.createUniverseBasicMap();
      for (auto d = 1u; d < nScatterRangeDims; d+=1) {
        epilogieMapToZero.addConstraint_inplace(epilogueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
      }

      assert(!scatterRange.isEmpty());
      auto min = affineHull(alltoall(prologueDomainSpace.universeBasicSet(), scatterRange)).dimMin(0) - 1; /* { [1] } */
      // min.setInTupleId_inplace(prologueId); /* { scattering[nScatterDims] -> [1] } */
      this->beforeScopScatter = prologueScatterSpace.createZeroMultiAff();
      this->beforeScopScatter.setAff_inplace(0, min);

      //min.addDims_inplace(isl_dim_in, nScatterDims);
      //auto beforeScopScatter = scatterRangeSpace.mapsTo(scatterRangeSpace).createZeroMultiPwAff(); /* { scattering[nScatterDims] -> scattering[nScatterDims] } */
      //beforeScopScatter.setPwAff_inplace(0, min);
      beforeScopScatterRange = beforeScopScatter.toMap().getRange();


      auto afterBeforeScatter = beforeScopScatter.setAff(1, beforeScopScatter.getDomainSpace().createConstantAff(1));
      afterBeforeScatterRange = afterBeforeScatter.toMap().getRange();

      // Insert fake read access after scop
      auto epilogueScatter = max.toMap(); //scatterSpace.createMapFromAff(max);
      epilogueScatter.addDims_inplace(isl_dim_out, nScatterRangeDims - 1);
      //epilogueScatter.setTupleId(isl_dim_out, scatterRangeSpace.getTupleId(isl_dim_set));
      epilogueScatter.setInTupleId_inplace(epilogueId);
      epilogueScatter.setOutTupleId_inplace(scatterId);
      epilogueScatter.intersect_inplace(epilogieMapToZero);

      //auto epilogieScatterAsDomain = epilogueScatter.getRange().setTupleId(epilogueId);

      //auto allIterationDomains = domain(writeAccesses);
      auto allIndexsets = range(writeAccesses);
      //auto relateAll = isl::UnionMap::createFromDomainAndRange(
      auto epilogueTouchEverything = isl::UnionMap::createFromDomainAndRange(epilogueDomain, allIndexsets);

      // Find the data flows
      readAccesses.unite_inplace(epilogueTouchEverything);
      schedule.unite_inplace(epilogueScatter);
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

#ifndef NDEBUG
      isl::UnionMap mustFlow2;
      isl::UnionMap mustNosrc2;

      isl::simpleFlow(readAccesses, writeAccesses, schedule, &mustFlow2, &mustNosrc2);
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
        inputFlow.addSet_inplace(inpMap.domain());
      }

      for (auto dep : mustFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        auto readTuple = dep.getOutTupleId();
        if (readTuple == epilogueId) {
          auto out = dep.getDomain();
          outputFlow.addSet_inplace(out);
        } else {
          dataFlow.addMap_inplace(dep);
        }
      }
      isl::Approximation approx2;
      auto dataFlowClosure = dataFlow.transitiveClosure(approx2);


#pragma region Flow for nonfield accesses
      auto universeAccessEverything = emptyMap; // { epilogue[] -> [indexset] }
      for (auto access : nonfieldWriteAccesses.getMaps()) {//TODO: Only add those that are read outsidee the SCoP
        auto indexsetSpace = access.getRangeSpace();
        auto universe = indexsetSpace.universeBasicSet();
        auto accessEverything = alltoall(epilogueDomain, universe); // { epilogue[] -> nonfield[indexset] }
        universeAccessEverything.unite_inplace(accessEverything);
      }
      universeAccessEverything = alltoall(epilogueDomain, nonfieldUsedOutside);
      nonfieldReadAccesses.unite_inplace(universeAccessEverything);

      isl::UnionMap nonfieldMustFlow;
      isl::UnionMap nonfieldMayFlow;
      isl::UnionMap nonfieldMustNosrc;
      isl::UnionMap nonfieldMayNosrc;
      isl::computeFlow(nonfieldReadAccesses, nonfieldWriteAccesses, emptyMap, schedule, /*out*/nonfieldMustFlow, /*out*/nonfieldMayFlow, /*out*/nonfieldMustNosrc, /*out*/nonfieldMayNosrc);

      auto nonfieldInputFlow = nonfieldMustNosrc.domain(); // TODO: Remove epilogue from it
      auto nonfieldDataFlow = emptyMap;
      auto nonfieldOutputFlow = emptySet;

      for (auto dep : nonfieldMustFlow.getMaps()) {
        if (dep.getOutTupleId() == epilogueId) {
          nonfieldOutputFlow.addSet_inplace(dep.getDomain());
        } else {
          nonfieldDataFlow.addMap_inplace(dep);
        }
      }

      isl::Approximation approx;
      auto nonfieldDataFlowClosure = nonfieldDataFlow.transitiveClosure(approx); // { [domain] -> [domain] }
      auto nonfieldDataFlowClosureRev = nonfieldDataFlowClosure.reverse();
#pragma endregion

      /////////////////////////////////////////////////
      // Decide where to execute the statements

      /// List of statement instances that are not yet executed on at least one node
      auto notyetExecuted = emptySet;
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmtCtx = getScopStmtContext(*itStmt);

        auto domain = stmtCtx->getDomain();
        stmtCtx->setWhere(alltoall(domain.getSpace().universeBasicSet(), nodeSpace.emptySet())); // Reset to execute nowhere
        notyetExecuted.unite_inplace(domain);
      }


      // Stmts producing scalars that are used after the SCoP must be executed everywhere, since every node requires the up-to-date data; 
      for (auto outSet : nonfieldOutputFlow.getSets()) { // { [domain] }
        auto stmt = getScopStmtContext(outSet);

        stmt->addWhere(alltoall(outSet.getSpace().universeBasicSet(), nodeSpace.universeBasicSet()));
        notyetExecuted.substract_inplace(outSet);
      }
      localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);


      // "owner computes": execute statements that are valid after the exit of the SCoP at the value's home locations 

      for (auto output : outputFlow.getSets()) {
        //auto  outputnotyet =  intersect(output, notyetExecuted);
        auto stmtCtx = getScopStmtContext(output); 
        auto accessed = stmtCtx->getAccessRelation(); // { writeStmt[domain] -> field[indexset] }
        auto accessedButNotyetExecuted = accessed.intersectDomain(notyetExecuted); // { stmt[domain] -> field[indexset] }
        auto fvar = stmtCtx->getFieldVariable();
        auto fieldHome = fvar->getHomeAff(); // { [domain] -> [cluster] }
        auto homeAcc = accessedButNotyetExecuted.applyRange(fieldHome); // { stmt[domain] -> [cluster] }
        stmtCtx->addWhere(homeAcc);

        notyetExecuted.substract_inplace(accessedButNotyetExecuted.domain());
        localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
      }

      // "source computes": execute statements that read data from before the SCoP at the home location of that data

      for (auto input : inputFlow.getSets()) {
        auto stmtCtx = getScopStmtContext(input); 
        auto accessed = stmtCtx->getAccessRelation();
        auto accessedButNotyetExecuted = accessed.intersectDomain(notyetExecuted); 
        auto fvar = stmtCtx->getFieldVariable();
        auto fieldHome = fvar->getHomeAff();
        auto homeAcc = accessedButNotyetExecuted.applyRange(fieldHome); // { stmt[domain] -> [cluster] }
        stmtCtx->addWhere(homeAcc);

        notyetExecuted.substract_inplace(accessedButNotyetExecuted.domain());
        localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
      }


      while (true) {
        bool changed = false;
        if (notyetExecuted.isEmpty()) break;
        // "Dependence computes": Execute statements that computes value preferably on the node where the value is needed again
        for (auto flow : dataFlow.getMaps()) {
          auto producerSpace = flow.getDomainSpace();
          auto consumerSpace = flow.getRangeSpace();

          auto producerStmt = getScopStmtContext(producerSpace);
          auto consumerStmt = getScopStmtContext(consumerSpace);

          auto flowNotyetExecuted = flow.intersectRange(notyetExecuted); // { producer[domain] -> consumer[domain] }
          auto flowProducerWhere = flowNotyetExecuted.wrap().chainSubspace(producerStmt->getWhere()); // { (producer[domain] -> consumer[domain]) -> consumer[cluster] }
          auto producerSuggestedWhere = flowNotyetExecuted.applyDomain(producerStmt->getWhere()).reverse(); // { producer[domain] -> [cluster] }

          if (!producerSuggestedWhere.isEmpty()) {
            producerStmt->addWhere(producerSuggestedWhere);
            notyetExecuted.substract_inplace(producerSuggestedWhere.domain());
            localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
            changed = true;
          }
        }

        //TODO: This fixpoint computation should be done on a higher level; on unbounded domains, this method might not even finished
        // See UnionMap::transitiveClosure for how it should work
        if (changed) {
          //localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);
        } else {
          break;
        }
      }

#if 0
      // Apply owner computes until everything gets computed
      while (true) {

        localizeNonfieldFlowDeps(notyetExecuted, nonfieldDataFlowClosure);


        if (notyetExecuted.isEmpty())
          break;


        // Execute statements of the output flow at the owner (owner computes) 
        // TODO: What about non-output-flow stmts that cannot be omitted, like containing printf
        //auto outputOnly = outputFlow - dataFlow.domain(); // results that are required by other nodes will be schedules anyway (self-recursive-only dependencies never leak and thus can be ignored)
        //TODO: Find some senseful ordering

        // All the stuff that we do not know where to execute it, we need a location for; Yoda, I'm with you.
        isl::Set next; // { [domain] }
        notyetExecuted.foreachSet([&next](isl::Set set) -> bool {
          next= set;
          return false;
        });

        next.intersect_inplace(notyetExecuted);
        auto writeStmtCtx = getScopStmtContext(next); //TODO: Not all the ScopStmts are field accesses
        auto writeAccessed = writeStmtCtx->getAccessRelation(); // { writeStmt[domain] -> field[indexset] }
        auto out = writeAccessed.intersectDomain(next); // { writeStmt[domain] -> field[indexset] }
        auto fty = writeStmtCtx->getFieldType();
        auto fvar = writeStmtCtx->getFieldVariable();
        auto fieldHome = fvar->getHomeAff(); // { [domain] -> [cluster] }
        auto homeAcc = out.applyRange(fieldHome);
        writeStmtCtx->addWhere(homeAcc);
        notyetExecuted.substract_inplace(next);
      }
#endif
      /////////////////////////////////////////////////


      for (auto inp : inputFlow.getSets()) { 
        // Value source is outside this scop 
        // Value must be read from home location
        genInputCommunication(inp);
      }

      for (auto dep : dataFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        genFlowCommunication(dep);
      }

      for (auto out : outputFlow.getSets()) {
        // This means the data is visible outside the scop and must be written to its home location
        // There is no explicit read access
        genOutputCommunication(out);
      }


      //TODO: For every may-write, copy the original value into that the target memory (unless they are the same), so we copy the correct values into the buffers
      // i.e. treat ever may write as, write original value if not written
    }

    /// Make the data that is required to compute a ScopStmt (also) execute the value-computing ScopStmt
    void localizeNonfieldFlowDeps(isl::UnionSet &notyetExecuted, isl::UnionMap nonfieldDataFlowClosure)    {
      // When a stmt reads a value from a nonfield, the producing stmt must also be executed on the same node; communication between nodes is possible with fields only
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
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
      for (auto map : nonfieldDataFlowClosure.getMaps()) {
        auto source = map.getInTupleId();
        auto stmtCtx = getScopStmtContext(source);
        stmtCtx->setWhere(stmtCtx->getWhere().removeRedundancies_consume().coalesce_consume());
      }
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



#if 0
    void genFlowRead(MollyScopStmtProcessor *readStmtCtx, isl::Set chunks) {
      // chunks: { (recv[domain], readStmt[domain], dstNode[cluster], field[index], writeStmt[domain], srcNode[cluster]) }

      auto readDomain = readStmtCtx->getDomain();
      auto readDomainSpace = readStmtCtx->getDomainSpace();
      auto readNodeSpace = getStmtNodeSpace(readStmtCtx);

      auto readFlowWhere = chunks.reorganizeSubspaces(readDomainSpace, readNodeSpace);
      auto readFlowDomain = readFlowWhere.getDomain();
      auto readFlowScatter = readWhere;

      auto readEditor = readStmtCtx->getEditor();
      auto readFlowEditor = readEditor.createStmt(readFlowWhere.getDomain(), readFlowScatter, readFlowWhere, "readFlow");
    }
#endif

  private:
    isl::MultiAff beforeScopScatter; // { prologue[] -> scattering[scatter] }
    isl::MultiAff afterScopScatter; // { epilogue[] -> scattering[scatter] }


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
      auto readScatter = readStmt->getScattering().intersectDomain(readDomain); //  { readStmt[domain] -> scattering[scatter] }
      assert(readScatter.matchesMapSpace(readDomain.getSpace(), scatterTupleId));
      auto readWhere = readStmt->getWhere().intersectDomain(readDomain).setOutTupleId(dstNodeId); // { stmtRead[domain] -> dstNode[cluster] }
      auto readEditor = readStmt->getEditor();
      auto readVal = readStmt->getLoadAccessor();
      auto readAccessedAff = readStmt->getAccessed();

      auto fvar = readFvar;
      auto fty = fvar->getFieldType();
      auto homeAff = fvar->getHomeAff().setOutTupleId(srcNodeId); // { fvar[indexset] -> srcNode[cluster] }
      auto indexsetSpace = homeAff.getDomainSpace(); // { fvar[indexset] }

      auto transfer = readAccRel.chain(homeAff).wrap().chainSubspace(readWhere).wrap(); // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
      auto local = intersect(transfer, transfer.getSpace().equalBasicSet(dstNodeSpace, srcNodeSpace));
      auto remoteTransfer = transfer-local; // { readStmt[domain], field[index], srcNode[cluster], dstNode[cluster] }
      auto localTransfer = local.reorganizeSubspaces(indexsetSpace >> (readDomainSpace >> srcNodeSpace)).cast((indexsetSpace >> (readDomainSpace >> clusterSpace)).wrap()); // { fvar[indexset], readStmt[domain], [cluster] }


      ScopEditor editor(scop, asPass());

      if (!localTransfer.isEmpty()) { // TODO: For this part, we may just reuse the original ScopStmt
        auto readlocalWhere = localTransfer.reorderSubspaces(readDomainSpace, clusterSpace);
        auto readlocalScatter = readScatter;
        auto readlocalEditor = readEditor.createStmt(readlocalWhere.domain(), readlocalScatter, readlocalWhere, "readinput_local");
        readEditor.removeInstances(readlocalWhere);
        auto readlocalStmt = readlocalEditor.getStmt();
        auto readlocalStmtCtx = getScopStmtContext(readlocalStmt);

        auto readlocalCodegen = readlocalStmtCtx->makeCodegen();
        auto readlocalCurrentNode = readlocalStmtCtx->getClusterMultiAff(); // { readlocalStmt[domain] -> rank[cluster] }
        //auto readlocalCurrentAccessed = readAccRel.castDomain(readlocalStmtCtx->getDomainSpace()); // { readlocalStmt[domain] -> field[indexset] }
        auto readlocalCurrentAccessed = readAccessedAff.castDomain(readlocalStmtCtx->getDomainSpace()); // { readlocalStmt[domain] -> field[indexset] }

        auto readlocalVal = readlocalCodegen.codegenLoadLocal(fvar, readlocalCurrentNode, readlocalCurrentAccessed);
        readlocalCodegen.updateScalar(readVal->getPointerOperand(), readlocalVal);
      }


      if (!remoteTransfer.isEmpty()) {
        auto prologueDomain = beforeScopScatter.getDomain(); // { prologue[] }
        auto epilogueDomain = afterScopScatter.getDomain(); // { epilogue[] }

        auto comrel = alltoall(prologueDomain, remoteTransfer.reorderSubspaces((srcNodeSpace >> dstNodeSpace) >> indexsetSpace));  // { prologue[] -> (src[cluster], dst[cluster], fvar[indexset]) }
        auto combuf = pm->newCommunicationBuffer(fty, comrel);
        combuf->doLayoutMapping();


        // { dst[cluster] }: send_wait
        {
          auto sendwaitWhere = remoteTransfer.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto sendwaitScatter = constantScatter(dstNodeSpace.mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -3); // { srcNode[cluster] -> scattering[scatter] }
          auto sendwaitEditor = editor.createStmt(sendwaitWhere.domain(), sendwaitScatter, sendwaitWhere, "sendwait");
          auto sendwaitStmt = sendwaitEditor.getStmt();
          auto sendwaitStmtCtx = getScopStmtContext(sendwaitStmt);

          auto sendwaitCodegen = sendwaitStmtCtx->makeCodegen();
          auto sendwaitChunk = sendwaitStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { sendwaitStmt[domain] -> prologue[] }
          auto sendwaitSrc = sendwaitStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { sendwaitStmt[domain] -> src[cluster] }
          auto sendwaitDst = sendwaitEditor.getCurrentIteration().castRange(dstNodeSpace); // { sendwaitStmt[domain] -> dst[cluster] }

          combuf->codegenSendWait(sendwaitCodegen, sendwaitChunk, sendwaitSrc, sendwaitDst);
        }

        // { dst[cluster], readStmt[domain] }: getinput
        {
          auto getinputDomain = remoteTransfer.reorderSubspaces(dstNodeSpace >> readDomainSpace);
          auto getinputWhere = remoteTransfer.reorderSubspaces(dstNodeSpace >> readDomainSpace, srcNodeSpace).castRange(clusterSpace); // { (dst[cluster], readStmt[domain]) -> src[cluster] }
          auto getinputScatter = constantScatter(getinputWhere.getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -2); // { (dst[cluster], readStmt[domain]) -> scattering[scatter] }
          auto getinputEditor = editor.createStmt(getinputWhere.domain(), getinputScatter, getinputWhere, "getinput");
          auto getinputStmt = getinputEditor.getStmt();
          auto getinputStmtCtx = getScopStmtContext(getinputStmt);

          auto getinputCodegen = getinputStmtCtx->makeCodegen();
          auto getInputChunk = getinputStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { getinputStmt[domain] -> prologue[] }
          auto getinputSrc = getinputStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { getinputStmt[domain] -> src[cluster] }
          auto getinputReadStmt = getinputEditor.getCurrentIteration().sublist(readDomainSpace); // { getinputStmt[domain] -> readStmt[domain] }
          auto getinputIndex=  getinputReadStmt.applyRange(readAccessedAff);  // { getinputStmt[domain] -> fvar[indexset] }
          auto getinputDst = getinputEditor.getCurrentIteration().sublist(dstNodeSpace);  // { getinputStmt[domain] -> dst[cluster] }

          auto getinputVal = getinputCodegen.codegenLoadLocal(fvar, getinputSrc.castRange(clusterSpace), getinputIndex);
          //auto getinputBufptr = combuf->codegenPtrToSendBuf(getinputCodegen, getInputChunk, getinputSrc, getinputDst, getinputIndex);
          //getinputCodegen.createArrayStore(getinputVal, )
          //getinputCodegen.getIRBuilder().CreateStore(getinputVal, getinputBufptr);
          combuf->codegenStoreInSendbuf(getinputCodegen, getInputChunk, getinputSrc, getinputDst, getinputIndex, getinputVal);
        }


        // { dst[cluster] }: send
        {
          auto sendWhere = remoteTransfer.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto sendScatter = constantScatter(dstNodeSpace.mapsTo(prologueDomain.getSpace()).createZeroMultiAff(), beforeScopScatter, -1); // { srcNode[cluster] -> scattering[scatter] }
          auto sendEditor = editor.createStmt(sendWhere.domain(), sendScatter, sendWhere, "sendwait");
          auto sendStmt = sendEditor.getStmt();
          auto sendStmtCtx = getScopStmtContext(sendStmt);

          auto sendCodegen = sendStmtCtx->makeCodegen();
          auto sendChunk = sendStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { sendwaitStmt[domain] -> prologue[] }
          auto sendSrc = sendStmtCtx->getClusterMultiAff().castRange(srcNodeSpace); // { sendwaitStmt[domain] -> src[cluster] }
          auto sendDst = sendEditor.getCurrentIteration().castRange(dstNodeSpace); // { sendwaitStmt[domain] -> dst[cluster] }

          combuf->codegenSend(sendCodegen, sendChunk, sendSrc, sendDst);
        }


        // { src[cluster] } recv_wait
        {
          auto recvwaitWhere = remoteTransfer.reorderSubspaces(srcNodeSpace, dstNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto recvwaitScatter = relativeScatter2(remoteTransfer.reorderSubspaces(readDomainSpace, srcNodeSpace) /* { readStmt[domain] -> src[cluster] } */, readScatter, -1); // { src[cluster] -> scattering[] }
          auto recvwaitEditor = editor.createStmt(recvwaitWhere.domain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recvwait");
          auto recvwaitStmt = recvwaitEditor.getStmt();
          auto recvwaitStmtCtx = getScopStmtContext(recvwaitStmt);

          auto recvwaitCodegen = recvwaitStmtCtx->makeCodegen();
          auto recvwaitChunk = recvwaitStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { recvwaitStmt[domain] -> prologue[] }
          auto recvwaitSrc = recvwaitEditor.getCurrentIteration().castRange(srcNodeSpace); // { recvwaitStmt[domain] -> src[cluster] }
          auto recvwaitDst = recvwaitStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { recvwaitStmt[domain] -> dst[cluster] }

          combuf->codegenRecvWait(recvwaitCodegen, recvwaitChunk, recvwaitSrc, recvwaitDst);
        }


        // { readStmt[domain] }: readflow
        {
          auto readflowWhere = remoteTransfer.reorderSubspaces(readDomainSpace, dstNodeSpace).castRange(clusterSpace); // { readStmt[domain] -> dst[cluster] }
          auto readflowScatter = readScatter; // { readStmt[domain] -> scattering[] }
          auto readflowEditor = readEditor.createStmt(readflowWhere.domain(), readflowScatter, readflowWhere, "readflow");
          readEditor.removeInstances(readflowWhere.castRange(clusterSpace));
          auto readflowStmt = readflowEditor.getStmt();
          auto readflowStmtCtx = getScopStmtContext(readflowStmt);

          auto readflowCodegen = readflowStmtCtx->makeCodegen();
          auto readflowChunk = readflowStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { readflowStmt[domain] -> prologue[] }
          auto readflowDst = readflowStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { readflowStmt[domain] -> dst[cluster] }
          auto readflowOrig =readflowEditor.getCurrentIteration().castRange(readDomainSpace);  // { readflowStmt[domain] -> readStmt[domain] }
          auto readflowIndex = readflowOrig.applyRange(readAccessedAff); // { readflowStmt[domain] -> fvar[indexset] }
          auto readflowSrc =  readflowIndex.applyRange(homeAff).castRange(srcNodeSpace); // { readflowStmt[domain] -> src[indexset] }

          //auto readflowBufptr = combuf->codegenPtrToRecvBuf(readflowCodegen, readflowChunk, readflowSrc, readflowDst, readflowIndex);
          //auto readflowVal = readflowCodegen.getIRBuilder().CreateLoad(readflowBufptr, "readflowval");
         auto readflowVal = combuf->codegenLoadFromRecvBuf(readflowCodegen, readflowChunk, readflowSrc, readflowDst, readflowIndex);
          readflowCodegen.updateScalar(readVal->getPointerOperand(), readflowVal);
        }


        // { src[cluster] } recv
        {
          auto recvWhere = remoteTransfer.reorderSubspaces(srcNodeSpace, dstNodeSpace).castRange(clusterSpace); // { src[cluster] -> dst[cluster] }
          auto recvScatter = relativeScatter2(remoteTransfer.reorderSubspaces(readDomainSpace, srcNodeSpace) /* { readStmt[domain] -> src[cluster] } */, readScatter, +1); // { src[cluster] -> scattering[] }
          auto recvEditor = editor.createStmt(recvWhere.domain(), recvScatter.copy(), recvWhere.copy(), "recv");
          auto recvStmt = recvEditor.getStmt();
          auto recvStmtCtx = getScopStmtContext(recvStmt);

          auto recvCodegen = recvStmtCtx->makeCodegen();
          auto recvChunk = recvStmtCtx->getDomainSpace().mapsTo(prologueDomain.getSpace()).createZeroMultiAff(); // { recvwaitStmt[domain] -> prologue[] }
          auto recvSrc = recvEditor.getCurrentIteration().castRange(srcNodeSpace); // { recvwaitStmt[domain] -> src[cluster] }
          auto recvDst = recvStmtCtx->getClusterMultiAff().castRange(dstNodeSpace); // { recvwaitStmt[domain] -> dst[cluster] }

          combuf->codegenRecv(recvCodegen, recvChunk, recvSrc, recvDst);
        }
      }

      assert(readEditor.getInstances().isEmpty());
    }




  private:
    /// return the first dimension from which on all dependencies are always lexicographically positive
    /// examples:
    /// { (1,0,0) -> (0,1,0) } returns 1
    /// { (0,i,0) -> (0,i+1,-1) } returns 1
    /// { (0,0) -> (0,0) } returns 2, these are never lexicographically positive
    unsigned computeLevelOfDependence(const isl::Map &depScatter) {  /* dep: { scattering[scatter] -> scattering[scatter] } */
      auto depMap = depScatter.getSpace();
      auto dependsVector = depMap.emptyMap(); /* { read[scatter] -> (read[scatter] - write[scatter]) } */
      auto nParam = depScatter.getParamDimCount();
      auto nIn = depScatter.getInDimCount();
      auto nOut = depScatter.getOutDimCount();
      assert(nIn==nOut);
      auto scatterSpace = depScatter.getDomainSpace(); /* { scattering[scatter] } */
      auto nCompareDims = std::min(nIn, nOut);

      // get the first always-positive coordinate such that the dependence is satisfied
      // Everything after that doesn't matter anymore to meet that dependence
      for (auto i = nCompareDims-nCompareDims; i < nCompareDims; i+=1) {
        auto orderMap = depMap.universeBasicMap().orderLt(isl_dim_in, i, isl_dim_out, i);
        // auto fulfilledDeps = depScatter.intersect(orderMap);
        if (orderMap >= depScatter) {
          // All dependences are fullfilled from here
          // i.e. we can shuffle stuff as we want in lower dimensions
          return i;
        }
      }

      // No always-positive dependence, therefore return the first out-of-range
      return std::max(nIn, nOut);
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
      auto readEditor = readStmt->getEditor();

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
      auto levelOfDep = computeLevelOfDependence(scatterDep);
      auto levelOfIndep = scatterSpace.getSetDimCount() - levelOfDep;

      // partition into subdomains such that all elements of a subdomain can be arbitrarily ordered
      // TODO: It would be better to derive the partitioning from dep (the data flow), so we derive a send and recv matching
      auto recvId = islctx->createId("recv");
      auto readPartitioning = partitionDomain(readScatter, levelOfDep).setOutTupleId(recvId); // { readStmt[domain] -> recv[domain] }
      auto sendId = islctx->createId("send");
      auto writePartitioning = partitionDomain(writeScatter, levelOfDep).setOutTupleId(sendId); // { writeStmt[domain] -> send[domain] }

      auto depPartitioning = readPartitioning.reverse().chainNested(isl_dim_out, dep.reverse()); // { recv[domain] -> (readStmt[domain], writeStmt[domain]) }
      auto readChunks_ = chunkDomain(readScatter, levelOfDep, false).setInTupleId(recvId); // { recv[domain] -> readStmt[domain] }
      auto writeChunks_ = chunkDomain(writeScatter, levelOfDep, true); // { lastWrite[domain] -> writeStmt[domain] }
      auto readChunkAff = chunkDomainPwAff(readScatter.toPwMultiAff(), levelOfDep, false).setOutTupleId(recvId); // { readStmt[domain] -> recv[domain] }
      auto readChunks = readChunkAff.reverse();

      auto sendDomain = writePartitioning.getRange();
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
      //auto depSubInst = depSub.applyDomain(writeInstMap).applyRange(readInstMap); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster]) }


      //auto depSubInstLoc = depSubInst.chainNested(isl_dim_out, readInstAccess); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster], field[index]) }
      auto depInstLoc = depInst.chainNested(isl_dim_in, writeAccRel).chainNested(isl_dim_out, readAccRel); // { (writeStmt[domain], node[cluster]) -> (readStmt[domain], node[cluster], field[index]) }
      auto eqField = depInstLoc.getSpace().equalSubspaceBasicMap(indexsetSpace);
      assert(depInstLoc <= eqField);
      depInstLoc.intersect_inplace(eqField);
      depInstLoc.projectOutSubspace_inplace(isl_dim_in, indexsetSpace);

      auto depInstLocChunks = depInstLoc.reverse().wrap().chainSubspace(readChunkAff).reverse(); // { recv[domain] -> (readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }

      // For every read, we need to select one write instance. It will decide from which node we will receive the data
      // TODO: This selection is arbitrary; preferable select data from local node and nodes we have to send data from anyways
      // { (readStmt[domain], node[cluster], field[index]) -> (writeStmt[domain], node[cluster]) }
      auto sourceOfRead = depInstLoc.wrap().reorganizeSubspaces(readInstDomain.getSpace() >> indexsetSpace, writeInstDomain.getSpace()).lexminPwMultiAff();

      // Organize in chunks
      // { recv[domain] -> (readStmt[domain], node[cluster], field[index], writeStmt[domain], node[cluster]) }
      auto sourceOfReadChunks = sourceOfRead.wrap().chainSubspace(readChunkAff).reverse();
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
      auto chunks = recvChunksWrapped.intersect(firstWritePerChunk.wrap().reorganizeSubspaces(recvChunksWrapped.getSpace()));
      writeInstDomain = chunks.reorganizeSubspaces(writeInstDomain.getSpace()); // { (writeStmt[domain], node[cluster]) }
      writeInstScatter = writeInstDomain.chainSubspace(writeScatter);  // { (writeStmt[domain], node[cluster]) -> scattering[scatter] }
      auto writeInstWhere = writeInstDomain.reorganizeSubspaces(writeInstDomain.getSpace(), writeInstDomain.getSpace().unwrap().getRangeSpace()); // { (writeStmt[domain], node[cluster]) -> scattering[scatter] }

      // Order write is chunks as defined by already selected read chunks
      auto sendByWrite = chunks.reorganizeSubspaces(recvInstDomain.getSpace(), writeInstDomain.getSpace()); // { (recv[domain], node[cluster]) -> writeStmt[domain] } 
      // { (recv[domain] -> node[cluster]) -> sendScattering[scatter] }
      //auto sendScatter = relativeScatter(sendByWrite, writeScatter.wrap().reorganizeSubspaces(writeInstDomain.getSpace(), writeScatter.getRangeSpace()), -1);


      auto writeFlowWhere = chunks.reorganizeSubspaces(writeDomain.getSpace() >> readNodeShape.getSpace() >> readChunkAff.getRangeSpace(), writeNodeShape.getSpace()); // { (writeStmt[domain], dstNode[cluster], recv[domain]) -> node[cluster] }
      auto writeFlowDomain = writeFlowWhere.getDomain(); // { (writeStmt[domain], dstNode[cluster], recv[domain]) }
      auto writeFlowScatter = writeFlowDomain.chainSubspace(writeScatter);

      auto readFlowWhere = chunks.reorganizeSubspaces(readDomain.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { readStmt[domain] -> node[cluster] }
      auto readFlowDomain = readFlowWhere.getDomain();
      //auto readFlowScatter = readWhere;

      //auto communicationSet = depInstLoc;
      //communicationSet.projectOutSubspace_inplace(isl_dim_in, writeDomain.getSpace()); /* { (nodeSrc[cluster], nodeDst[cluster]) -> field[indexset] } */
      //communicationSet.projectOutSubspace_inplace(isl_dim_in, indexsetSpace);
      //communicationSet.projectOutSubspace_inplace(isl_dim_out, readDomain.getSpace());
      // { writeNode[cluster] -> (readNode[cluster], field[index]) }
      //communicationSet.uncurry_inplace();
      // { (writeNode[cluster], readNode[cluster]) -> field[index] }
      auto communicationSet = sourceOfRead.wrap().reorganizeSubspaces(writeNodeShape.getSpace() >> readNodeShape.getSpace(), indexsetSpace);
      auto comRelation = chunks.reorganizeSubspaces(chunkSpace, (writeNodeShape.getSpace() >> readNodeShape.getSpace()) >> indexsetSpace); // { recv[] -> (writeNode[cluster], readNode[cluster], field[indexset]) }

      // We may need to send to/recv from any node in the cluster
      auto sendDstDomain = depInst.getDomain().unwrap().setInTupleId(sendDomain.getTupleId()).intersectDomain(sendDomain).wrap(); // { (writeStmt[domain], nodeDst[cluster]) }
      auto recvSrcDomain = depInst.getRange().unwrap().setInTupleId(recvDomain.getTupleId()).intersectDomain(recvDomain).wrap(); // { (readStmt[domain], nodeDst[cluster]) }

      // Execute all send/recv statement in parallel, i.e. same scatter point
      auto sendDstScatter = sendDstDomain.chainSubspace(writeScatter.setInTupleId(sendDomain.getTupleId())); // { (writeStmt[domain], nodeDst[cluster]) -> scattering[scatter] }
      auto recvSrcScatter = recvSrcDomain.chainSubspace(readScatter.setInTupleId(recvDomain.getTupleId())); // { (writeStmt[domain], nodeDst[cluster]) -> scattering[scatter] }

      auto sendDstWhere = sendDstDomain.unwrap().rangeMap(); // { (writeStmt[domain], nodeDst[cluster]) -> nodeDst[cluster] }
      auto recvSrcWhere = recvSrcDomain.unwrap().rangeMap(); // { (readStmt[domain], nodeSrc[cluster]) -> nodeSrc[cluster] }


      // Codegen
      //TODO: Handle local communication separately, do not use combuf
      auto combuf = pm->newCommunicationBuffer(fty, comRelation); 
      combuf->doLayoutMapping();

      ScopEditor editor(scop, asPass());

      // send_wait
      auto sendwaitWhere = chunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> readNodeShape.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId);  // { (recv[domain], dstNode[cluster]) -> srcNode[cluster] }
      auto sendwaitScatter = relativeScatter(chunks.reorganizeSubspaces(sendwaitWhere.getDomainSpace(), writeDomain.getSpace()), writeScatter, -1); // { (recv[domain], dstNode[cluster]) -> scatter[scattering] }
      auto sendwaitEditor = editor.createStmt(sendwaitWhere.getDomain() /* { recv[domain], dstNode[cluster] } */, sendwaitScatter.copy(), sendwaitWhere.copy(), "send_wait");
      auto sendwaitStmt = getScopStmtContext(sendwaitEditor.getStmt());
      auto sendwaitCodegen = sendwaitStmt->makeCodegen();

      auto sendwaitCurrentIteration = sendwaitEditor.getCurrentIteration(); // { [] -> (recv[domain], dstNode[cluster]) }
      auto sendwaitCurrentChunk = sendwaitCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
      auto sendwaitCurrentDst = sendwaitCurrentIteration.sublist(readNodeShape.getSpace()); // { [] -> dstNode[cluster] }
      auto sendwaitCurrentNode = sendwaitStmt->getClusterMultiAff().setOutTupleId(writeNodeId); // { [] -> srcNode[cluster] }
      combuf->codegenSendWait(sendwaitCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, sendwaitCurrentDst);


      // write
      auto writeFlowEditor = writeEditor.createStmt(writeFlowDomain /* { (writeStmt[domain], dstNode[cluster], recv[domain]) } */, writeFlowScatter, writeFlowWhere.setOutTupleId(clusterTupleId) /* { (writeStmt[domain], dstNode[cluster], recv[domain]) -> srcNode[cluster] } */, "writeFlow");
      writeEditor.removeInstances(writeFlowWhere.wrap().reorganizeSubspaces(writeDomain.getSpace(), writeNodeShape.getSpace()).setOutTupleId(clusterTupleId));
      auto writeFlowStmt = getScopStmtContext(writeFlowEditor.getStmt());
      auto writeFlowCodegen = writeFlowStmt->makeCodegen();

      auto writeStore = writeStmt->getStoreAccessor();
      auto writeAccessed = writeStmt->getAccessed().toPwMultiAff(); // { writeStmt[domain] -> field[indexset] }
      auto writeVal = writeStore->getValueOperand();
      auto writeflowVal = writeFlowCodegen.materialize(writeVal);

      auto writeFlowCurrentIteration = writeFlowEditor.getCurrentIteration();
      auto writeFlowCurrentDst = writeFlowCurrentIteration.sublist(readNodeShape.getSpace());
      auto writeFlowCurrentChunk = writeFlowCurrentIteration.sublist(readChunkAff.getRangeSpace());
      auto writeFlowCurrentDomain = writeFlowCurrentIteration.sublist(writeDomain.getSpace());
      auto writeFlowCurrentWriteAccessed = writeFlowCurrentDomain.applyRange(writeAccessed);
      auto writeFlowCurrentNode = writeFlowStmt->getClusterMultiAff().setOutTupleId(writeNodeId);
      combuf->codegenStoreInSendbuf(writeFlowCodegen, writeFlowCurrentChunk, writeFlowCurrentNode, writeFlowCurrentDst, writeFlowCurrentWriteAccessed, writeflowVal);


      // send
      auto sendWhere = sendwaitWhere; // { (recv[domain], dstNode[cluster]) -> srcNode[cluster] }
      auto sendScatter = relativeScatter(chunks.reorganizeSubspaces(sendWhere.getDomainSpace(), writeDomain.getSpace()), writeScatter, +1); // { (recv[domain], dstNode[cluster]) -> scatter[scattering] }
      auto sendEditor = editor.createStmt(sendWhere.getDomain() /* { recv[domain], dstNode[cluster] } */, sendScatter.copy(), sendWhere.copy(), "send");
      auto sendStmt = getScopStmtContext(sendEditor.getStmt());
      auto sendCodegen = sendStmt->makeCodegen();

      auto sendCurrentIteration = sendEditor.getCurrentIteration(); // { [] -> (recv[domain], dstNode[cluster]) }
      auto sendCurrentChunk = sendCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
      auto sendCurrentDst = sendCurrentIteration.sublist(readNodeShape.getSpace()); // { [] -> dstNode[cluster] }
      auto sendCurrentNode = sendStmt->getClusterMultiAff().setOutTupleId(writeNodeId); // { [] -> srcNode[cluster] }
      combuf->codegenSend(sendCodegen, sendCurrentChunk, sendCurrentNode, sendCurrentDst);


      // recv_wait
      auto recvwaitWhere = chunks.reorganizeSubspaces(readChunkAff.getRangeSpace() >> writeNodeShape.getSpace(), readNodeShape.getSpace()).setOutTupleId(clusterTupleId); // { (readStmt[domain], srcNode[cluster]) -> dstNode[cluster] } 
      {
        auto recvwaitScatter = relativeScatter(chunks.reorganizeSubspaces(recvwaitWhere.getDomainSpace(), readDomain.getSpace()), readScatter, -1);
        auto recvwaitEditor = editor.createStmt(recvwaitWhere.getDomain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recv");
        auto recvwaitStmt = getScopStmtContext(recvwaitEditor.getStmt());
        auto recvwaitCodegen = recvwaitStmt->makeCodegen();

        auto recvwaitCurrentIteration = recvwaitEditor.getCurrentIteration(); // { [] -> (recv[domain], srcNode[cluster]) }
        auto recvwaitCurrentChunk = recvwaitCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
        auto recvwaitCurrentSrc = recvwaitCurrentIteration.sublist(writeNodeShape.getSpace()); // { [] -> srcNode[cluster] }
        auto recvwaitCurrentNode = recvwaitStmt->getClusterMultiAff().setOutTupleId(readNodeId); // { [] -> dstNode[cluster] }
        combuf->codegenRecvWait(recvwaitCodegen, recvwaitCurrentChunk, recvwaitCurrentSrc, recvwaitCurrentNode);
      }


      // read
      // Modify read such that it reads from the combuf instead
      auto readflowScatter = readScatter;
      auto readFlowEditor = readEditor.createStmt(readFlowDomain, readflowScatter, readFlowWhere, "readFlow");
      readEditor.removeInstances(readFlowWhere.setOutTupleId(clusterTupleId));
      auto readFlowStmt = getScopStmtContext(readFlowEditor.getStmt());
      auto readFlowCodegen = readFlowStmt->makeCodegen();

      auto readLoad = readStmt->getLoadAccessor();
      auto readAccessed = readStmt->getAccessed().toPwMultiAff(); // { readStmt[domain] -> field[indexset] }

      auto readFlowCurrentDomain = readFlowEditor.getCurrentIteration().setOutTupleId(readDomain.getTupleId());
      auto readFlowCurrentChunk = readFlowCurrentDomain.applyRange(readChunkAff);
      auto readFlowCurrentReadAccessed = readFlowCurrentDomain.applyRange(readAccessed);
      auto readFlowCurrentNode = readFlowStmt->getClusterMultiAff().setOutTupleId(readNodeId);
      auto whatsthesourceInfo = rangeProduct(rangeProduct(readFlowCurrentDomain, readFlowCurrentNode), readFlowCurrentReadAccessed.toMultiPwAff()).toPwMultiAff();
      auto readFlowCurrentSrc = sourceOfRead.sublist(writeNodeShape.getSpace()).pullback(whatsthesourceInfo);
      auto readflowVal = combuf->codegenLoadFromRecvBuf(readFlowCodegen, readFlowCurrentChunk, readFlowCurrentSrc, readFlowCurrentNode, readFlowCurrentReadAccessed);
      readFlowCodegen.updateScalar(readLoad, readflowVal);


      // recv
      {
        auto recvWhere = recvwaitWhere; // { (readStmt[domain], srcNode[cluster]) -> dstNode[cluster] } 
        auto recvScatter = relativeScatter(chunks.reorganizeSubspaces(recvWhere.getDomainSpace(), readDomain.getSpace()), readScatter, +1);
        auto recvEditor = editor.createStmt(recvWhere.getDomain(), recvScatter.copy(), recvWhere.copy(), "recv");
        auto recvStmt = getScopStmtContext(recvEditor.getStmt());
        auto recvCodegen = recvStmt->makeCodegen();

        auto recvCurrentIteration = recvEditor.getCurrentIteration(); // { [] -> (recv[domain], srcNode[cluster]) }
        auto recvCurrentChunk = recvCurrentIteration.sublist(readChunkAff.getRangeSpace()); // { [] -> recv[domain] }
        auto recvCurrentSrc = recvCurrentIteration.sublist(writeNodeShape.getSpace()); // { [] -> srcNode[cluster] }
        auto recvCurrentNode = recvStmt->getClusterMultiAff().setOutTupleId(readNodeId); // { [] -> dstNode[cluster] }
        combuf->codegenRecv(recvCodegen, recvCurrentChunk, recvCurrentSrc, recvCurrentNode);
      }

    } // void genFlowCommunication(const isl::Map &dep) 


    void genOutputCommunication(const isl::Set &outSet/* { writeStmt[domain] } */) {
      auto writeDomainTuple = outSet.getTupleId();
      //auto writeStmt = tupleToStmt[writeDomainTuple.keep()];
      auto writeStmtCtx = getScopStmtContext(outSet);
      auto writeEditor = writeStmtCtx->getEditor();
      auto srcNodeId = islctx->createId("srcNode");
      auto dstNodeId = islctx->createId("dstNode");
      auto fvar = writeStmtCtx->getFieldVariable();
      auto fvarId = fvar->getTupleId();
      auto clusterSpace = getClusterConfig()->getClusterSpace();
      auto clusterNodeId = clusterSpace.getSetTupleId();
      auto dstNodeSpace = clusterSpace.setSetTupleId(dstNodeId);

      auto writeAccessed = writeStmtCtx->getAccessRelation(); // { writeStmt[domain] -> field[indexset] }
      auto out = writeAccessed.intersectDomain(outSet); // { writeStmt[domain] -> field[indexset] }
      auto fty = writeStmtCtx->getFieldType();
      auto fieldHome = fty->getHomeAff(); // { field[indexset] -> node[cluster] }
      auto readHome = fieldHome.setOutTupleId(dstNodeId).setInTupleId(fvarId);
      auto writeHome = fieldHome.setOutTupleId(srcNodeId).setInTupleId(fvarId);
      auto indexsetSpace = writeAccessed.getRangeSpace();
      auto indexsetTupleId = indexsetSpace.getSetTupleId();

      auto writeWhere = writeStmtCtx->getWhere().setOutTupleId(srcNodeId);
      auto srcNodeSpace = writeWhere.getRangeSpace();
      auto writeOutInstances = writeWhere.wrap().chainSubspace(out); // { (writeStmt[domain], writeNode[cluster]) -> field[indexset] }

      // For each index, select one write instance that will be stored into the array
      auto selectedWrite = writeOutInstances.reverse().lexminPwMultiAff(); // { field[indexset] -> (writeStmt[domain], writeNode[cluster]) }

      auto writeInstDomain = selectedWrite.getRange(); // { (writeStmt[domain], writeNode[cluster]) }
      auto writeInstDomainSpace = writeInstDomain.getSpace();
      auto writeDomain = writeInstDomain.unwrap().getDomain(); // { writeStmt[domain] }
      auto writeDomainSpace = writeDomain.getSpace();
      auto writeScatter = writeStmtCtx->getScattering().intersectDomain(writeDomain); // { writeStmt[domain] -> scattering[scatter] }
      auto epilogueDomain = afterScopScatter.getDomain(); // { epilogue[] }
      auto epilogueDomainSpace = epilogueDomain.getSpace();

      auto transfer = selectedWrite.wrap().chainSubspace(readHome).wrap(); // { (field[indexset], writeStmt[domain], srcNode[cluster], dstNode[cluster]) }
      auto local = transfer.intersect(transfer.getSpace().equalBasicSet(dstNodeSpace, srcNodeSpace));
      auto remoteTransfers = transfer-local;
      auto localTransfers = local.reorderSubspaces(indexsetSpace >> (writeDomainSpace >> srcNodeSpace)).cast((indexsetSpace >> (writeDomainSpace >> clusterSpace)).normalizeWrapped()); // { (field[indexset], writeStmt[domain], node[cluster]) } 

      auto comrel = alltoall(epilogueDomain, remoteTransfers.reorderSubspaces((srcNodeSpace >> dstNodeSpace) >> indexsetSpace)); // { recv[] -> (writeNode[cluster], readNode[cluster], field[indexset]) }


      ScopEditor editor(scop, asPass());


      // { writeStmt[domain] }: writeback (local)
      if (!localTransfers.isEmpty()) {
        auto writelocalWhere = localTransfers.reorderSubspaces(writeDomainSpace, clusterSpace);
        auto writelocalScatter = writeScatter;
        auto writelocalEditor = writeEditor.createStmt(writelocalWhere.domain(), writelocalScatter, writelocalWhere, "writeback_local");
        writeEditor.removeInstances(writelocalWhere);
        auto writelocalStmt = writelocalEditor.getStmt();
        auto writelocalStmtCtx = getScopStmtContext(writelocalStmt);
        auto writelocalCodegen = writelocalStmtCtx->makeCodegen();

        auto writeStore = writeStmtCtx->getStoreAccessor();
        auto writeVal = writeStore->getValueOperand();
        auto writeAccessRel = writeStmtCtx->getAccessRelation();
        auto writeAccessed = writeStmtCtx->getAccessed();

        auto writelocalCurrent = writelocalEditor.getCurrentIteration().setOutTupleId(writeDomainTuple);
        auto writelocalCurrentNode = writelocalStmtCtx->getClusterMultiAff(); // { writelocalStmt[domain] -> node[cluster] }
        auto writelocalCurrentAccessed = writeAccessed.setInTupleId(writelocalCurrent.getInTupleId()); // { writelocalStmt[domain] -> field[indexset] }

        SmallVector<Value*,4> writelocalCurrentIndex;
        auto nDims = fty->getNumDimensions();
        writelocalCurrentIndex.reserve(nDims);
        for (auto i=nDims-nDims; i<nDims; i+=1) {
          auto v = writeStmtCtx->getAccessedCoordinate(i);
          writelocalCurrentIndex.push_back(v);
        } 

        auto writelocalVal = writelocalCodegen.materialize(writeVal);
        writelocalCodegen.codegenStoreLocal(writelocalVal, fvar, writelocalCurrentNode, writelocalCurrentAccessed);
      }


      if (!remoteTransfers.isEmpty()) {
        auto combuf = pm->newCommunicationBuffer(fty, comrel);
        combuf->doLayoutMapping();

        // { readNode[cluster] }: send_wait
        auto sendwaitWhere = remoteTransfers.reorderSubspaces(dstNodeSpace, srcNodeSpace).castRange(clusterSpace); // { readNode[cluster] -> writeNode[cluster] }
        auto sendwaitScatter = relativeScatter2(remoteTransfers.reorderSubspaces(writeDomainSpace, dstNodeSpace) /* { writeStmt[domain] -> readNode[cluster] } */, writeScatter, -1); // { readNode[cluster] -> scattering[scatter] }
        auto sendwaitEditor = editor.createStmt(sendwaitWhere.getDomain(), sendwaitScatter.copy(), sendwaitWhere.copy(), "sendwait");
        auto sendwaitStmt = sendwaitEditor.getStmt();
        auto sendwaitStmtCtx = getScopStmtContext(sendwaitStmt);
        auto sendwaitCodegen = sendwaitStmtCtx->makeCodegen();

        auto sendwaitCurrentChunk = dstNodeSpace.mapsTo(epilogueDomainSpace).createZeroMultiAff(); // { readNode[cluster] -> epilogue[] }
        auto sendwaitCurrentNode = sendwaitStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);
        auto sendwaitCurrentDst = sendwaitEditor.getCurrentIteration().setOutTupleId(dstNodeId);
        combuf->codegenSendWait(sendwaitCodegen, sendwaitCurrentDst, sendwaitCurrentNode, sendwaitCurrentNode);


        // { writeStmt[domain] }: writeflow
        auto writeflowWhere = remoteTransfers.reorderSubspaces(writeDomainSpace, srcNodeSpace);
        auto writeflowScatter = writeScatter;
        auto writeflowEditor = writeEditor.createStmt(writeflowWhere.getDomain(), writeflowScatter.copy(), writeflowWhere, "writeflow");
        writeEditor.removeInstances(writeflowWhere);
        auto writeflowStmt = writeflowEditor.getStmt();
        auto writeflowStmtCtx = getScopStmtContext(writeflowStmt);
        auto writeflowCodegen = writeflowStmtCtx->makeCodegen();

        auto writeStore = writeStmtCtx->getStoreAccessor();
        auto writeAccessedAff = writeStmtCtx->getAccessed(); // { writeStmt[domain] -> field[indexset] }

        auto writeflowCurrent = writeflowEditor.getCurrentIteration();
        auto writeflowCurrentDomain = writeflowCurrent.setOutTupleId(writeDomainTuple);
        auto writeflowCurrentNode = writeflowStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);
        auto writeflowCurrentIndex = writeAccessedAff.pullback(writeflowCurrentDomain).toPwMultiAff();
        auto writeflowCurrentDst = fieldHome.pullback(writeflowCurrentIndex);
        auto writeflowCurrentChunk = writeDomainSpace.mapsTo(epilogueDomainSpace).createZeroMultiAff() ; // {  writeStmt[domain] -> prologue[] }
        auto writeflowPtr = combuf->codegenPtrToSendBuf(writeflowCodegen, writeflowCurrentChunk, writeflowCurrentNode, writeflowCurrentDst, writeflowCurrentIndex);
        auto writeflowStore = writeflowCodegen.getIRBuilder().CreateStore(writeStore->getValueOperand(), writeflowPtr);


        // { dstNode[cluster] }: send
        auto sendWhere = remoteTransfers.reorderSubspaces(dstNodeSpace, srcNodeSpace); // { dstNode[cluster] -> srcNode[cluster] }
        auto sendScatter = relativeScatter2(remoteTransfers.reorderSubspaces(writeDomainSpace, dstNodeSpace), writeScatter, +1);
        auto sendEditor = editor.createStmt(sendWhere.getDomain(), sendScatter.copy(), sendWhere.copy(), "send");
        auto sendStmt = sendEditor.getStmt();
        auto sendStmtCtx = getScopStmtContext(sendStmt);
        auto sendCodegen = sendStmtCtx->makeCodegen();

        auto sendCurrent = sendEditor.getCurrentIteration();
        auto sendCurrentDst = sendCurrent.setOutTupleId(dstNodeId);
        auto sendCurrentNode = sendStmtCtx->getClusterMultiAff().setOutTupleId(srcNodeId);
        combuf->codegenSend(sendCodegen, sendCurrentNode, sendCurrentNode, sendCurrentDst);


        // { srcNode[cluster] }: recv_wait
        auto recvwaitWhere = remoteTransfers.reorderSubspaces(srcNodeSpace, dstNodeSpace); // { src[cluster] -> dst[cluster] }
        auto recvwaitScatter = constantScatter(srcNodeSpace.mapsTo(afterScopScatter.getDomainSpace()).createZeroMultiAff(), afterScopScatter, 0); // { srcNode[cluster] -> scattering[scatter] }
        auto recvwaitEditor = editor.createStmt(recvwaitWhere.getDomain(), recvwaitScatter.copy(), recvwaitWhere.copy(), "recvwait");
        auto recvwaitStmt = recvwaitEditor.getStmt();
        auto recvwaitStmtCtx = getScopStmtContext(recvwaitStmt);
        auto recvwaitCodegen = recvwaitStmtCtx->makeCodegen();

        auto recvwaitCurrent = recvwaitEditor.getCurrentIteration(); // { srcNode[cluster] -> [...] }
        auto recvwaitCurrentSrc = recvwaitCurrent.setOutTupleId(srcNodeId);
        auto recvwaitCurrentNode = recvwaitStmtCtx->getClusterMultiAff().setOutTupleId(dstNodeId);
        combuf->codegenRecvWait(recvwaitCodegen, recvwaitCurrentSrc, recvwaitCurrentSrc, recvwaitCurrentNode);


        // { field[indexset] }: writeback/read
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
        auto writebackCurrentSrc = selectedWrite.sublist(srcNodeSpace); // { field[indexset] -> srcNode[cluster] }

        auto writebackLocalPtr = combuf->codegenPtrToRecvBuf(writebackCodegen, sendwaitCurrentChunk, sendwaitCurrentNode, writebackCurrentSrc, writebackCurrentIndex);
        auto writebackVal = writebackCodegen.getIRBuilder().CreateLoad(writebackLocalPtr, "loadfromcombuf");
        writebackCodegen.codegenStoreLocal(writebackVal, fvar, writebackCurrentNode, writebackCurrentIndex);


        // { srcNode[cluster] }: recv
        auto recvWhere = recvwaitWhere;
        auto recvScatter = constantScatter(srcNodeSpace.mapsTo(afterScopScatter.getDomainSpace()).createZeroMultiAff(), afterScopScatter, +2);
        auto recvEditor = editor.createStmt(recvWhere.getDomain(), recvScatter.copy(), recvWhere.copy(), "recv");
        auto recvStmt = recvEditor.getStmt();
        auto recvStmtCtx = getScopStmtContext(recvStmt);

        auto recvCurrent = recvEditor.getCurrentIteration(); // { srcNode[cluster] -> [...] }
        auto recvCurrentSrc = recvCurrent.setOutTupleId(srcNodeId);
        auto recvCurrentNode = recvStmtCtx->getClusterMultiAff().setOutTupleId(dstNodeId);
        combuf->codegenRecv(writebackCodegen, recvCurrentSrc, recvCurrentSrc, recvCurrentNode);
      }

#if 0
      auto instances = out.getDomain();
      auto itDomainWrite = instances; //dep.getDomain(); /* write_acc[domain] */
      auto stmtWrite = tupleToStmt[itDomainWrite.getTupleId().keep()];
      assert(stmtWrite);
      //auto memaccWrite = tupleToAccess[itDomainWrite.getTupleId().keep()];
      //assert(memaccWrite);
      auto faccWrite = getFieldAccess(stmtWrite); 
      assert(faccWrite.isValid());
      auto memaccWrite = faccWrite.getPollyMemoryAccess();
      //auto stmtWrite = faccWrite.getPollyScopStmt();
      auto scatterWrite = getScattering(stmtWrite); /* { write_stmt[domain] -> scattering[scatter] } */
      auto relWrite = faccWrite.getAccessRelation(); /* { write_stmt[domain] -> field[indexset] } */
      auto domainWrite = itDomainWrite.setTupleId(isl::Id::enwrap(stmtWrite->getTupleId())); /* write_stmt[domain] */
      auto fvar = faccWrite.getFieldVariable();
      auto fty = fvar->getFieldType();
      auto nScatterDims = scatterWrite.getOutDimCount();
      auto scatterSpace = scatterWrite.getRangeSpace();

      // What is written here?
      auto indexset = domainWrite.apply(relWrite); /* { field[indexset] } */

      // Where is it written?
      auto writtenLoc = getWhereMap(stmtWrite); /* { write_stmt[domain] -> [nodecoord] } */
      writtenLoc.intersectDomain_inplace(domainWrite);
      auto valueLoc = writtenLoc.applyDomain(relWrite); /* { field[indexset] -> [nodecoord] } */

      // Get the subset that is already home
      auto homeAff = fty->getHomeAff(); /* { field[indexset] -> [nodecoord] } */
      homeAff = faccWrite.getHomeAff();
      auto affectedNodes = indexset.apply(homeAff); /* { [nodecoord] } */
      auto homeRel = homeAff.toMap().reverse().intersectRange(indexset); /* { [nodecoord] -> field[indexset] } */
      auto alreadyHome = intersect(valueLoc.reverse(), homeRel).getRange(); /* { field[indexset] } */
      auto writeAlreadyHomeDomain = alreadyHome.apply(relWrite.reverse()); /* { write_stmt[domain] } */

      // For all others, where do they are to be transported?
      writtenLoc;

#pragma region CodeGen
      auto store = faccWrite.getStoreInst();
      auto val = store->getValueOperand();

      IRBuilder<> prologueBuilder(func->getEntryBlock().getFirstNonPHI());
      auto stackMem = prologueBuilder.CreateAlloca(val->getType());


      // Add writes to local storage for already-home writes
      auto leastmostOne = scatterSpace.mapsTo(nScatterDims).createZeroMultiAff().setAff(nScatterDims-1, scatterSpace.createConstantAff(1));
      auto afterWrite = scatterWrite.sum(scatterWrite.applyRange(leastmostOne).setOutTupleId(scatterWrite.getOutTupleId())).intersectDomain(writeAlreadyHomeDomain);
      auto wbWhere = homeRel.applyRange(relWrite.reverse()).reverse();

      ScopEditor editor(scop);
      auto writebackLocalStmtEditor = editor.replaceStmt(stmtWrite, wbWhere.copy(), "writeback_local");
      //createStmt(writeAlreadyHomeDomain.copy(), afterWrite.copy(), wbWhere.copy(),  "writeback_local"); //createScopStmt(scop, bb, stmtWrite->getRegion(), "writeback_local", stmtWrite->getLoopNests(), writeAlreadyHomeDomain.copy(), afterWrite.move());
      auto writebackLocalStmt = writebackLocalStmtEditor.getStmt();
      auto writebackLocalStmtCtx = getScopStmtContext(writebackLocalStmt);

      auto bb = writebackLocalStmtEditor.getBasicBlock();
      //BasicBlock *bb = BasicBlock::Create(llvmContext, "OutputWrite", func);
      DefaultIRBuilder builder(bb);

      auto codegen = writebackLocalStmtCtx->makeCodegen();
      codegen.codegenStoreLocal(val, fvar, faccWrite.getCoordinates(), faccWrite.getAccessRelation());



      //writebackLocalStmt->setWhereMap(homeRel.applyRange(relWrite.reverse()).reverse().take());
      //writebackLocalStmt->addAccess(MemoryAccess::READ, 
      //writebackLocalStmt->addAccess(MemoryAccess::MUST_WRITE, 

      //scop->addScopStmt(writebackLocalStmt);
      modifiedScop();

      // Do not execute the stmt this is ought to replace anymore
      auto oldDomain = getIterationDomain(stmtWrite) ; /* { write_stmt[domain] } */
      faccWrite.getPollyScopStmt()->setDomain(oldDomain.subtract(writeAlreadyHomeDomain).take());

      auto stmtWriteCtx = getScopStmtContext(stmtWrite);
      stmtWriteCtx->validate();
      writebackLocalStmtCtx->validate();

      modifiedScop();


      // CodeGen to transfer data to their home location

      // 1. Create a buffer
      // 2. Write the data into that buffer
      // 3. Send buffer
      // 4. Receive buffer
      // 5. Writeback buffer data to home location

      writebackLocalStmtEditor.getTerminator();
#pragma endregion
#endif
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
        runPass( polly::createPlutoOptimizerPass());
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

    llvm::Pass *asPass() LLVM_OVERRIDE {
      return this;
    }

    bool runOnScop(Scop &S) LLVM_OVERRIDE {
      return false;
    }


  private:
    SCEVExpander scevCodegen;
    //std::map<const isl_id *, Value *> paramToValue;

    isl::Space getParamsSpace() LLVM_OVERRIDE {
      //TODO: All parameters should be collected in an initialization phase, thereafter nothing needs to be realigned
      scop->realignParams();
      return enwrap(scop->getParamSpace()).getParamsSpace();
    }

    const SmallVector<const SCEV *, 8> &getParamSCEVs() LLVM_OVERRIDE {
      return scop->getParams();
    }

    /// SCEVExpander remembers which SCEVs it already expanded in order to reuse them; this is why it is here
    llvm::Value *codegenScev(const llvm::SCEV *scev, llvm::Instruction *insertBefore) LLVM_OVERRIDE {
      return scevCodegen.expandCodeFor(scev, nullptr, insertBefore);
    }


    //llvm::Value *allocStackSpace(llvm::Type *ty) LLVM_OVERRIDE {

    // }


    /// ScalarEvolution remembers its SCEVs
    const llvm::SCEV *scevForValue(llvm::Value *value) LLVM_OVERRIDE {
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
    isl::Id getIdForLoop(const Loop *loop) LLVM_OVERRIDE {
      auto &id = storedDomIds[loop];
      if (id.isValid())
        return id;

      auto depth = loop->getLoopDepth();
      loop->getCanonicalInductionVariable();
      auto indvar = loop->getCanonicalInductionVariable();
      id = islctx->createId("dom" + Twine(depth-1), loop);
      return id;
    }


    /// Here, polly::Scop is supposed to remember (or create) them
    isl::Id idForSCEV(const llvm::SCEV *scev) LLVM_OVERRIDE {
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
    void dump() const LLVM_OVERRIDE {
      dbgs() << "ScopProcessor:\n";
      if (scop)
        scop->dump();
    }


    void validate() const LLVM_OVERRIDE {
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
      for (auto i = nDims-nDims; i<nDims; i+=1) {
        auto scev = getClusterCoordinate(i);
        NewParameters.push_back(scev);
      }
      scop->addParams(NewParameters);

      auto currentNodeCoordSpace = isl::Space::createMapFromDomainAndRange(0, clusterSpace);
      currentNodeCoordSpace.alignParams_inplace(enwrap(scop->getParamSpace()));
      currentNodeCoord = currentNodeCoordSpace.createZeroMultiAff();
      for (auto i = nDims-nDims;i<nDims;i+=1) {
        auto scev = NewParameters[i];
        auto id = enwrap(scop->getIdForParam(scev));
        currentNodeCoord.setAff_inplace(i, currentNodeCoordSpace.getDomainSpace().createAffOnParam(id));
      }
    }


    isl::MultiAff getCurrentNodeCoordinate() LLVM_OVERRIDE {
      prepareCurrentNodeCoordinate();
      return currentNodeCoord;
    }

  }; // class MollyScopContextImpl
} // namespace


char MollyScopContextImpl::ID = '\0';
MollyScopProcessor *MollyScopProcessor::create(MollyPassManager *pm, polly::Scop *scop) {
  return new MollyScopContextImpl(pm, scop);
}
