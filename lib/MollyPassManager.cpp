#define DEBUG_TYPE "molly"
#include "MollyPassManager.h"

#include <llvm/Support/Debug.h>
#include <llvm/Pass.h>
#include <polly/PollyContextPass.h>
#include "MollyUtils.h"
#include <llvm/ADT/DenseSet.h>
#include "MollyContextPass.h"
#include <polly/ScopInfo.h>
#include <polly/LinkAllPasses.h>
#include "FieldDistribution.h"
#include "FieldDetection.h"
#include "FieldCodeGen.h"
#include "ClusterConfig.h"
#include <llvm/Support/CommandLine.h>
#include "molly/RegisterPasses.h"
#include "MollyContextPass.h"
#include <polly/RegisterPasses.h>
#include <llvm/Support/Debug.h>
#include "FieldType.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <polly/ScopInfo.h>
#include "MollyUtils.h"
#include <llvm/Support/ErrorHandling.h>
#include "ScopUtils.h"
#include "MollyFieldAccess.h"
#include "islpp/Set.h"
#include "islpp/Map.h"
#include <llvm/Analysis/ScalarEvolution.h>
#include "FieldVariable.h"
#include <llvm/IR/Intrinsics.h>
#include "islpp/UnionMap.h"
#include "ScopEditor.h"
#include "islpp/Point.h"
#include <polly/TempScopInfo.h>
#include <polly/LinkAllPasses.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <polly/CodeGen/BlockGenerators.h>
#include "llvm/ADT/StringRef.h"
#include "MollyIntrinsics.h"
#include "CommunicationBuffer.h"
#include <clang/CodeGen/MollyRuntimeMetadata.h>
#include "IslExprBuilder.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace molly {

  class MollyPassManager : public llvm::ModulePass {
    typedef IRBuilder<> BuilderTy;

#pragma region AnalysisResolver
    class MollyResolver : public AnalysisResolver {
    protected:
      MollyPassManager *pm;

      MollyResolver(MollyPassManager *pm) : AnalysisResolver(*static_cast<PMDataManager*>(nullptr)), pm(pm) {
        assert(pm);
      }

    protected:
      virtual Pass *getPass(AnalysisID PI, bool ifavailable) const = 0;

    public:
      Pass *findImplPass(Pass *P, AnalysisID PI, Function &F) LLVM_OVERRIDE {
        return findImplPass(P, PI, F);
      }

      Pass *findImplPass(AnalysisID PI) LLVM_OVERRIDE {
        return getPass(PI, false);
      }

      Pass *getAnalysisIfAvailable(AnalysisID ID, bool Direction) const LLVM_OVERRIDE {
        return getPass(ID, true);
      }
    }; // class MollyResolver


    class MollyModuleResolver : public MollyResolver {
    protected:
      Pass *getPass(AnalysisID PI, bool ifavailable) const LLVM_OVERRIDE {
        return ifavailable ? pm->findAnalysis(PI) : pm->findOrRunAnalysis(PI);
      }
    public:
      MollyModuleResolver(MollyPassManager *pm) : MollyResolver(pm) {
        assert(pm);
      }
    }; // class MollyModuleResolver

    class MollyFunctionResolver : public MollyResolver {
    private:
      Function *func;

    protected:
      Pass *getPass(AnalysisID PI, bool ifavailable) const LLVM_OVERRIDE {
        return ifavailable ?  pm->findAnalysis(PI, func) : pm->findOrRunAnalysis(PI, func);
      }

    public:
      MollyFunctionResolver(MollyPassManager *pm, Function *func) : MollyResolver(pm) , func(func) {
        assert(pm);
        assert(func);
      }
    }; // class MollyFunctionResolver


    class MollyRegionResolver : public MollyResolver {
    private:
      Region *region;

    protected:
      Pass *getPass(AnalysisID PI, bool ifavailable) const LLVM_OVERRIDE {
        return ifavailable ? pm->findAnalysis(PI, region) : pm->findOrRunAnalysis(PI, region);
      }

    public:
      MollyRegionResolver(MollyPassManager *pm, Region *region) : MollyResolver(pm) , region(region) {
        assert(pm);
        assert(region);
      }
    }; // class MollyRegionResolver
#pragma endregion


#pragma region Contexts
    class MollyScopStmtContext {
    private:
      MollyPassManager *pm;
      ScopStmt *stmt;

    public:
      MollyScopStmtContext(MollyPassManager *pm, ScopStmt *stmt) : pm(pm), stmt(stmt) {}

#pragma region Apply whete to execute stmt

      void applyWhere() {
        auto where = getWhereMap(stmt);
      }

#pragma enregion
    }; // class MollyScopStmtContext


    class MollyScopContext {
    private:
      MollyPassManager *pm;
      Scop *scop;
      Function *func;
      isl::Ctx *islctx;

      ScalarEvolution *SE;

      bool changedScop ;
      void modifiedScop() {
        changedScop = true;
      }

      void runPass(Pass *pass) {
        switch (pass->getPassKind()) {
        case PT_Region:
          pm->runRegionPass( static_cast<RegionPass*>(pass), &scop->getRegion());
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
      MollyScopContext(MollyPassManager *pm, Scop *scop) : pm(pm), scop(scop), changedScop(false) {
        func = getParentFunction(scop);
        islctx = pm->getIslContext();
      }

#pragma region Scop Distribution
      void processFieldAccess(MollyFieldAccess &acc, isl::Map &executeWhereWrite, isl::Map &executeWhereRead) {
        //if (acc.isPrologue() || acc.isEpilogue()) {
        //  return; // These are not computed anywhere
        //}

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

        auto executeWhereWrite = islctx->createEmptyMap(itSpace, pm->clusterConf->getClusterSpace());
        auto executeWhereRead = executeWhereWrite.copy();
        auto executeEverywhere = islctx->createAlltoallMap(itDomain,  pm->clusterConf->getClusterShape());
        auto executeMaster = islctx->createAlltoallMap(itDomain, pm->clusterConf->getMasterRank().toMap().getRange());

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

        result.setOutTupleId_inplace(pm->clusterConf->getClusterTuple());
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


      void genCommunication() {
        auto funcName = func->getName();
        DEBUG(llvm::dbgs() << "run ScopFieldCodeGen on " << scop->getNameStr() << " in func " << funcName << "\n");
        if (func->getName() == "sink") {
          int a = 0;
        }

        //auto func = scop->getRegion().getEntry()->getParent();
        auto &llvmContext = func->getContext();
        auto module = func->getParent();
        auto scatterTuple = getScatterTuple(scop);
        auto clusterTuple = pm->clusterConf->getClusterTuple();
        auto nodeSpace = pm->clusterConf->getClusterSpace();
        auto nodes = pm->clusterConf->getClusterShape();
        auto nClusterDims = nodeSpace.getSetDimCount();

        // Make some space between the stmts in which we can insert our communication
        spreadScatterings(scop, 2);

        // Collect information
        // Like polly::Dependences::collectInfo, but finer granularity (MemoryAccess instead ScopStmt)
        //DenseMap<const isl_id*, polly::MemoryAccess*> tupleToAccess;
        DenseMap<const isl_id*, ScopStmt*> tupleToStmt;
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
        auto epilogueId = islctx->createId("epilogue");
        auto epilogueDomainSpace = islctx->createSetSpace(0,0).setSetTupleId(epilogueId);
        auto scatterRangeSpace = getScatteringSpace(scop);
        assert(scatterRangeSpace.isSetSpace());
        auto scatterId = scatterRangeSpace.getSetTupleId();
        auto nScatterRangeDims = scatterRangeSpace.getSetDimCount();
        auto epilogueScatterSpace = isl::Space::createMapFromDomainAndRange(epilogueDomainSpace, scatterRangeSpace);

        auto allScatters = range(schedule);
        assert(allScatters.nSet()==1);
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

        auto min = scatterRange.dimMin(0) - 1; /* { [1] } */
        min.addDims_inplace(isl_dim_in, nScatterDims);
        min.setInTupleId_inplace(scatterId); /* { scattering[nScatterDims] -> [1] } */
        auto beforeScopScatter = scatterRangeSpace.mapsTo(scatterRangeSpace).createZeroMultiPwAff(); /* { scattering[nScatterDims] -> scattering[nScatterDims] } */
        beforeScopScatter.setPwAff_inplace(0, min);
        auto beforeScopScatterRange = beforeScopScatter.toMap().getRange();

        auto afterBeforeScatter = beforeScopScatter.setPwAff(1, scatterRangeSpace.createConstantAff(1));
        auto afterBeforeScatterRange = afterBeforeScatter.toMap().getRange();

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

          auto stmtTuple = dep.getInTupleId();
          auto fieldTuple = dep.getOutTupleId();
          assert(dep.getSpace().matchesMapSpace(stmtTuple, fieldTuple));

          auto stmtRead = tupleToStmt[stmtTuple.keep()];
          auto domainSpace = enwrap(stmtRead->getDomainSpace());
          assert(domainSpace.getSetTupleId() == stmtTuple);

          auto facc = getFieldAccess(stmtRead);
          auto fvar = facc.getFieldVariable();
          auto fty = facc.getFieldType();
          assert(tupleToFty[fieldTuple.keep()] == fty);
          auto indexsetSpace = fty->getIndexsetSpace();
          auto readUsed = facc.getLoadInst();
          auto nFieldDims = fty->getNumDimensions();
          assert(indexsetSpace.getSetDimCount() == nFieldDims);

          auto domainRead = dep.getDomain(); /* { stmtRead[domain] } */
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
          homeRel.setInTupleId_inplace(pm->clusterConf->getClusterTuple());
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


          // CodeGen already home
          if (!alreadyHomeAccesses.isEmpty()) {
            BasicBlock *bb = BasicBlock::Create(llvmContext, "InputLocal", func);
            IRBuilder<> builder(bb);

            auto val = codegenReadLocal(builder, fvar, facc.getCoordinates());
            readUsed->replaceAllUsesWith(val); 

            // TODO: Don't assume every stmt accessed just one value
            auto replacementStmt = replaceScopStmt(stmtRead, bb, "home_input_flow", alreadyHomeAccesses.getDomain().unwrap());
            scop->addScopStmt(replacementStmt);
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


            ScopEditor editor(scop);
            // Send on source node
            // Copy to buffer
            auto copyArea = product(nodes, combuf->getRelation().getRange()); /* { [cluster,indexset] } */
            auto copyAreaSpace = copyArea.getSpace();
            auto copyScattering = islctx->createAlltoallMap(copyArea, beforeScopScatterRange);
            auto copyWhere = fty->getHomeRel(); // Execute where that value is home
            auto memmovStmt = editor.createBlock(copyArea.copy(), copyScattering.copy(), copyWhere.copy(), "memmove_nonhome");
            BasicBlock *memmovBB = memmovStmt->getBasicBlock();

            //BasicBlock::Create(llvmContext, "memmove_nonhome", func);
            IRBuilder<> memmovBuilder(memmovBB);
            auto copyValue = codegenReadLocal(memmovBuilder, fvar, facc.getCoordinates());
            //auto memmovStmt = createScopStmt(scop, memmovBB, stmtRead->getRegion(), "memmove_nonhome", stmtRead->getLoopNests()/*not accurate*/, );
            std::map<isl_id *, llvm::Value *> scalarMap;
            editor.getParamsMap(scalarMap, memmovStmt);
            combuf->codegenWriteToBuffer(memmovBuilder, scalarMap, copyValue, facc.getCoordinates()/*correct?*/ );
            memmovBuilder.CreateUnreachable(); // Terminator, removed by Scop code generator

            // Execute send
            auto singletonDomain = islctx->createSetSpace(0, 0).universeBasicSet();
            auto sendStmtEditor = editor.createStmt( singletonDomain.copy(), islctx->createAlltoallMap(singletonDomain,afterBeforeScatterRange), homeRel.copy(), "send_nonhome");
            BasicBlock *sendBB = sendStmtEditor.getBasicBlock();
            IRBuilder<> sendBuilder(sendBB);

            auto nodeCoords =  ArrayRef<Value*>(facc.getCoordinates()).slice(0, nodes.getSetDimCount());
            auto dstRank = pm->clusterConf->codegenComputeRank(sendBuilder, nodeCoords);
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
          }
        }


        for (auto dep : outputFlow.getMaps()) {
          assert(dep.getOutTupleId() == epilogueId);
          // This means the data is visible outside the scop and must be written to its home location
          // There is no explicit read access

          auto itDomainWrite = dep.getDomain(); /* write_acc[domain] */
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
          BasicBlock *bb = BasicBlock::Create(llvmContext, "OutputWrite", func);
          IRBuilder<> builder(bb);

          builder.CreateStore(val, stackMem); // stackMem contains a buffer with the value


          SmallVector<Type*, 8> tys;
          SmallVector<Value*, 8> args;
          tys.push_back(fty->getType()->getPointerTo()); // target field
          args.push_back(faccWrite.getFieldPtr());
          tys.push_back(fty->getEltPtrType()); // source buffer
          args.push_back(stackMem);
          auto nDims = alreadyHome.getDimCount();
          for (auto i = 0; i < nDims; i+=1) {
            tys.push_back(Type::getInt32Ty(llvmContext));
            args.push_back( faccWrite.getCoordinate(i));
          }
          auto intrSetLocal = Intrinsic::getDeclaration(module, Intrinsic::molly_set_local, tys);
          builder.CreateCall(intrSetLocal, args);
          builder.CreateBr(bb); // This is a dummy branch; at the moment this is dead code, but Polly's code generator will hopefully incorperate it

          auto leastmostOne = scatterSpace.mapsTo(nScatterDims).createZeroMultiAff().setAff(nScatterDims-1, scatterSpace.createConstantAff(1));
          auto afterWrite = scatterWrite.sum(scatterWrite.applyRange(leastmostOne).setOutTupleId(scatterWrite.getOutTupleId())).intersectDomain(writeAlreadyHomeDomain);
          auto writebackLocalStmt = createScopStmt(scop, bb, stmtWrite->getRegion(), "writeback_local", stmtWrite->getLoopNests(), writeAlreadyHomeDomain.copy(), afterWrite.move());
          //writebackLocalStmt->addAccess(MemoryAccess::READ, 
          //writebackLocalStmt->addAccess(MemoryAccess::MUST_WRITE, 

          scop->addScopStmt(writebackLocalStmt);
          modifiedScop();

          // Do not execute the stmt this is ought to replace anymore
          auto oldDomain = getIterationDomain(stmtWrite) ; /* { write_stmt[domain] } */
          faccWrite.getPollyScopStmt()->setDomain(oldDomain.subtract(writeAlreadyHomeDomain).take());
          modifiedScop();

          // CodeGen to transfer data to their home location

          // 1. Create a buffer
          // 2. Write the data into that buffer
          // 3. Send buffer
          // 4. Receive buffer
          // 5. Writeback buffer data to home location



#pragma endregion
        }


        for (auto dep : stmtFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
          auto itDomainWrite = dep.getDomain(); /* { write_acc[domain] } */
          auto writeTuple = itDomainWrite.getTupleId();
          auto stmtWrite = tupleToStmt[writeTuple.keep()];
          assert(stmtWrite);
          auto faccWrite =  getFieldAccess(stmtWrite);
          auto memaccWrite = faccWrite.getPollyMemoryAccess();
          //auto memaccWrite = tupleToAccess[itDomainWrite.getTupleId().keep()];
          //assert(memaccWrite);
          //auto faccWrite = getFieldAccess(memaccWrite);
          //auto stmtWrite = faccWrite.getPollyScopStmt();
          auto scatterWrite = getScattering(stmtWrite); /* { write_stmt[domain] -> scattering[scatter] } */
          auto relWrite = faccWrite.getAccessRelation(); /* { write_stmt[domain] -> field[indexset] } */
          auto domainWrite = itDomainWrite.setTupleId(isl::Id::enwrap(stmtWrite->getTupleId())); /* write_stmt[domain] */
          auto fvar = faccWrite.getFieldVariable();
          auto fty = fvar->getFieldType();

          auto itDomainRead = dep.getRange();
          auto itReadTuple = itDomainRead.getTupleId();
          auto readStmt = tupleToStmt[itReadTuple.keep()];

          //auto memaccRead = tupleToAccess[itDomainRead.getTupleId().keep()];
          // auto faccRead = MollyFieldAccess::fromMemoryAccess(memaccRead);
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
          //auto direction = scatterSpace.universeSet();
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


#if 0
          auto fulfilledPrefix = scatter.projectOut(isl_dim_set, insertPos, ignored);
          auto prefix = fulfilledPrefix.projectOut(isl_dim_set, order, 1);

          //FIXME: What if scatterRange is unbounded?
          auto min = scatterRange.dimMin(insertPos) - 1;
          auto max = scatterRange.dimMax(insertPos) + 1;

          prefix.addDims(isl_dim_set, ignored);
          for (auto i = insertPos+1; i < nScatterRangeDims; i+=1) {
            prefix.fix(isl_dim_set, i, 0);
          }


          auto beforeConst = scatterRangeSpace.emptySet();
          for (auto piece : min.getPieces()) {
            auto set = piece.first;
            auto aff = piece.second;
            auto eqbset = set.addContraint(scatterRangeSpace.createEqConstraint(aff.copy(), isl_dim_set, insertPos));
            beforeConst.union_inplace(eqbset);
          }

          auto beforeScatter = prefix.copy();


          //prefix.addContraint(scatterRangeSpace.createVarAff(isl_dim_set, insertPos) == min.toExpr());

          //intersect(scatterRangeSpace.universeBasicSet().addConstraint());
#endif

        }

        //TODO: For every may-write, copy the original value into that the target memory (unless they are the same), so we copy the correct values into the buffers
        // i.e. treat ever may write as, write original value if not written
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

    }; // class MollyScopContext


    // This is actually not a pass
    // It used internally for actions on functions and remembering function-specific data
    // Could be changed to a pass in later versions once we find out how to pass data between passes, without unrelated passes in between destroying them
    class MollyFunctionContext : public llvm::FunctionPass {
    private:
      MollyPassManager *pm;
      Function *func;

      void modifiedIR() {
        pm->modifiedIR();
      }

    public:
      static char ID;
      MollyFunctionContext(MollyPassManager *pm, Function *func): FunctionPass(ID), pm(pm), func(func) {
        assert(pm);
        assert(func);

        this->setResolver(new MollyFunctionResolver(pm , func));
      }

      bool runOnFunction(Function &F) LLVM_OVERRIDE {
        llvm_unreachable("This is not a pass");
      }

#pragma region Access CodeGen
    protected:
      Value *createAlloca(Type *ty, const Twine &name = Twine()) {
        auto entry = &func->getEntryBlock();
        auto insertPos = entry->getFirstNonPHIOrDbgOrLifetime();
        auto result = new AllocaInst(ty, name, insertPos);
        return result;
      }


      Value *createPtrTo(BuilderTy &builder,  Value *val, const Twine &name = Twine()) {
        auto valueSpace = createAlloca(val->getType(), name);
        builder.CreateStore(val, valueSpace);
        return valueSpace;
      }


      void emitRead(MollyFieldAccess &access){
        assert(access.isRead());
        auto accessor = access.getAccessor();
        auto bb = accessor->getParent();
        auto nDims = access.getNumDims();

        IRBuilder<> builder(bb);
        builder.SetInsertPoint(accessor->getNextNode()); // Behind the old load instr

        auto buf = builder.CreateAlloca(access.getElementType(), NULL, "getbuf");

        SmallVector<Value*,6> args;
        args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
        args.push_back(buf);
        for (auto d = nDims-nDims; d < nDims; d+=1) {
          auto coord = access.getCoordinate(d);
          args.push_back(coord);
        }

        auto call =builder.CreateCall(access.getFieldType()->getFuncGetBroadcast(), args);
        auto load = builder.CreateLoad(buf, "loadget");
        accessor->replaceAllUsesWith(load);
      }


      void emitWrite(MollyFieldAccess &access) {
        //TODO: Support structs
        assert(access.isWrite());
        auto accessor = cast<StoreInst>(access.getAccessor());
        auto bb = accessor->getParent();
        auto nDims = access.getNumDims();
        auto writtenValue = accessor->getOperand(0);

        IRBuilder<> builder(bb);
        builder.SetInsertPoint(accessor->getNextNode()); // Behind the old store instr


        SmallVector<Value*,6> args;
        args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
        auto stackPtr = createPtrTo(builder, writtenValue);
        args.push_back(stackPtr);
        for (auto d = nDims-nDims; d < nDims; d+=1) {
          auto coord = access.getCoordinate(d);
          args.push_back(coord);
        }

        builder.CreateCall(access.getFieldType()->getFuncSetBroadcast(), args);
      }


      void emitAccess(MollyFieldAccess &access) {
        auto accessor = access.getAccessor();
        auto bb = accessor->getParent();

        if (access.isRead()) {
          emitRead(access);
        } else if (access.isWrite()) {
          emitWrite(access);
        } else {
          llvm_unreachable("What is it?");
        }

        // Remove old access
        auto call = access.getFieldCall();
        accessor->eraseFromParent();
        if (call != accessor) { //FIXME: Comparison after delete accessor
          call->eraseFromParent();
        }
      }


      void emitLocalLengthCall(CallInst *callInst) {
        auto &context = callInst->getContext();
        auto bb = callInst->getParent();

        auto selfArg = callInst->getArgOperand(0);
        auto dimArg = callInst->getArgOperand(1);

        auto fty = pm->getFieldType(selfArg);
        auto locallengthFunc = fty->getLocalLengthFunc();
        assert(locallengthFunc);

        //TODO: Optimize for common case where dim arg is constant, or will LLVM do proper inlining itself?

        // BuilderTy builder(bb);
        //builder.SetInsertPoint(callInst);
        callInst->setCalledFunction(locallengthFunc);
        modifiedIR();
      }

      void emitIslocalCall(CallInst *callInstr) {
        auto &context = callInstr->getContext();
        auto selfArg = callInstr->getArgOperand(0);
        auto fty = pm->getFieldType(selfArg);

        // This is just a redirection to the implentation
        callInstr->setCalledFunction(fty->getIslocalFunc());

        modifiedIR();
      }

    public:
      void replaceRemainaingIntrinsics() {
        if (func->getName() == "test") {
          int a = 0;
          DEBUG(llvm::dbgs() << "### before FieldCodeGen ########\n");
          DEBUG(llvm::dbgs() << *func);
          DEBUG(llvm::dbgs() << "################################\n");
        }

        //MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
        //this->fields = &getAnalysis<FieldDetectionAnalysis>();
        this->func = func;

        modifiedIR();
        SmallVector<Instruction*, 16> instrs;
        collectInstructionList(func, instrs);

        // Replace intrinsics
        for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
          auto instr = *it;
          if (auto callInstr = dyn_cast<CallInst>(instr)) {
            auto calledFunc = callInstr->getCalledFunction();
            if (calledFunc) {
              switch (calledFunc->getIntrinsicID()) {
              case Intrinsic::molly_locallength:
                emitLocalLengthCall(callInstr);
                break;
              case Intrinsic::molly_islocal:
                emitIslocalCall(callInstr);
                break;
              default:
                break;
              }
            }
          }

          auto access = pm->getFieldAccess(instr);
          if (!access.isValid())
            continue;  // Not an access to a field

          emitAccess(access);
        }

        //bool funcEmitted = emitFieldFunctions();

        if (func->getName() == "main" || func->getName() == "__molly_orig_main") {
          DEBUG(llvm::dbgs() << "### after FieldCodeGen ########\n");
          DEBUG(llvm::dbgs() << *func);
          DEBUG(llvm::dbgs() << "###############################\n");
        }
      }

#pragma endregion


      Pass *findOrRunAnalysis(AnalysisID passID) {
        return pm->findOrRunAnalysis(passID, func, nullptr);
      }

      template<typename T>
      T *findOrRunAnalysis() {
        return pm->findOrRunAnalysis<T>(func, nullptr);
      }

      void removePass(Pass *pass) {
        pm->removePass(pass);
      }


      MollyFieldAccess getFieldAccess(Instruction* instr) {
        return pm->getFieldAccess(instr);
      }


#pragma region Isolate field accesses

      void isolateInBB(MollyFieldAccess &facc) {
        auto accessInstr = facc.getAccessor();
        auto callInstr = facc.getFieldCall();
        auto bb = accessInstr->getParent();

        //SmallVector<Instruction*, 8> isolate;

        //if (callInstr)
        //  isolate.push_back(accessInstr);



        // FIXME: This creates trivial BasicBlocks, only containing PHI instrunctions and/or the callInstr, and arithmetic instructions to compute the coordinate; Those should be moved into the isolated BB
        if (bb->getFirstNonPHI() != accessInstr) {
          bb = SplitBlock(bb, accessInstr, this);
        }
        auto isolatedBB = bb;

        auto followInstr = accessInstr->getNextNode();
        if (followInstr && !isa<BranchInst>(followInstr)) {
          bb = SplitBlock(bb, followInstr, this);
        }

        auto oldName = isolatedBB->getName();
        if (oldName.endswith(".split")) 
          oldName = oldName.drop_back(6);
        isolatedBB->setName(oldName + ".isolated");

        if (callInstr) {
          assert(callInstr->hasOneUse());
          callInstr->moveBefore(accessInstr);
        }
#if 0
        // Move operands into the isolated block
        // Required to detect the affine access relations
        SmallVector<Instruction*,8> worklist;
        worklist.push_back(accessInstr);

        while(!worklist.empty()) {
          auto instr = worklist.pop_back_val();    // instr is already in isolated block

          for (auto it = instr->op_begin(), end = instr->op_end(); it!=end; ++it) {
            assert( it->getUser() == instr);
            auto val = it->get();
            if (!isa<Instruction>(val))
              continue;  // No need to move a constant
            auto usedInstr = cast<Instruction>(val);

            if (isa<PHINode>(usedInstr)) 
              continue; // No need to move

            if (usedInstr->)

          }

        }
#endif
      }

      void isolateFieldAccesses() {
        // "Normalize" existing BasicBlocks
        auto ib = findOrRunAnalysis(&polly::IndependentBlocksID);
        removePass(ib);

        //for (auto &b : *func) {
        // auto bb = &b;

        bool changed =false;
        SmallVector<Instruction*, 16> instrs;
        collectInstructionList(func, instrs);
        for (auto instr : instrs){
          auto facc = getFieldAccess(instr);
          if (!facc.isValid())
            continue;
          // Create a distinct BB for this access
          // polly::IndependentBlocks will then it independent from the other BBs and
          // polly::ScopInfo create a ScopStmt for it
          isolateInBB(facc);
          changed = true;
        }
        //}

        if (changed) {
          // Also make new BBs independent
          ib = findOrRunAnalysis(&polly::IndependentBlocksID);
        }


        //auto sci = findOrRunAnalysis<TempScopInfo>();
        //auto sd = findOrRunAnalysis<ScopDetection>();
        //for (auto region : *sd) {
        //}

        // Check consistency
#ifndef NDEBUG
        for (auto &b : *func) {
          auto bb = &b;

          for (auto &i : *bb){
            auto instr = &i;
            auto facc = getFieldAccess(instr);
            if (facc.isValid()) {
              //assert(isFieldAccessBasicBlock(bb));
              break;
            }
          }
        }
#endif
      }

#pragma endregion

    }; // class MollyFunctionContext


    class MollyFieldContext {
    public:
    }; // class MollyFieldContext


    class MollyModuleContext {
    public:
    }; // class MollyModuleContext
#pragma endregion


  private:
    bool changedIR;

    void modifiedIR() {
      changedIR = true;
    }

  private:
    Module *module;

    llvm::DenseSet<AnalysisID> alwaysPreserve;
    llvm::DenseMap<llvm::AnalysisID, llvm::ModulePass*> currentModuleAnalyses; 
    llvm::DenseMap<std::pair<llvm::AnalysisID, llvm::Function*>, llvm::FunctionPass*> currentFunctionAnalyses; 
    llvm::DenseMap<std::pair<llvm::AnalysisID,llvm::Region*>, llvm::RegionPass*> currentRegionAnalyses; 

    void removePass(Pass *pass) {
      if (!pass)
        return;

      //FIXME: Also remove transitively dependent passes
      //TODO: Can this be done more efficiently?
      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto moduleAnalysisID = it->first;
        auto moduleAnalysisPass = it->second;

        if (moduleAnalysisPass==pass)
          currentModuleAnalyses.erase(it);
      }

      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto function = it->first.second;
        auto functionAnalysisID = it->first.first;
        auto functionAnalysisPass = it->second;

        if (functionAnalysisPass==pass)
          currentFunctionAnalyses.erase(it);
      }

      decltype(currentRegionAnalyses) rescuedRegionAnalyses;
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto region = it->first.second;
        auto regionAnalysisID = it->first.first;
        auto regionAnalysisPass = it->second;

        if (regionAnalysisPass==pass)
          currentRegionAnalyses.erase(it);
      }

      pass->releaseMemory();
      delete pass;
    }


    void removeUnpreservedAnalyses(const AnalysisUsage &AU, Function *func, Region *inRegion) {
      if (AU.getPreservesAll()) 
        return;

      if (!func && inRegion) {
        func = getParentFunction(inRegion);
      }
      //TODO: Also mark passes inherited from Resolver->getAnalysisIfAvailable as invalid so they are not reuse
      auto &preserved = AU.getPreservedSet();
      DenseSet<AnalysisID> preservedSet;
      for (auto it = preserved.begin(), end = preserved.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }
      for (auto it = alwaysPreserve.begin(), end = alwaysPreserve.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }

      DenseSet<Pass*> donotFree; 
      decltype(currentModuleAnalyses) rescuedModuleAnalyses;
      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto moduleAnalysisID = it->first;
        auto moduleAnalysisPass = it->second;

        if (preservedSet.find(moduleAnalysisID) != preservedSet.end()) {
          donotFree.insert(moduleAnalysisPass);
          rescuedModuleAnalyses[moduleAnalysisID] = moduleAnalysisPass;
        }
      }

      decltype(currentFunctionAnalyses) rescuedFunctionAnalyses;
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto function = it->first.second;
        auto functionAnalysisID = it->first.first;
        auto functionAnalysisPass = it->second;

        if (preservedSet.count(functionAnalysisID) || (func && func!=function)) {
          donotFree.insert(functionAnalysisPass);
          rescuedFunctionAnalyses[std::make_pair(functionAnalysisID, function)] = functionAnalysisPass;
        }
      }

      decltype(currentRegionAnalyses) rescuedRegionAnalyses;
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto region = it->first.second;
        auto regionAnalysisID = it->first.first;
        auto regionAnalysisPass = it->second;

        if (preservedSet.count(regionAnalysisID) || (inRegion && inRegion!=region)) {
          donotFree.insert(regionAnalysisPass);
          rescuedRegionAnalyses[std::make_pair(regionAnalysisID, region)] = regionAnalysisPass;
        }
      }


      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      } 
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      }
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      }

      currentModuleAnalyses = std::move(rescuedModuleAnalyses);
      currentFunctionAnalyses = std::move(rescuedFunctionAnalyses);
      currentRegionAnalyses = std::move(rescuedRegionAnalyses);
    }


    void removeAllAnalyses() {
      DenseSet<Pass*> doFree; // To avoid double-free

      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      } 
      currentModuleAnalyses.clear();
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentFunctionAnalyses.clear();
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentRegionAnalyses.clear();

      for (auto it = doFree.begin(), end = doFree.end(); it!=end; ++it) {
        auto pass = *it;
        pass->releaseMemory();
        delete pass;
      }
    }

  private:
    std::vector<Pass*> pollyPasses;

    void addPollyPass(Pass *pass) {
      pollyPasses.push_back(pass);
    }

  public:
    static char ID;
    MollyPassManager() : ModulePass(ID) {
      //alwaysPreserve.insert(&polly::PollyContextPassID);
      //alwaysPreserve.insert(&molly::MollyContextPassID);
      //alwaysPreserve.insert(&polly::ScopInfo::ID);
      //alwaysPreserve.insert(&polly::IndependentBlocksID);

      this->    callToMain = nullptr;
    }


    ~MollyPassManager() {
      removeAllAnalyses();
    }


    void runModulePass(llvm::ModulePass *pass, bool permanent = false) {
      auto passID = pass->getPassID();
      assert(!currentModuleAnalyses.count(passID));

      // Execute prerequisite passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, false);
      }

      // Execute pass
      pass->setResolver(new MollyModuleResolver(this));
      bool changed = pass->runOnModule(*module);
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, nullptr);

      // Register pass 
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentModuleAnalyses[intfPassID] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }
      currentModuleAnalyses[passID] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      }
    }


    ModulePass *findAnalysis(AnalysisID passID) {
      auto preexisting = Resolver->getAnalysisIfAvailable(passID, true);
      if (preexisting) {
        assert(preexisting->getPassKind() == PT_Module);
        return static_cast<ModulePass*>(preexisting);
      }

      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end()) {
        return it->second;
      }
      return nullptr;
    }
    ModulePass *findOrRunAnalysis(AnalysisID passID, bool permanent = false) {
      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end()) {
        return it->second;
      }

      auto parentPass = getResolver()->getAnalysisIfAvailable(passID, true);
      if (parentPass) {
        assert(parentPass->getPassKind() == PT_Module);
        return static_cast<ModulePass*>(parentPass);
      }

      auto pass = createPassFromId(passID);
      assert(pass->getPassKind() == PT_Module);
      auto modulePass = static_cast<ModulePass*>(pass);
      runModulePass(modulePass, permanent);
      return modulePass;
    }
    ModulePass *findOrRunAnalysis(const char &passID, bool permanent = false) {
      return findOrRunAnalysis(&passID, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis(bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis() {
      return findOrRunAnalysis<T>(true);
    }


    void runFunctionPass(llvm::FunctionPass *pass, Function *func, bool permanent=false) {
      auto passID = pass->getPassID(); 
      assert(!currentFunctionAnalyses.count(std::make_pair(passID, func)));

      // Execute prequisite passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, func, false);
      }

      // Execute pass
      pass->setResolver(new MollyFunctionResolver(this, func));
      bool changed = pass->runOnFunction(*func);
      if (changed)
        removeUnpreservedAnalyses(AU, func, nullptr);

      // Register pass
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentFunctionAnalyses[std::make_pair(intfPassID, func)] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }
      currentFunctionAnalyses[std::make_pair(passID, func)] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      }
    }

    Pass *findAnalysis(AnalysisID passID, Function *func) {
      auto it = currentFunctionAnalyses.find(make_pair(passID, func));
      if (it != currentFunctionAnalyses.end()) {
        return it->second;
      }

      return findAnalysis(passID);
    }
    Pass *findOrRunAnalysis(AnalysisID passID, Function *func, bool permanent = false) {
      auto existing = findAnalysis(passID, func);
      if (existing)
        return existing;

      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass), permanent);
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func, permanent);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }
      return pass;
    }
    Pass *findOrRunAnalysis(const char &passID,  Function *func,bool permanent ) {
      return findOrRunAnalysis(&passID,func, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis(Function *func,bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, func, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis(Function *func) {
      return (T*)findOrRunAnalysis(&T::ID, func, false)->getAdjustedAnalysisPointer(&T::ID); 
    }


    void runRegionPass(llvm::RegionPass *pass, Region *region, bool permanent = false) {
      auto passID = pass->getPassID();
      assert(!currentRegionAnalyses.count(std::make_pair(passID, region)));

      // Execute required passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, region, false);
      }

      // Execute pass
      pass->setResolver(new MollyRegionResolver(this, region));
      bool changed = pass->runOnRegion(region, *(static_cast<RGPassManager*>(nullptr)));
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, region); //TODO: Re-add required analyses?

      // Add pass info
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentRegionAnalyses[std::make_pair(intfPassID, region)] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }
      currentRegionAnalyses[std::make_pair(passID, region)] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      } 
    }


    Pass *findAnalysis(AnalysisID passID, Function *func, Region *region) {
      if (region && !func) {
        func = getParentFunction(region);
      }

      if (region) {
        auto it = currentRegionAnalyses.find(make_pair(passID, region));
        if (it != currentRegionAnalyses.end())
          return it->second;
      }

      if (func) {
        auto it = currentFunctionAnalyses.find(make_pair(passID, func));
        if (it != currentFunctionAnalyses.end())
          return it->second;
      }

      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end())
        return it->second;

      auto preexisting = Resolver->getAnalysisIfAvailable(passID, true);
      if (preexisting) {
        assert(preexisting->getPassKind() == PT_Module);
        return preexisting;
      }

      return nullptr;
    }

    template<typename T>
    T* findAnalysis(Function *func, Region *region) {
      auto passID = &T::ID;
      auto pass = findAnalysis(passID, func, region);
      return static_cast<T*>(pass->getAdjustedAnalysisPointer(passID));
    }


    Pass *findOrRunAnalysis(AnalysisID passID, Function *func, Region *region) {
      if (region && !func)
        func = getParentFunction(region);

      auto foundPass = findAnalysis(passID, func, region);
      if (foundPass)
        return foundPass;

      auto newPass = createPassFromId(passID);
      switch (newPass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(newPass));
        break;
      case PT_Function:
        assert(func);
        runFunctionPass(static_cast<FunctionPass*>(newPass), func);
        break;
      case PT_Region:
        assert(region);
        runRegionPass(static_cast<RegionPass*>(newPass), region);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }

      return newPass;
    }

    Pass *findAnalysis(AnalysisID passID, Region *region) {
      auto it = currentRegionAnalyses.find(make_pair(passID, region));
      if (it != currentRegionAnalyses.end()) {
        return it->second;
      }

      auto func = region->getEntry()->getParent();
      return findAnalysis(passID, func);
    }
    Pass *findOrRunAnalysis(AnalysisID passID, Region *region, bool permanent = false) {
      auto existing = findAnalysis(passID, region);
      if (existing)
        return existing;

      auto func = region->getEntry()->getParent();
      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass), permanent);
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func, permanent);
        break;
      case PT_Region:
        runRegionPass(static_cast<RegionPass*>(pass), region, permanent);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }
      return pass;
    }
    Pass *findOrRunAnalysis(const char &passID,  Region *region,bool permanent ) {
      return findOrRunAnalysis(&passID,region, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis( Region *region,bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, region, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis( Region *region) {
      return (T*)findOrRunAnalysis(&T::ID, region, false)->getAdjustedAnalysisPointer(&T::ID); 
    }


  protected:


  private:
    void runOnFunction(Function *func) {
    }

  private:
    //llvm::OwningPtr< isl::Ctx > islctx;
    isl::Ctx *islctx;

  public:
    isl::Ctx *getIslContext() { return islctx; }

  private:
    llvm::OwningPtr<ClusterConfig> clusterConf;

    void parseClusterShape() {
      string shape = MollyShape;
      SmallVector<StringRef, 4> shapeLengths;
      SmallVector<unsigned, 4> clusterLengths;
      StringRef(shape).split(shapeLengths, "x");

      for (auto it = shapeLengths.begin(), end = shapeLengths.end(); it != end; ++it) {
        auto slen = *it;
        unsigned len;
        auto retval = slen.getAsInteger(10, len);
        assert(retval==false);
        clusterLengths.push_back(len);
      }

      clusterConf->setClusterLengths(clusterLengths);
    }


#pragma region Field Detection
  private:
    llvm::DenseMap<llvm::StructType*, FieldType*> fieldTypes;

    void fieldDetection() {
      auto &glist = module->getGlobalList();
      auto &flist = module->getFunctionList();
      auto &alist = module->getAliasList();
      auto &mlist = module->getNamedMDList();
      auto &vlist = module->getValueSymbolTable();

      auto fieldsMD = module->getNamedMetadata("molly.fields"); 
      if (fieldsMD) {
        auto numFields = fieldsMD->getNumOperands();

        for (unsigned i = 0; i < numFields; i+=1) {
          auto fieldMD = fieldsMD->getOperand(i);
          FieldType *field = FieldType::createFromMetadata(islctx, module, fieldMD);
          fieldTypes[field->getType()] = field;
        }
      }
    }
#pragma endregion


#pragma region Field Distribution  
    void fieldDistribution_processFieldType(FieldType *fty) {
      auto lengths = fty->getLengths();
      auto cluster = clusterConf->getClusterLengths();
      auto nDims = lengths.size();

      SmallVector<int,4> locallengths;
      for (auto d = nDims-nDims; d<nDims; d+=1) {
        auto len = lengths[d];
        auto clusterLen = (d < cluster.size()) ? cluster[d] : 1;

        assert(len % clusterLen == 0);
        auto localLen = (len + clusterLen - 1) / clusterLen;
        locallengths.push_back(localLen);
      }
      fty->setDistributed();
      fty->setLocalLength(locallengths, clusterConf->getClusterTuple());
    }


    void fieldDistribution() {
      for (auto it : fieldTypes) {
        fieldDistribution_processFieldType(it.second);
      }
    }
#pragma endregion


#pragma region Field CodeGen
    Function *emitLocalLength(FieldType *fty) {
      auto &context = module->getContext();
      auto llvmTy = fty->getType();
      auto nDims = fty->getNumDimensions();
      auto intTy = Type::getInt32Ty(context);
      auto lengths = fty->getLengths();
      //auto molly = &getAnalysis<MollyContextPass>();

      SmallVector<Type*, 5> argTys;
      argTys.push_back(PointerType::getUnqual(llvmTy));
      argTys.append(nDims, intTy);
      auto localLengthFuncTy = FunctionType::get(intTy, argTys, false);

      auto localLengthFunc = Function::Create(localLengthFuncTy, GlobalValue::InternalLinkage, "molly_locallength", module);
      auto it = localLengthFunc->getArgumentList().begin();
      auto fieldArg = &*it;
      ++it;
      auto dimArg = &*it;
      ++it;
      assert(localLengthFunc->getArgumentList().end() == it);

      auto entryBB = BasicBlock::Create(context, "Entry", localLengthFunc); 
      auto defaultBB = BasicBlock::Create(context, "Default", localLengthFunc); 
      new UnreachableInst(context, defaultBB);

      IRBuilder<> builder(entryBB);
      builder.SetInsertPoint(entryBB);

      DEBUG(llvm::dbgs() << nDims << " Cases\n");
      auto sw = builder.CreateSwitch(dimArg, defaultBB, nDims);
      for (auto d = nDims-nDims; d<nDims; d+=1) {
        DEBUG(llvm::dbgs() << "Case " << d << "\n");
        auto caseBB = BasicBlock::Create(context, "Case_dim" + Twine(d), localLengthFunc);
        ReturnInst::Create(context, ConstantInt::get(intTy, lengths[d] / clusterConf ->getClusterLength(d)/*FIXME: wrong if nondivisible*/), caseBB);

        sw->addCase(ConstantInt::get(intTy, d), caseBB);
      }

      modifiedIR();
      fty->setLocalLengthFunc(localLengthFunc);
      return localLengthFunc;
    }

    void generateAccessFuncs() {
      for (auto it : fieldTypes) {
        emitLocalLength(it.second);
      }
    }

  private:
    CallInst *callToMain;

    void wrapMain() {
      auto &context = module->getContext();

      // Search main function
      auto origMainFunc = module->getFunction("main");
      if (!origMainFunc) {
        //FIXME: This means that either we are compiling modules independently (instead of whole program as intended), or this program as already been optimized 
        // The driver should resolve this
        return;
        llvm_unreachable("No main function found");
      }

      // Rename old main function
      const char *replMainName = "__molly_orig_main";
      auto replMainFunc = module->getFunction(replMainName);
      if (replMainFunc) {
        llvm_unreachable("main already replaced?");
      }

      origMainFunc->setName(replMainName);

      // Find the wrapper function from MollyRT
      auto rtMain = module->getOrInsertFunction("__molly_main", Type::getInt32Ty(context), Type::getInt32Ty(context)/*argc*/, PointerType::get(Type::getInt8PtrTy(context), 0)/*argv*/, NULL);
      assert(rtMain);

      // Create new main function
      Type *parmTys[] = {Type::getInt32Ty(context)/*argc*/, PointerType::get(Type::getInt8PtrTy(context), 0)/*argv*/ };
      auto mainFuncTy = FunctionType::get(Type::getInt32Ty(context), parmTys, false);
      auto wrapFunc = Function::Create(mainFuncTy, GlobalValue::ExternalLinkage, "main", module);

      auto entry = BasicBlock::Create(context, "entry", wrapFunc);
      IRBuilder<> builder(entry);

      // Create a call to the wrapper main function
      SmallVector<Value *, 2> args;
      collect(args, wrapFunc->getArgumentList());
      //args.append(wrapFunc->arg_begin(), wrapFunc->arg_end());
      auto ret = builder.CreateCall(rtMain, args, "call_to_rtMain");
      this->callToMain = ret;
      DEBUG(llvm::dbgs() << ">>>Wrapped main\n");
      modifiedIR();
      builder.CreateRet(ret);
    }
#pragma endregion


    MollyFunctionContext *getFuncContext(Function *func) {
      auto &ctx = funcs[func];
      if (!ctx)
        ctx = new MollyFunctionContext(this, func);
      return ctx;
    }

    MollyScopContext *getScopContext(Scop *scop) {
      auto &ctx = scops[scop];
      if (!ctx)
        ctx = new MollyScopContext(this, scop);
      return ctx;
    }

  private:
    DenseMap<ScopStmt*, MollyScopStmtContext*> stmts;
  public:
    MollyScopStmtContext *getScopStmtContext(ScopStmt *stmt) {
      auto &ctx = stmts[stmt];
      if (!ctx)
        ctx = new MollyScopStmtContext(this, stmt);
      return ctx;
    }

#pragma region Scop Detection
  private:
    DenseMap<Function*, MollyFunctionContext*> funcs;
    DenseMap<Scop*, MollyScopContext*> scops;

    ScopInfo *si;

    void runScopDetection() {
      for (auto &it : *module) {
        auto func = &it;
        if (func->isDeclaration())
          continue;

        auto funcCtx = new MollyFunctionContext(this, func);
        funcs[func] = funcCtx;

        auto regionInfo = findOrRunAnalysis<RegionInfo>(func);
        SmallVector<Region*,12> regions;
        collectAllRegions(regionInfo->getTopLevelRegion(), regions);
        for (auto region : regions) {
          auto scopInfo = new ScopInfo(islctx->keep()); // We need ScopInfo to convice to take out isl_ctx
          runRegionPass(scopInfo, region, true);
          //findOrRunAnalysis<ScopInfo>(region);
          auto scop = scopInfo->getScop();
          if (scop) {
            auto scopCtx = new MollyScopContext(this, scop);
            scops[scop] = scopCtx;
          }
        }
      }
    }
#pragma endregion


    FieldType *lookupFieldType(llvm::StructType *ty) {
      auto result = fieldTypes.find(ty);
      if (result == fieldTypes.end())
        return nullptr;
      return result->second;
    }


  public:
    FieldType *getFieldType(llvm::StructType *ty) {
      auto result = lookupFieldType(ty);
      assert(result);
      return result;
    }

    FieldType *getFieldType(llvm::Value *val) {
      auto ty = val->getType();
      if (auto pty = dyn_cast<PointerType>(ty)) {
        ty = pty->getPointerElementType();
      }
      return getFieldType(llvm::cast<llvm::StructType>(ty));
    }


  public:
    FieldVariable *getFieldVariable(GlobalVariable *gvar) {
      auto fieldTy = getFieldType(cast<StructType>(gvar->getType()->getPointerElementType()));
      auto res = FieldVariable::create(gvar, fieldTy);
      return res;
    }


    void augmentFieldVariable(MollyFieldAccess &facc) {
      if (!facc.isValid())
        return;

      auto base = facc.getBaseField();
      auto globalbase = dyn_cast<GlobalVariable>(base);
      assert(globalbase && "Currently only global fields supported");
      auto gvar = getFieldVariable(globalbase);
      assert(gvar);
      facc.setFieldVariable(gvar);
    }


    MollyFieldAccess getFieldAccess(llvm::Instruction *instr) {
      assert(instr);
      auto result = MollyFieldAccess::fromAccessInstruction(instr);
      augmentFieldVariable(result); 
      return result;
    }


    MollyFieldAccess getFieldAccess(ScopStmt *stmt) {
      assert(stmt);
      auto result = MollyFieldAccess::fromScopStmt (stmt);
      augmentFieldVariable(result);
      return result;
    }


    MollyFieldAccess getFieldAccess(polly::MemoryAccess *memacc) {
      assert(memacc);
      auto result = MollyFieldAccess::fromMemoryAccess(memacc);
      augmentFieldVariable(result);
      return result;
    }


#pragma region Communication buffers
  private:
    std::vector<CommunicationBuffer *> combufs;

  public :
    CommunicationBuffer *newCommunicationBuffer(FieldType *fty, isl::Map &&relation) {
      auto comvarSend = new GlobalVariable(*module, runtimeMetadata.tyCombufSend, false, GlobalValue::PrivateLinkage, nullptr, "combufsend");
      auto comvarRecv = new GlobalVariable(*module, runtimeMetadata.tyCombufRecv, false, GlobalValue::PrivateLinkage, nullptr, "combufrecv");
      auto result =  CommunicationBuffer::create(comvarSend, comvarRecv, fty, relation.copy());
      combufs.push_back(result);
      return result;
    }

  private:
    llvm::Function* emitCombufInit(CommunicationBuffer *combuf) {
      auto &llvmContext = module->getContext();
      auto func = createFunction(nullptr, module, GlobalValue::PrivateLinkage, "initCombuf");
      auto bb = BasicBlock::Create(llvmContext, "entry", func);
      IRBuilder<> builder (bb);
      auto rtn = builder.CreateRetVoid();

      auto editor = ScopEditor::newScop(rtn, getFuncContext(func));
      auto scop = editor.getScop();
      auto scopCtx = getScopContext(scop);

      auto rel = combuf->getRelation(); /* { (src[coord] -> dst[coord]) -> field[indexset] } */
      auto mapping = combuf->getMapping();
      auto eltCount = combuf->getEltCount();

      auto scatterTupleId = isl::Space::enwrap( scop->getScatteringSpace() ).getSetTupleId();
      auto singletonSet = islctx->createSetSpace(0,1).setSetTupleId(scatterTupleId).createUniverseBasicSet().fix(isl_dim_set, 0, 0);

      // Send buffers
      auto sendWhere = rel.getDomain().unwrap(); /* { src[coord] -> dst[coord] } */ // We are on the src node, send to dst node
      auto sendDomain = sendWhere.getRange(); /* { dst[coord] }*/
      auto sendScattering = islctx->createAlltoallMap(sendDomain, singletonSet) ; /* { dst[coord] -> scattering[] } */
      auto stmtEditor = editor.createStmt(sendDomain.copy(), sendScattering.copy(), sendWhere.copy(), "sendcombuf_create");
      auto sendStmt = stmtEditor.getStmt();
      auto sendBB = stmtEditor.getBasicBlock();
      IRBuilder<> sendBuilder(stmtEditor. getTerminator());
      auto domainVars = stmtEditor.getDomainValues();
      auto sendDstRank = clusterConf->codegenComputeRank(sendBuilder,domainVars);
      std::map<isl_id *, llvm::Value *> sendParams;
      editor.getParamsMap( sendParams, sendStmt);
      auto sendSize =  buildIslAff(sendBuilder, eltCount, sendParams);
      Value* sendArgs[] = { sendDstRank, sendSize };
      sendBuilder.CreateCall(runtimeMetadata.funcCreateSendCombuf, sendArgs);

      // Receive buffers
      auto recvWhere = sendWhere.reverse(); /* { src[coord] -> dst[coord] } */ // We are on dst node, recv from src
      auto recvDomain = recvWhere.getDomain(); /* { src[coord] } */
      auto recvScatter = islctx->createAlltoallMap(recvDomain, singletonSet) ; /* { src[coord] -> scattering[] } */
      auto recvEditor = editor.createStmt(sendDomain.copy(), sendScattering.copy(), sendWhere.copy(), "sendcombuf_create");
      auto recvStmt = recvEditor.getStmt();
      auto recvBB = recvEditor.getBasicBlock();
      IRBuilder<> recvBuilder(recvEditor.getTerminator());
      auto recvVars = recvEditor.getDomainValues();
      auto recvSrcRank = clusterConf->codegenComputeRank(sendBuilder,recvVars);
      std::map<isl_id *, llvm::Value *> recvParams;
      editor.getParamsMap(recvParams, recvStmt);
      auto recvSize = buildIslAff(recvBuilder, eltCount, recvParams);
      Value* recvArgs[] = { recvSrcRank, recvSize };
      sendBuilder.CreateCall(runtimeMetadata.funcCreateRecvCombuf, sendArgs);

      return func;
    }


    Function *emitAllCombufInit() {
      auto &llvmContext = module->getContext();
      auto allInitFunc = createFunction(nullptr, module, GlobalValue::PrivateLinkage, "initCombufs");
      auto bb = BasicBlock::Create(llvmContext, "entry", allInitFunc);
      IRBuilder<> builder (bb);

      for (auto combuf : combufs) {
        auto func = emitCombufInit(combuf);
        builder.CreateCall(func);
      }

      builder.CreateRetVoid();
      return allInitFunc;
    }


    void addCallToCombufInit() {
      auto initFunc = emitAllCombufInit();
      IRBuilder<> builder(callToMain);
      builder.CreateCall(initFunc);
    } 
#pragma endregion


#pragma region Runtime
  private:
    clang::CodeGen::MollyRuntimeMetadata runtimeMetadata;

  public:
    llvm::Type *getCombufSendType() { return runtimeMetadata.tyCombufSend; } 
    llvm::Type *getCombufRecvType() { return runtimeMetadata.tyCombufRecv; } 
    llvm::Function *getCombufSendFunc() { return runtimeMetadata.funcCombufSend; }
#pragma endregion


#pragma region llvm::ModulePass
  public:
    void releaseMemory() LLVM_OVERRIDE {
      removeAllAnalyses();
    }
    const char *getPassName() const LLVM_OVERRIDE { 
      return "MollyPassManager"; 
    }
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
      AU.addRequired<DataLayout>();
      // Requires nothing, preserves nothing
    }

    bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
      this->changedIR = false;
      this->module = &M;

      // PollyContext
      // Need one isl_ctx to combine isl objects from different SCoPs 
      this->islctx = isl::Ctx::create();

      // Cluster configuration
      this->clusterConf.reset(new ClusterConfig(islctx));
      parseClusterShape();

      // Get runtime info
      runtimeMetadata.readMetadata(module);

      // Search fields
      fieldDetection();

      // Decide where fields should have their home location
      fieldDistribution();

      // Generate access functions
      generateAccessFuncs();

      wrapMain();

      for (auto &f : *module) {
        auto func = &f;
        if (func->isDeclaration())
          continue;

        auto funcCtx = getFuncContext(func);
        funcCtx->isolateFieldAccesses();
      }

      // Find all scops
      runScopDetection();

      for (auto &it : scops) {
        auto scop = it.first;
        auto scopCtx = it.second;

        // Decide on which node(s) a ScopStmt should execute 
        scopCtx->computeScopDistibution();

        // Insert communication between ScopStmt
        scopCtx->genCommunication();
      }

      // Create some SCoPs that init the combufs
      addCallToCombufInit();

      for (auto &it : scops) {
        auto scop = it.first;
        auto scopCtx = it.second;
        for (auto stmt : *scop) {
          auto stmtCtx = getScopStmtContext(stmt);
          stmtCtx->applyWhere();
        }

        // Let polly optimize and and codegen the scops
        //scopCtx->pollyOptimize();
        scopCtx->pollyCodegen();
      }


      // Replace all remaining accesses by some generated intrinsic
      for (auto &func : *module) {
        auto funcCtx = getFuncContext(&func);
        funcCtx->replaceRemainaingIntrinsics();
      }

      //FIXME: Find all the leaks
      //this->islctx.reset();
      this->clusterConf.reset();
      return changedIR;
    }
#pragma endregion

  }; // class MollyPassManager
} // namespace molly


char molly::MollyPassManager::ID = 0;
extern char &molly::MollyPassManagerID = molly::MollyPassManager::ID;
ModulePass *molly::createMollyPassManager() {
  return new MollyPassManager();
}


char molly::MollyPassManager::MollyFunctionContext::ID;
