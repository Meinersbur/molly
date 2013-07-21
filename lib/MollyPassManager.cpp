#define DEBUG_TYPE "molly"
#include "MollyPassManager.h"

#include <llvm/Pass.h>
#include <polly/PollyContextPass.h>
//#include <llvm/IR/Module.h>
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

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace molly {
  class MollyPassManager;
  class MollyScopContext;
  class MollyFunctionContext;




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

    public:
      MollyScopContext(MollyPassManager *pm, Scop *scop) : pm(pm), scop(scop), changedScop(false) {
        func = getParentFunction(scop);
        islctx = pm->getIslContext();
      }

#pragma region Scop Distribution
      void processFieldAccess(MollyFieldAccess &acc, isl::Map &executeWhereWrite, isl::Map &executeWhereRead) {
        if (acc.isPrologue() || acc.isEpilogue()) {
          return; // These are not computed anywhere
        }

        auto fieldVar = acc.getFieldVariable();
        auto fieldTy = acc.getFieldType();
        auto stmt = acc.getPollyScopStmt();
        auto scop = stmt->getParent();

        auto rel = acc.getAffineAccess(SE); /* iteration coord -> field coord */
        auto home = fieldTy->getHomeAff(); /* field coord -> cluster coord */
        auto it2rank = rel.toMap().applyRange(home.toMap());

        if (acc.isRead()) {
          executeWhereRead = unite(executeWhereRead.substractDomain(it2rank.getDomain()), it2rank);
        }

        if (acc.isWrite()) {
          executeWhereWrite = unite(executeWhereWrite.substractDomain(it2rank.getDomain()), it2rank);
        }
      }

    protected:
      void processScopStmt(ScopStmt *stmt) {
        if (stmt->isPrologue() || stmt->isEpilogue())
          return;

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
        result = unite(result.substractDomain(executeWhereRead.getDomain()), executeWhereRead);
        result = unite(result.substractDomain(executeWhereWrite.getDomain()), executeWhereWrite);
        stmt->setWhereMap(result.take());

        modifiedScop();
      }

    public:
      void computeScopDistibution() {
        SE = pm->findOrRunAnalysis<ScalarEvolution>(&scop->getRegion());

        for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
          auto stmt = *itStmt;
          processScopStmt(stmt);
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
        DEBUG(llvm::dbgs() << "run ScopFieldCodeGen on " << scop->getNameStr() << " in func " << func->getName() << "\n");
        if (func->getName() == "test") {
          int a = 0;
        }


        auto func = scop->getRegion().getEntry()->getParent();
        auto &llvmContext = func->getContext();
        auto module = func->getParent();

        // Make some space between the stmts in which we can insert our communication
        spreadScatterings(scop, 2);

        // Collect information
        // Like polly::Dependences::collectInfo, but finer granularity (MemoryAccess instead ScopStmt)
        DenseMap<const isl_id*, polly::MemoryAccess*> tupleToAccess;

        auto space = isl::enwrap(scop->getParamSpace());
        auto readAccesses = space.createEmptyUnionMap(); /* { stmt[iteration] -> access[indexset] } */
        auto writeAccesses = readAccesses.copy(); /* { stmt[iteration] -> access[indexset] } */
        auto schedule = readAccesses.copy(); /* { stmt[iteration] -> scattering[] } */

        for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
          auto stmt = *itStmt;
          auto domain = getIterationDomain(stmt);
          auto scattering = getScattering(stmt);
          scattering.intersectDomain_inplace(domain);

          for (auto itAcc = stmt->memacc_begin(), endAcc = stmt->memacc_end(); itAcc!=endAcc; ++itAcc) { 
            auto memacc = *itAcc;

            auto facc = pm->getFieldAccess(memacc);
            //facc.augmentFieldDetection(Fields);
            if (!facc.isValid())
              continue;
            auto fvar = facc.getFieldVariable();
            auto fty = facc.getFieldType();

            auto accId = facc.getAccessTupleId();
            assert(!tupleToAccess.count(accId.keep()) && "accId must be unique");
            tupleToAccess[accId.keep()] = memacc;

            auto accessRel = facc.getAccessRelation();
            accessRel.intersectDomain_inplace(domain);
            accessRel.setInTupleId_inplace(accId);

            //FIXME: access withing the same stmt are without order, what happens if they depend on each other?
            // - Such accesses that do not appear outside must be filtered out
            // - Add another coordinate to give them an order
            if (facc.isRead()) {
              readAccesses.addMap_inplace(accessRel);
            }
            if (facc.isWrite()) {
              writeAccesses.addMap_inplace(accessRel);
            }

            schedule.addMap_inplace(facc.getAccessScattering());
          }
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

        auto max = scatterRange.dimMax(0) + 1;
        max.setInTupleId_inplace(scatterId);
        isl::Set epilogueDomain = epilogueDomainSpace.universeSet();
        auto epilogieMapToZero = epilogueScatterSpace.createUniverseBasicMap();
        for (auto d = 1; d < nScatterRangeDims; d+=1) {
          epilogieMapToZero.addConstraint_inplace(epilogueScatterSpace.createVarExpr(isl_dim_out, d) == 0);
        }

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

        for (auto dep : inputFlow.getMaps()) {
          // Value source is outside this scop 
          // Value must be read from home location
        }


        for (auto dep : outputFlow.getMaps()) {
          assert(dep.getOutTupleId() == epilogueId);
          // This means the data is visible outside the scop and must be written to its home location
          // There is no explicit read access

          auto itDomainWrite = dep.getDomain(); /* write_acc[domain] */
          auto memaccWrite = tupleToAccess[itDomainWrite.getTupleId().keep()];
          assert(memaccWrite);
          auto faccWrite = pm->getFieldAccess(memaccWrite); 
          auto stmtWrite = faccWrite.getPollyScopStmt();
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
          auto affectedNodes = indexset.apply(homeAff); /* { [nodecoord] } */
          auto homeRel = homeAff.toMap().reverse().intersectRange(indexset) ; /* { [nodecoord] -> field[indexset] } */
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
          tys.push_back(fty->getType()); // target field
          args.push_back(faccWrite.getFieldPtr());
          tys.push_back(fty->getEltType()); // source buffer
          args.push_back(stackMem);
          auto nDims = alreadyHome.getDimCount();
          for (auto i = 0; i < nDims; i+=1) {
            tys.push_back(Type::getInt32Ty(llvmContext));
            args.push_back( faccWrite.getCoordinate(i));
          }
          auto intrSetLocal = Intrinsic::getDeclaration(module, Intrinsic::molly_set_local, tys);
          builder.CreateCall(intrSetLocal, args, "set_local");

          auto leastmostOne = scatterSpace.createZeroMultiAff().setAff(nScatterDims-1, scatterSpace.createConstantAff(1));
          auto afterWrite = scatterWrite.intersectDomain(writeAlreadyHomeDomain).sum(leastmostOne);
          auto writebackLocalStmt = createScopStmt(scop, bb, stmtWrite->getRegion(), "writeback_local", stmtWrite->getLoopNests(), writeAlreadyHomeDomain.move(), afterWrite.move());


          // CodeGen to transfer data to their home location

          // 1. Create a buffer
          // 2. Write the data into that buffer
          // 3. Send buffer
          // 4. Receive buffer
          // 5. Writeback buffer data to home location



#pragma endregion
        }


        for (auto dep : stmtFlow.getMaps()) { /* dep: { write_acc[domain] -> read_acc[domain] } */
          auto itDomainWrite = dep.getDomain(); /* write_acc[domain] */
          auto memaccWrite = tupleToAccess[itDomainWrite.getTupleId().keep()];
          assert(memaccWrite);
          auto faccWrite = pm->getFieldAccess(memaccWrite);
          auto stmtWrite = faccWrite.getPollyScopStmt();
          auto scatterWrite = getScattering(stmtWrite); /* { write_stmt[domain] -> scattering[scatter] } */
          auto relWrite = faccWrite.getAccessRelation(); /* { write_stmt[domain] -> field[indexset] } */
          auto domainWrite = itDomainWrite.setTupleId(isl::Id::enwrap(stmtWrite->getTupleId())); /* write_stmt[domain] */
          auto fvar = faccWrite.getFieldVariable();
          auto fty = fvar->getFieldType();

          auto itDomainRead = dep.getRange();
          auto itReadTuple = itDomainRead.getTupleId();

          auto memaccRead = tupleToAccess[itDomainRead.getTupleId().keep()];
          auto faccRead = MollyFieldAccess::fromMemoryAccess(memaccRead);
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

        for (auto itStmt = scop->begin(), endStmt = scop->end(); itStmt!=endStmt; ++itStmt) {
          auto stmt = *itStmt;
          processScopStmt(stmt);
        }
      }
#pragma endregion
    }; // class MollyScopContext


    class MollyFunctionContext {
    private:
      MollyPassManager *pm;
      Function *func;


      void modifiedIR() {
        pm->modifiedIR();
      }

    public:
      MollyFunctionContext(MollyPassManager *pm, Function *func):pm(pm), func(func) {
        assert(pm);
        assert(func);
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
        delete pass;
      } 
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        delete pass;
      }
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
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

      switch (Optimizer) {
#ifdef SCOPLIB_FOUND
      case OPTIMIZER_POCC:
        addPollyPass( polly::createPoccPass());
        break;
#endif
#ifdef PLUTO_FOUND
      case OPTIMIZER_PLUTO:
        addPollyPass( polly::createPlutoOptimizerPass());
        break;
#endif
      case OPTIMIZER_ISL:
        addPollyPass( polly::createIslScheduleOptimizerPass());
        break;
      case OPTIMIZER_NONE:
        break; 
      }

      switch (CodeGenerator) {
#ifdef CLOOG_FOUND
      case CODEGEN_CLOOG:
        addPollyPass(polly::createCodeGenerationPass());
        if (PollyVectorizerChoice == VECTORIZER_BB) {
          VectorizeConfig C;
          C.FastDep = true;
          addPollyPass(createBBVectorizePass(C));
        }
        break;
#endif
      case CODEGEN_ISL:
        addPollyPass(polly::createIslCodeGenerationPass());
        break;
      case CODEGEN_NONE:
        break;
      }

    }

    ~MollyPassManager() {
      removeAllAnalyses();
    }



    void runModulePass(llvm::ModulePass *pass, bool permanent) {
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
      pass->setResolver(new MollyModuleResolver(this));

      // Execute pass
      bool changed = pass->runOnModule(*module);
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, nullptr);
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


    void runFunctionPass(llvm::FunctionPass *pass, Function *func, bool permanent) {
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

      // Execute pass
      pass->setResolver(new MollyFunctionResolver(this, func));
      bool changed = pass->runOnFunction(*func);
      if (changed)
        removeUnpreservedAnalyses(AU, func, nullptr);
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


    llvm::Pass *runRegionPass(llvm::RegionPass *pass, Region *region, bool permanent) {
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
      pass->setResolver(new MollyRegionResolver(this, region));

      // Execute pass
      bool changed = pass->runOnRegion(region, *(static_cast<RGPassManager*>(nullptr)));
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, region); //TODO: Re-add required analyses?
      return pass;
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
    llvm::OwningPtr< isl::Ctx > islctx;

  public:
    isl::Ctx *getIslContext() { return islctx.get(); }

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
          FieldType *field = FieldType::createFromMetadata(islctx.get(), module, fieldMD);
          fieldTypes[field->getType()] = field;
        }
      }
    }
