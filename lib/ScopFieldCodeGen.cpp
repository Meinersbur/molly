#define DEBUG_TYPE "molly"
#include "ScopFieldCodeGen.h"

#include <polly/ScopPass.h>
#include "MollyContextPass.h"
#include "FieldDetection.h"
#include <polly/Dependences.h>
//#include <llvm/Analysis/ScalarEvolution.h>
#include <polly/ScopInfo.h>
#include "ScopUtils.h"
#include "ScopEditor.h"
#include <llvm/Support/Debug.h>
#include <polly/PollyContextPass.h>
#include <islpp/Map.h>
#include <islpp/UnionMap.h>
#include "MollyFieldAccess.h"
#include <islpp/Point.h>
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include "IslExprBuilder.h"

using namespace molly;
using namespace llvm;
using namespace polly;
using isl::enwrap;
using std::move;

namespace molly {
  class ScopFieldCodeGen : public polly::ScopPass {
  private:
    bool changed;
    FieldDetectionAnalysis *Fields;
    //Dependences *Deps;
    MollyContextPass *Ctx;
    isl::Ctx *islctx;
    //ScalarEvolution *SE;

    //isl::UnionMap flowDeps;

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
      //AU.addRequired<polly::Dependences>();

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


    void processScop(Scop *scop) {
      auto func = scop->getRegion().getEntry()->getParent();
      auto &llvmContext = func->getContext();
      auto module = func->getParent();

      // Make some space between the stmts in which we can insert our communication
      spreadScatterings(scop, 2);

      // Collect information
      // Like polly::Dependences::collectInfo, but finer granularity (MemoryAccess instead ScopStmt)
      DenseMap<const isl_id*, polly::MemoryAccess*> tupleToAccess;

      auto space = enwrap(scop->getParamSpace());
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

          auto facc = MollyFieldAccess::fromMemoryAccess(memacc);
          facc.augmentFieldDetection(Fields);
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
        auto faccWrite = MollyFieldAccess::fromMemoryAccess(memaccWrite);
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
        auto faccWrite = MollyFieldAccess::fromMemoryAccess(memaccWrite);
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
      //Deps = &getAnalysis<Dependences>();

      //flowDeps = getFlowDependences(Deps);

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
