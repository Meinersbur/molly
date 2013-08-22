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

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace {

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
    MollyScopContextImpl(MollyPassManager *pm, Scop *scop) : ScopPass(ID), pm(pm), scop(scop), changedScop(false), scevCodegen(*pm->findOrRunAnalysis<ScalarEvolution>(nullptr, &scop->getRegion()), "scopprocessor") {
      func = molly::getParentFunction(scop);
      islctx = pm->getIslContext();
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


    const SCEV *getClusterCoordinate(int i) {
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
      for (auto i = 0; i < nClusterDims; i+=1) {
        result.push_back(getClusterCoordinate(i));
      }
      return result;
    }


  protected:
    template<typename Analysis> 
    Analysis *findOrRunAnalysis() {
      return pm->findOrRunAnalysis<Analysis>(func, &scop->getRegion());
    }

    Region *getRegion() {
      return &scop->getRegion();
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
      auto home = fieldTy->getHomeAff(); /* field coord -> cluster coord */
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

      SE = nullptr;
    }
#pragma endregion


#pragma region Scop CommGen
    static void spreadScatterings(Scop *scop, int multiplier) {
      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;
        auto scattering = getScattering(stmt);
        auto space = scattering.getSpace();
        auto nParam = space.getParamDimCount();
        auto nIn = space.getInDimCount();
        auto nOut = space.getOutDimCount();
        auto newScattering = space.emptyMap();

        for (auto bmap : scattering.getBasicMaps()) {
          auto newbmap = space.universeBasicMap();
          for (auto c : bmap.getConstraints()) {
            c.setConstant_inplace(c.getConstant()*multiplier);
            for (auto i = nParam-nParam; i < nParam; i+=1) {
              c.setCoefficient_inplace(isl_dim_param, i, c.getCoefficient(isl_dim_param, i)*multiplier);
            }
            for (auto i = nIn-nIn; i < nIn; i+=1) {
              c.setCoefficient_inplace(isl_dim_in, i, c.getCoefficient(isl_dim_in, i)*multiplier);
            }
            newbmap.addConstraint_inplace(c);
          }
          newScattering = unite(move(newScattering), newbmap);
        }

        stmt->setScattering(newScattering.take());
      }
    }

  private:
    DenseMap<const isl_id*, ScopStmt*> tupleToStmt;
    isl::Id epilogueId;
    isl::Set beforeScopScatterRange;
    isl::Set afterBeforeScatterRange;

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
      spreadScatterings(scop, 2);

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

      auto paramSpace = isl::enwrap(scop->getParamSpace());

      auto readAccesses = paramSpace.createEmptyUnionMap(); /* { stmt[iteration] -> access[indexset] } */
      auto writeAccesses = readAccesses.copy(); /* { stmt[iteration] -> access[indexset] } */
      auto schedule = readAccesses.copy(); /* { stmt[iteration] -> scattering[scatter] } */


      for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
        auto stmt = *itStmt;

        auto facc = getFieldAccess(stmt);
        if (!facc.isValid())
          continue; // Does not contain a field access

        auto domainTuple = getDomainTuple(stmt);
        auto domain = getIterationDomain(stmt); /* { stmt[domain] } */
        assert(domain.getSpace().matchesSetSpace(domainTuple));

        auto scattering = getScattering(stmt); /* { stmt[domain] -> scattering[scatter] }  */
        assert(scattering.getSpace().matchesMapSpace(domainTuple, scatterTuple));
        scattering.intersectDomain_inplace(domain);

        auto fvar = facc.getFieldVariable();
        auto fty = facc.getFieldType();
        auto indexsetTuple = fty->getIndexsetTuple();

        assert(!tupleToFty.count(indexsetTuple.keep()) || tupleToFty[indexsetTuple.keep()]==fty);
        tupleToFty[indexsetTuple.keep()] = fty;

        auto accessRel = facc.getAccessRelation(); /*  { stmt[domain] -> field[indexset] } */
        assert(accessRel.getSpace().matchesMapSpace(domainTuple, indexsetTuple));
        accessRel.intersectDomain_inplace(domain);

        if (facc.isRead()) {
          readAccesses.addMap_inplace(accessRel);
        }
        if (facc.isWrite()) {
          writeAccesses.addMap_inplace(accessRel);
        }
        schedule.addMap_inplace(scattering);
      }

      // To find the data that needs to be written back after the scop has been executed, we add an artificial stmt that reads all the data after everything has been executed
      epilogueId = islctx->createId("epilogue");
      auto epilogueDomainSpace = islctx->createSetSpace(0,0).setSetTupleId(epilogueId);
      auto scatterRangeSpace = getScatteringSpace(scop);
      assert(scatterRangeSpace.isSetSpace());
      auto scatterId = scatterRangeSpace.getSetTupleId();
      auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();
      auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);

      auto allScatters = range(schedule);
      //assert(allScatters.nSet()==1);
      auto scatterRange = allScatters.extractSet(scatterRangeSpace); 
      assert(scatterRange.isValid());
      auto nScatterDims = scatterRange.getDimCount();

      auto max = scatterRange.dimMax(0) + 1;
      max.setInTupleId_inplace(scatterId);
      isl::Set epilogueDomain = epilogueDomainSpace.universeSet();
      auto epilogieMapToZero = epilogueScatterSpace.createUniverseBasicMap();
      for (auto d = 1; d < nScatterRangeDims; d+=1) {
        epilogieMapToZero.addConstraint_inplace(epilogueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
      }

      assert(!scatterRange.isEmpty());
      auto min = scatterRange.dimMin(0) - 1; /* { [1] } */
      min.addDims_inplace(isl_dim_in, nScatterDims);
      min.setInTupleId_inplace(scatterId); /* { scattering[nScatterDims] -> [1] } */
      auto beforeScopScatter = scatterRangeSpace.mapsTo(scatterRangeSpace).createZeroMultiPwAff(); /* { scattering[nScatterDims] -> scattering[nScatterDims] } */
      beforeScopScatter.setPwAff_inplace(0, min);
      beforeScopScatterRange = beforeScopScatter.toMap().getRange();

      auto afterBeforeScatter = beforeScopScatter.setPwAff(1, scatterRangeSpace.createConstantAff(1));
      afterBeforeScatterRange = afterBeforeScatter.toMap().getRange();

      // Insert fake read access after scop
      auto epilogueScatter = max.toMap(); //scatterSpace.createMapFromAff(max);
      epilogueScatter.addDims_inplace(isl_dim_out, nScatterRangeDims - 1);
      //epilogueScatter.setTupleId(isl_dim_out, scatterRangeSpace.getTupleId(isl_dim_set));
      epilogueScatter.setInTupleId_inplace(epilogueId);
      epilogueScatter.setOutTupleId_inplace(scatterId);
      epilogueScatter.intersect_inplace(epilogieMapToZero);

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
      isl::computeFlow(readAccesses.copy(), writeAccesses.copy(), islctx->createEmptyUnionMap(), schedule.copy(), &mustFlow, &mayFlow, &mustNosrc, &mayNosrc);
      assert(mayNosrc.isEmpty());
      assert(mayFlow.isEmpty());
      //TODO: verify that computFlow generates direct dependences

      auto inputFlow = unite(mustNosrc, mayNosrc);
      auto stmtFlow = mustFlow.getSpace().createEmptyUnionMap();
      auto outputFlow = mustFlow.getSpace().createEmptyUnionMap();
      for (auto dep : mustFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        //auto itDomainWrite = dep.getDomain(); /* write_acc[domain] */
        //auto itDomainRead = dep.getRange(); /* read_acc[domain] */
        if (dep.getOutTupleId() == epilogueId) {
          outputFlow.addMap_inplace(dep);
        } else {
          stmtFlow.addMap_inplace(dep);
        }
      }


      for (auto dep : inputFlow.getMaps()) { /* dep: { stmtRead[domain] -> field[indexset] } */
        // Value source is outside this scop 
        // Value must be read from home location
        genInputCommunication(dep.getDomain());
      }

      for (auto dep : stmtFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
        genFlowCommunication(dep);
      }

      for (auto dep : outputFlow.getMaps()) {
        // This means the data is visible outside the scop and must be written to its home location
        // There is no explicit read access
        assert(dep.getOutTupleId() == epilogueId);

        genOutputCommunication(dep.getDomain());
      }


      //TODO: For every may-write, copy the original value into that the target memory (unless they are the same), so we copy the correct values into the buffers
      // i.e. treat ever may write as, write original value if not written
    }



  private:
    void genInputCommunication(const isl::Set &instances) {
      auto clusterConf = pm->getClusterConfig();
      auto clusterTuple = clusterConf->getClusterTuple();
      auto nClusterDims = clusterConf->getClusterDims();
      auto nodeSpace = clusterConf->getClusterSpace();
      auto nodes = clusterConf->getClusterShape();

      auto stmtTuple = instances.getTupleId();
      //auto fieldTuple = dep.getOutTupleId();
      //assert(dep.getSpace().matchesMapSpace(stmtTuple, fieldTuple));

      auto stmtRead = tupleToStmt[stmtTuple.keep()];
      auto domainSpace = enwrap(stmtRead->getDomainSpace());
      assert(domainSpace.getSetTupleId() == stmtTuple);

      auto facc = getFieldAccess(stmtRead);
      auto fvar = facc.getFieldVariable();
      auto fty = facc.getFieldType();
      //assert(tupleToFty[fieldTuple.keep()] == fty);
      auto fieldTuple = fty->getIndexsetTuple();
      auto indexsetSpace = fty->getIndexsetSpace();
      auto readUsed = facc.getLoadInst();
      auto nFieldDims = fty->getNumDimensions();
      assert(indexsetSpace.getSetDimCount() == nFieldDims);

      auto domainRead = instances;//dep.getDomain(); /* { stmtRead[domain] } */
      assert(domainRead.getSpace().matchesSetSpace(domainSpace));

      auto scatteringRead = getScattering(stmtRead); /* { stmtRead[domain] -> scattering[scatter] } */

      auto accessRel = facc.getAccessRelation(); /* { stmtRead[domain] -> field[indexset] } */
      assert(accessRel.getSpace().matchesMapSpace(stmtTuple, fieldTuple));
      accessRel.intersectDomain_inplace(domainRead);

      auto whereRead = getWhereMap(stmtRead); /* { stmtRead[domain] -> cluster[node] } */
      assert(whereRead.getSpace().matchesMapSpace(domainSpace, nodeSpace));
      auto wherePairs = whereRead.wrap(); /* { (stmtRead[domain] -> rank[node]) } */ // All execution instances of this ScopStmt

      // Find those who are already at their home location
      // For every execution of a stmt on a specific node
      auto homeRel = fty->getHomeRel(); /* { cluster[node] -> field[indexset] } */
      homeRel.setInTupleId_inplace(clusterConf->getClusterTuple());
      assert(homeRel.getSpace().matchesMapSpace(clusterTuple, fieldTuple));

      auto wherePairsAccesses = accessRel.addInDims(nodeSpace.getSetDimCount()); 
      wherePairsAccesses.cast_inplace(wherePairs.getSpace().mapsTo(homeRel.getSpace().range()));
      wherePairsAccesses = wherePairsAccesses.intersectDomain(wherePairs); /* { (stmtRead[domain] -> cluster[node]) -> field[indexset] } */
      auto c = wherePairsAccesses.chain(homeRel.reverse()); /* { ((stmtRead[domain] -> cluster[node]) -> field[indexset]) -> cluster[node] } */ // Oh my goodness
      // condition: node on which read is executed==node the read value is home
      auto homePairAccesses = c;
      for (auto i = 0; i < nClusterDims; i+=1) {
        homePairAccesses.equate_inplace(isl_dim_in, domainRead.getDimCount() + i, isl_dim_out, i);
      }
      auto alreadyHomeAccesses = homePairAccesses.getDomain().unwrap(); /* { (stmtRead[domain] -> cluster[node]) -> field[indexset] } */
      auto notHomeAccesses = wherePairs.subtract(alreadyHomeAccesses.getDomain()).chain(wherePairsAccesses); /* { (stmtRead[domain] -> cluster[node]) -> field[indexset] } */


      ScopEditor editor(scop);

      // CodeGen already home
      if (!alreadyHomeAccesses.isEmpty()) {
        auto readLocalEdit = editor.replaceStmt(stmtRead, alreadyHomeAccesses.getDomain().unwrap(), "InputLocal");
        auto bb = readLocalEdit.getBasicBlock();

        //BasicBlock *bb = BasicBlock::Create(llvmContext, "InputLocal", func);
        DefaultIRBuilder builder(bb);

        auto val = codegenReadLocal(builder, fvar, facc.getCoordinates());
        readUsed->replaceAllUsesWith(val); 
        readLocalEdit.getTerminator();
        // TODO: Don't assume every stmt accessed just one value
        //auto replacementStmt = replaceScopStmt(stmtRead, bb, "home_input_flow", alreadyHomeAccesses.getDomain().unwrap());
        //scop->addScopStmt(replacementStmt);

        //TODO: Create polly::MemoryAccess
      }

      // CodeGen from other nodes
      if (!notHomeAccesses.isEmpty()) {
        // Where is the home node?
        auto notHomeLocation = wherePairsAccesses.intersectDomain(notHomeAccesses.wrap()).curry(); /* { (stmtRead[domain] -> dst[node]) -> (field[indexset] -> src[node]) } */
        auto instancesRecv = notHomeLocation.getDomain().unwrap(); /* { stmtRead[domain] -> cluster[node] } */
        auto dataSource = notHomeLocation.getRange().unwrap(); /* { field[indexset] -> cluster[node] } */

        // Create the communication buffer 
        auto dataToSend = dataSource.reverse(); /* { cluster[node] -> field[indexset] } */
        auto sendRelPerm = notHomeLocation.curry().getRange().unwrap(); /* { dst[node] -> (field[indexset] -> src[node]) } */

        unsigned perm[] = { nClusterDims+nFieldDims, 0, nClusterDims };
        auto sendRel = sendRelPerm.wrap().permuteDims(perm).unwrap() ; /* { (src[node] -> dst[node]) -> field[indexset] } */
        auto combuf = pm->newCommunicationBuffer(fty, sendRel.copy());

        // Send on source node
        // Copy to buffer
        auto copyArea = product(nodes, combuf->getRelation().getRange()); /* { [cluster,indexset] } */
        auto copyAreaSpace = copyArea.getSpace();
        auto copyScattering = islctx->createAlltoallMap(copyArea, beforeScopScatterRange);
        auto copyWhere = fty->getHomeRel(); // Execute where that value is home
        auto memmovEditor = editor.createStmt(copyArea.copy(), copyScattering.copy(), copyWhere.copy(), "memmove_nonhome");
        auto memmovStmt = memmovEditor.getStmt();
        BasicBlock *memmovBB = memmovStmt->getBasicBlock();

        //BasicBlock::Create(llvmContext, "memmove_nonhome", func);
        DefaultIRBuilder memmovBuilder(memmovBB);
        auto copyValue = codegenReadLocal(memmovBuilder, fvar, facc.getCoordinates());
        //auto memmovStmt = createScopStmt(scop, memmovBB, stmtRead->getRegion(), "memmove_nonhome", stmtRead->getLoopNests()/*not accurate*/, );
        std::map<isl_id *, llvm::Value *> scalarMap;
        editor.getParamsMap(scalarMap, memmovStmt);
        combuf->codegenWriteToBuffer(memmovBuilder, scalarMap, copyValue, facc.getCoordinates()/*correct?*/ );
        memmovBuilder.CreateUnreachable(); // Terminator, removed by Scop code generator

        // Execute send
        auto singletonDomain = islctx->createSetSpace(0, 0).universeBasicSet();
        auto sendStmtEditor = editor.createStmt(singletonDomain.copy(), islctx->createAlltoallMap(singletonDomain, afterBeforeScatterRange), homeRel.copy(), "send_nonhome");
        BasicBlock *sendBB = sendStmtEditor.getBasicBlock();
        DefaultIRBuilder sendBuilder(sendBB);

        auto nodeCoords =  ArrayRef<Value*>(facc.getCoordinates()).slice(0, nodes.getSetDimCount());
        auto dstRank = clusterConf->codegenComputeRank(sendBuilder, nodeCoords);
        codegenSendBuf(sendBuilder, combuf, dstRank);
        sendBuilder.CreateUnreachable();

        // Execute Recv
        auto recvDomain = combuf->getRelation().getDomain(); /* { (src[coord], dst[coord]) } */
        auto recvUserScatter = scatteringRead ;
        auto recvScatter = recvUserScatter ; /* { (src[coord], dst[coord]) -> scattering[scatter] } */
        auto recvWhere = recvDomain.unwrap().rangeMap(); /* { (src[coord], dst[coord]) -> dst[coord] } */
        auto recvStmtEditor = editor.createStmt(recvDomain.copy(), recvScatter.copy(), recvWhere.copy(), "recv_nonhome" );

        // Use the data where needed
        auto useWhere = notHomeAccesses.getRange().unwrap(); /* { readStmt[domain] -> cluster[coord] } */
        auto useEditor = editor.replaceStmt(stmtRead,useWhere.copy() , "use_nonhome");
        IRBuilder<> useBuilder(useEditor.getTerminator());
        auto useValue = combuf->codegenReadFromBuffer(useBuilder, scalarMap, facc.getCoordinates());
        auto useLoad = facc.getLoadUse();
        useBuilder.CreateStore(useValue, useLoad->getPointerOperand());

        //TODO: Create polly::MemoryAccess
      }
    }


    MollyScopStmtProcessor *getScopStmtContext(const isl::Id &id) {
      auto stmt = tupleToStmt[id.keep()];
      assert(stmt);
      return getScopStmtContext(stmt);
    }


    private:
    /// return the first dimension from which on all dependencies are always lexicographically positive
    /// examples:
    /// { (1,0,0) -> (0,1,0) } returns 1
    /// { (0,i,0) -> (0,i+1,-1) } returns 1
    /// { (0,0) -> (0,0) } returns 2, these are never lexicographically positive
    unsigned computeLevelOfInpendence(const isl::Map &depScatter) {  /* dep: { scattering[scatter] -> scattering[scatter] } */
      auto depMap = depScatter.getSpace();
      auto dependsVector = depMap.emptyMap(); /* { read[scatter] -> (read[scatter] - write[scatter]) } */
      auto nParam = depScatter.getParamDimCount();
      auto nIn = depScatter.getInDimCount();
      auto nOut = depScatter.getOutDimCount();
      assert(nIn==nOut);
      auto scatterSpace = depScatter.getDomainSpace(); /* { scattering[scatter] } */
      auto nCompareDims = std::min(nIn,nOut);

        // get the first always-positive coordinate such that the dependence is satisfied
      // Everything after that doesn't matter anymore to meet that dependence
      for (auto i = nCompareDims-nCompareDims; i < nCompareDims; i+=1) {
        auto orderMap = depMap.universeBasicMap();
        orderMap.orderLt_inplace(isl_dim_in, i, isl_dim_out, i);
           // auto fulfilledDeps = depScatter.intersect(orderMap);
        if (orderMap >= depScatter) {
           // All dependences are fullfilled from here
          // i.e. we can shuffle stuff as we want in lower dimensions
          return i;
        }
      }

      // No always-positive dependence, therefore return the first out-of-range
      return nCompareDims;

#if 0
      for (auto basicDep : depScatter.getBasicMaps()) {
        auto dependsBasicVector = basicDep.getSpace().universeBasicMap();
        for (auto constraint : basicDep.getConstraints()) {
          for (auto i = nIn-nIn; i < nIn; i+=1) {
            auto valRead = constraint.getCoefficient(isl_dim_in, i);
            auto valWrite = constraint.getCoefficient(isl_dim_out, i);
            constraint.setCoefficient_inplace(isl_dim_out, i, valRead - valWrite);
          }
          dependsBasicVector.addConstraint_inplace(constraint);
        }
        dependsVector = unite(move(dependsVector), dependsBasicVector);
      }


      // get the first always-positive coordinate such that the dependence is satisfied
      // Everything after that doesn't matter anymore to meet that dependence
      for (auto i = nIn-nIn; i < nIn; i+=1) {
        auto positiveDepVectors = scatterSpace.zeroPoint().apply(scatterSpace.lexGtFirstMap(i));
        auto fullfilledDeps = dependsVector.intersectRange(positiveDepVectors);
        if (fullfilledDeps.isSupersetOf(dependsVector)) {
          // All dependences are fullfilled from here
          // i.e. we can shuffle stuff as we want in lower dimensions
          return i;
        }
      }

      // No always-positive dependence, therefore 
      return nIn;
#endif
    }


    void genFlowCommunication(const isl::Map &dep) { /* dep: { writeStmt[domain] -> readStmt[domain] } */
      auto scatterTupleId = getScatterTuple(scop);

      auto readStmt = getScopStmtContext(dep.getInTupleId());
      assert(readStmt->isReadAccess());
      auto readDomain = readStmt->getDomain();
      assert(readStmt->isFieldAccess());
      auto readFVar = readStmt->getFieldVariable();
      auto readAccRel = readStmt->getAccessRelation(); /* { readStmt[domain] -> field[index] } */
      assert(readAccRel.getSpace().matchesMapSpace(readDomain.getSpace(), readFVar->getFieldType()->getIndexsetSpace()));
      auto readScatter = readStmt->getScattering();
      assert(readScatter.getSpace().matchesMapSpace(readDomain.getSpace(), scatterTupleId));
      //auto readFieldTuple = readAccRel.getOutTupleId();
      //readFieldTuple = islctx->createId(Twine() + readFieldTuple.getName() + "_read", readFVar);
      //readAccRel.setOutTupleId_inplace(readFieldTuple); /* { readStmt[domain] -> field_read[index] } */
      auto readWhere = readStmt->getWhere(); // { stmtRead[domain] -> node[cluster] }

      auto writeStmt = getScopStmtContext(dep.getOutTupleId());
      assert(writeStmt->isReadAccess());
      auto writeDomain = writeStmt->getDomain();
      assert(writeStmt->isFieldAccess());
      auto writeFVar = writeStmt->getFieldVariable();
      auto writeAccRel = writeStmt->getAccessRelation(); /* { writeStmt[domain] -> field[index] } */
      assert(writeAccRel.getSpace().matchesMapSpace(writeDomain.getSpace(), writeFVar->getFieldType()->getIndexsetSpace()));
      auto writeScatter = writeStmt->getScattering();
      assert(writeScatter.getSpace().matchesMapSpace(writeDomain.getSpace(), scatterTupleId));
      //auto writeFieldTuple = writeAccRel.getOutTupleId();
      //writeFieldTuple = islctx->createId(Twine() + writeFieldTuple.getName() + "_write", writeFVar);
      //writeAccRel.setOutTupleId_inplace(writeFieldTuple); /* { writeStmt[domain] -> field_write[index] } */
      auto writeWhere = writeStmt->getWhere(); // { writeRead[domain] -> node[cluster] }

     // A statment is either reading writing, but not both
      assert(readStmt != writeStmt);

      // flow is data transfer from a writing statement to a reding statement of the same location, i.e. also same field
      assert(readFVar == writeFVar);
      auto fvar = readFVar;
      auto fty = fvar->getFieldType();
       

      auto scatterDep = dep.applyDomain(writeScatter); /* { scatteringWrite[scatter] -> scatteringRead[scatter] } */
      scatterDep.applyRange_inplace(readScatter);
      auto levelOfIndep = computeLevelOfInpendence(scatterDep);

#if 0
      //auto table = dep.chain(readAccRel);  /* { (writeStmt[domain] -> readStmt[domain]) -> field_read[index] } */
      //table = table.chainNested(writeAccRel); /* (writeStmt[domain] -> readStmt[domain] -> field_read[index]) -> field_write[index] } */


      auto scatterRangeSpace = getScatteringSpace(scop);
      auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();

      auto itDomainWrite = dep.getDomain(); /* { write_acc[domain] } */
      auto writeTuple = itDomainWrite.getTupleId();
      auto stmtWrite = tupleToStmt[writeTuple.keep()];
      assert(stmtWrite);
      auto faccWrite =  getFieldAccess(stmtWrite);
      auto memaccWrite = faccWrite.getPollyMemoryAccess();
      auto scatterWrite = getScattering(stmtWrite); /* { write_stmt[domain] -> scattering[scatter] } */
      auto relWrite = faccWrite.getAccessRelation(); /* { write_stmt[domain] -> field[indexset] } */
      auto domainWrite = itDomainWrite.setTupleId(isl::Id::enwrap(stmtWrite->getTupleId())); /* write_stmt[domain] */
      //auto fvar = faccWrite.getFieldVariable();
      //auto fty = fvar->getFieldType();

      auto itDomainRead = dep.getRange();
      auto itReadTuple = itDomainRead.getTupleId();
      auto readStmt = tupleToStmt[itReadTuple.keep()];

      auto faccRead = getFieldAccess(readStmt);
      auto stmtRead = faccRead.getPollyScopStmt();
      auto scatterRead = faccRead.getAccessScattering();

      auto scatter = unite(scatterWrite, scatterRead);

      assert(scatterWrite.getSpace() == scatterRead.getSpace());
      auto scatterSpace = scatterRead.getSpace();

      // Determine the level of independence
      auto depScatter = dep.applyDomain(getScattering(stmtWrite)).applyRange(getScattering(stmtRead)); /* { write[scatter] -> read[scatter] } */
      depScatter.reverse_inplace();

      auto dependsVector = depScatter.getSpace().emptyMap(); /* { read[scatter] -> (read[scatter] - write[scatter]) } */
      auto nParam = depScatter.getParamDimCount();
      auto nIn = depScatter.getInDimCount();
      auto nOut = depScatter.getOutDimCount();
      assert(nIn==nOut);
      for (auto basicDep : depScatter.getBasicMaps()) {
        //TODO: Faster using (in)equality matrices
        auto dependsBasicVector = basicDep.getSpace().universeBasicMap();
        for (auto constraint : basicDep.getConstraints()) {
          for (auto i = nIn-nIn; i < nIn; i+=1) {
            auto valRead = constraint.getCoefficient(isl_dim_in, i);
            auto valWrite = constraint.getCoefficient(isl_dim_out, i);
            constraint.setCoefficient_inplace(isl_dim_out, i, valRead - valWrite);
          }
          dependsBasicVector.addConstraint_inplace(constraint);
        }
        dependsVector = unite(move(dependsVector), dependsBasicVector);
      }

      // get the first always-positive coordinate such that the dependence is satisfied
      // Everything after that doesn't matter anymore to meet that dependence
      unsigned order = -1;
      for (auto i = nIn-nIn; i < nIn; i+=1) {
        auto positiveDepVectors = scatterSpace.zeroPoint().apply(scatterSpace.lexGtFirstMap(i));
        auto fullfilledDeps = dependsVector.intersectRange(positiveDepVectors);
        if ( fullfilledDeps.isSupersetOf(dependsVector)) {
          // All dependences are fullfilled from here
          // i.e. we can shuffel stuff as we want in lower dimensions
          order = i;
          break;
        }
      }
      assert(order!=-1);

      //FIXME: what if fullfiled only at last dimension?
      auto insertPos = order+1;
      auto ignored = nScatterRangeDims-insertPos;

      auto indepScatter = scatter.moveDims(isl_dim_in, scatter.getInDimCount(), isl_dim_in, 0, order);

      auto indepMin = indepScatter.lexminPwMultiAff();
      auto indepMax = indepScatter.lexmaxPwMultiAff();

      //indepMin
#endif
    }

    void genOutputCommunication(const isl::Set &instances) {
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
    }


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

    llvm::Value *codegenScev( const llvm::SCEV *scev, llvm::Instruction *insertBefore) {
      return scevCodegen.expandCodeFor(scev, nullptr, insertBefore);
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

  }; // class MollyScopContextImpl
} // namespace


char MollyScopContextImpl::ID = '\0';
MollyScopProcessor *MollyScopProcessor::create(MollyPassManager *pm, polly::Scop *scop) {
  return new MollyScopContextImpl(pm, scop);
}