#pragma endregion


#pragma region Field Distribution  
    void fieldDistribution_processFieldType(FieldType *fty) {
      // auto &contextPass = getAnalysis<MollyContextPass>();
      auto lengths = fty->getLengths();
      auto cluster = clusterConf->getClusterLengths();
      auto nDims = lengths.size();

      SmallVector<int,4> locallengths;
      for (auto d = nDims-nDims; d<nDims; d+=1) {
        auto len = lengths[d];
        auto clusterLen = (d < cluster.size()) ? cluster[d] : 1;

        assert(len % clusterLen == 0);
        auto localLen = len / clusterLen;
        locallengths.push_back(localLen);
      }
      fty->setDistributed();
      fty->setLocalLength(locallengths);
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
      DEBUG(llvm::dbgs() << ">>>Wrapped main\n");
      modifiedIR();
      builder.CreateRet(ret);
    }
#pragma endregion


    MollyFunctionContext *getFuncContext(Function *func) {
      auto &ctx = funcs[func];
      if (!ctx)
        ctx =  new MollyFunctionContext(this, func);
      return ctx;
    }

    MollyScopContext *getScopContext(Scop *scop) {
      auto &ctx = scops[scop];
      if (!ctx)
        ctx =  new MollyScopContext(this, scop);
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


    MollyFieldAccess getFieldAccess(const llvm::Instruction *instr) {
      return MollyFieldAccess::fromAccessInstruction(const_cast<Instruction*>(instr));
    }


    MollyFieldAccess getFieldAccess(polly::MemoryAccess *memacc) {
      assert(memacc);
      auto instr = const_cast<Instruction*>(memacc->getAccessInstruction());
      assert(instr);

      // TODO: Get rid of this tmp
      auto tmp = MollyFieldAccess::fromAccessInstruction(instr);

      auto base = tmp.getBaseField();
      auto globalbase = dyn_cast<GlobalVariable>(base);
      assert(globalbase && "Currently only global fields supported");
      auto gvar = getFieldVariable(globalbase);
      auto result =  MollyFieldAccess::create(instr, memacc,gvar);
      return result;
    }


#pragma region Scop Distribution
    void scopDistribution() {
      for (auto it : scops) {
        auto scopCtx = it.second;
        scopCtx->computeScopDistibution();
      }
    }
#pragma endregion 


#pragma region Field CodeGen
    void scopFieldCodeGen() {

    }
#pragma endregion


#pragma region Polly Passes
    void runPollyPasses() {
      for (auto scop : scops) {
      }
    }
#pragma endregion



#pragma region Access CodeGen
    void accessCodeGen() {
      for (auto &func : *module) {
        auto funcCtx  = getFuncContext  (&func);
        funcCtx->replaceRemainaingIntrinsics();
      }
    }
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
      this->islctx.reset(isl::Ctx::create());

      // Cluster configuration
      this->clusterConf.reset( new ClusterConfig(islctx.get()));
      parseClusterShape();

      // Search fields
      fieldDetection();

      // Decide where fields shouold have their home location
      fieldDistribution();

      // Generate access functions
      generateAccessFuncs();

      wrapMain();

      // Find all scops
      runScopDetection();

      // Decide on which node(s) a ScopStmt should execute 
      scopDistribution();

      // Insert communication between ScopStmt
      scopFieldCodeGen();

      // Let polly optimize and and codegen the scops
      runPollyPasses();

      // Replace all remaining accesses by some generated intrinsic
      accessCodeGen();

#if 0
      findOrRunPemanentAnalysis<polly::PollyContextPass>();
      findOrRunPemanentAnalysis<molly::MollyContextPass>();
      findOrRunAnalysis(molly::FieldDetectionAnalysisPassID, true);
      findOrRunAnalysis(molly::FieldDistributionPassID, false);
      findOrRunAnalysis(molly::ModuleFieldGenPassID, false);

      for (auto it = M.begin(), end = M.end(); it!=end; ++it) {
        auto func = &*it;
        runOnFunction(func);
      }
#endif

      this->islctx.reset();
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
