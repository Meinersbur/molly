#include "MollyScopStmtProcessor.h"
#include "MollyPassManager.h"
#include "polly/ScopInfo.h"
#include "MollyUtils.h"
#include "ScopUtils.h"
#include "ClusterConfig.h"
#include "MollyScopProcessor.h"
#include "islpp/Set.h"
#include "islpp/Map.h"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include "Codegen.h"
#include "MollyRegionProcessor.h"
#include <llvm/Analysis/ScalarEvolution.h>
#include "MollyFieldAccess.h"
#include "polly/FieldAccess.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include "ScopUtils.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace {

  class MollyScopStmtProcessorImpl : public MollyScopStmtProcessor {
  private:
    MollyPassManager *pm;
    ScopStmt *stmt;
    MemoryAccess *fmemacc;
    FieldAccess facc;
    FieldVariable *fvar;

  public:
    MollyScopStmtProcessorImpl(MollyPassManager *pm, ScopStmt *stmt) : MollyScopStmtProcessor(this), pm(pm), stmt(stmt), fmemacc(nullptr), fvar(nullptr) {
      assert(pm);
      assert(stmt);

      for (auto it = stmt->memacc_begin(), end = stmt->memacc_end(); it!=end; ++it) {
        auto memacc = *it;
        auto accInstr = const_cast<Instruction*>(memacc->getAccessInstruction());
        facc.loadFromInstruction(accInstr);
        if (facc.isValid()) {
          fmemacc = memacc;
          break;
        }
      }

      if (fmemacc) {
        auto fptr = facc.getFieldPtr();
        fvar = pm->getFieldVariable(fptr);
        assert(fvar);
        //TODO: Validate that this ScopStmt contains nothing but stuff to access a field
      }
    }

  protected:
    Scop *getParent() const { return stmt->getParent(); }
    MollyScopProcessor *getParentProcessor() const { return pm->getScopContext(getParent()); }
    MollyFunctionProcessor *getFunctionProcessor() const { return pm->getFuncContext(getFunctionOf(stmt)); }
    ClusterConfig *getClusterConfig() const { return pm->getClusterConfig(); }
    isl::BasicSet getClusterShape() const { return getClusterConfig()->getClusterShape(); }

    void identifyDomainDims() {
      auto islctx = getIslContext();
      auto scopCtx = getScopProcessor();

      auto domain = getDomain();

      assert(domain.hasTupleId());
      auto nDims = domain.getDimCount();
      bool changed = false;
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        if (!domain.hasDimId(i)) {
          auto loop = stmt->getLoopForDimension(i);
          auto id = scopCtx->getIdForLoop(loop);
          domain.setDimId_inplace(i, id);
          changed = true;
        }
      }
      if (changed) {
        stmt->setDomain(domain.takeCopy());

        auto scatter = getScattering();
        auto scatterSpace = isl::Space::createMapFromDomainAndRange(domain.getSpace(), scatter.getRangeSpace());
        scatter.cast_inplace(scatterSpace);
        stmt->setScattering(scatter.take());

        auto where = getWhere();
        if (where.isValid()) {
          auto whereSpace = isl::Space::createMapFromDomainAndRange(domain.getSpace(), where.getRangeSpace());
          where.cast_inplace(whereSpace);
          stmt->setWhereMap(where.take());
        }
      }
    }


    isl::Set getDomain() const LLVM_OVERRIDE {  
      return enwrap(stmt->getDomain());
    }

    isl::Space getDomainSpace() const LLVM_OVERRIDE {
    return getDomain().getSpace();
    }

    isl::Set getDomainWithNamedDims() const LLVM_OVERRIDE {
      auto scopCtx = getScopProcessor();
      auto domain = getDomain();

      auto nDims = domain.getDimCount();
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        //if (!domain.hasDimId(i)) { /* Domains might be constructed from other means, like iterating over node coords; in this case, its id will be the cluster's rankdim id, which we do not want */
          auto loop = stmt->getLoopForDimension(i);
          auto id = scopCtx->getIdForLoop(loop);
          domain.setDimId_inplace(i, id);
        //}
      }
      return domain;
    }
    isl::Map getScattering() const LLVM_OVERRIDE {
      return enwrap(stmt->getScattering());
    }
    isl::PwMultiAff getScatteringAff() const LLVM_OVERRIDE {
      return getScattering().toPwMultiAff();
    }
    bool hasWhere() const {
      return enwrap(stmt->getWhereMap()).isValid();
    }
    isl::Map getWhere() const LLVM_OVERRIDE {
      return enwrap(stmt->getWhereMap());
    } 
    isl::Map getInstances() const LLVM_OVERRIDE {
      return enwrap(stmt->getWhereMap()).intersectDomain(getDomain());
    }
    BasicBlock *getBasicBlock() LLVM_OVERRIDE {
      return stmt->getBasicBlock();
    }
    polly::ScopStmt *getStmt() LLVM_OVERRIDE {
      return stmt;
    }

    llvm::Pass *asPass() LLVM_OVERRIDE {
      return getParentProcessor()->asPass();
    }


    MollyCodeGenerator makeCodegen() LLVM_OVERRIDE {
      auto term = getBasicBlock()->getTerminator();
      return MollyCodeGenerator(this, term);
    }


    MollyCodeGenerator makeCodegen(llvm::Instruction *insertBefore) LLVM_OVERRIDE {
      assert(insertBefore);
      return MollyCodeGenerator(this, insertBefore);
    }


    StmtEditor getEditor() LLVM_OVERRIDE {
      assert(stmt);
      return StmtEditor::create(stmt, asPass());
    }


    void applyWhere() LLVM_OVERRIDE {
      auto scop = stmt->getParent();
      auto func = getFunctionOf(stmt);
      //auto funcCtx = pm->getFuncContext(func);
      auto scopCtx = pm->getScopContext(scop);
      // auto se = pm->findOrRunAnalysis<ScalarEvolution>(func);
      auto clusterConf = pm->getClusterConfig();

      auto domain = getIterationDomain(stmt);
      auto where = getWhereMap(stmt);
      where.intersectDomain_inplace(domain);

      auto nCoords = clusterConf->getClusterDims();
      auto coordSpace = clusterConf->getClusterSpace();
      auto coordValues = scopCtx->getClusterCoordinates();
      assert(coordValues.size() == nCoords);

      auto coordMatches = coordSpace.createUniverseBasicSet();
      for (auto i = nCoords-nCoords; i < nCoords; i+=1) {
        // auto coordValue = coordValues[i];
        auto scev = coordValues[i];
        auto id = enwrap(scop->getIdForParam(scev));
        coordMatches.alignParams_inplace(pm->getIslContext()->createParamsSpace(1).setParamDimId(0,id)); // Add the param if not exists yet
        auto paramDim = coordMatches.findDim(id);
        assert(paramDim.isValid());
        auto coordDim = coordSpace.getSetDim(i);
        assert(coordDim.isValid());

        coordMatches.equate_inplace(paramDim, coordDim);
      }

      auto newDomain = where.intersectRange(coordMatches).getDomain();

      stmt->setDomain(newDomain.take());
      stmt->setWhereMap(nullptr);
    }


  private:
    llvm::DenseMap<const SCEV *, Value *> scevToValue;
    std::map<isl_id *, Value *> idToValue;

  protected:
    llvm::Value *getValueOf(const SCEV *scev) {
      auto scop = getParent();

      auto &result = scevToValue[scev];
      if (result)
        return result;

      result = getParentProcessor()->codegenScev(scev, getBasicBlock()->getFirstInsertionPt());

      auto id = scop->getIdForParam(scev);
      assert(idToValue.count(id));
      idToValue[id] = result;

      return result;
    }
    llvm::Value *getValueOf(Value *val) {
      return val;
    }


  protected:
    template<typename Analysis> 
    Analysis *findOrRunAnalysis() {
      return pm->findOrRunAnalysis<Analysis>(nullptr, getRegionOf(stmt));
    }


    std::map<isl_id *, Value *> &getIdToValueMap() LLVM_OVERRIDE {
      return idToValue;

      auto scop = getParent();
      auto SE = findOrRunAnalysis<ScalarEvolution>();

      auto &validParams = stmt->getValidParams();
      auto &paramsList = scop->getParams();
      auto paramsSpace = enwrap(scop->getParamSpace());
      auto domain = getDomain();
      auto nDims = domain.getDimCount();
      auto expectedIds = nDims + validParams.size() + paramsSpace.getParamDimCount(); 
      assert(idToValue.size() <= expectedIds);
      if (idToValue.size() == expectedIds)
        return idToValue;

      // 1. Valid parameters of this Scop
      // TODO: obsolete
      for (auto &param : validParams) {
        getValueOf(param);
      }

      // 2. Params
      auto nParamDims = paramsSpace.getParamDimCount();
      for (auto i = nParamDims-nParamDims; i<nParamDims; i+=1) {
        auto id = paramsSpace.getParamDimId(i);
      }

      // 3. The loop induction variables
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto iv = getDomainValue(i);
        auto id = getDomainId(i);
        assert(id.isValid());
        idToValue[id.keep()] = iv;
      }

      return idToValue;
    }


    llvm::Value *getDomainValue(unsigned i) LLVM_OVERRIDE {
      return const_cast<PHINode *>(stmt->getInductionVariableForDimension(i));
    }


    const llvm::SCEV *getDomainSCEV(unsigned i) LLVM_OVERRIDE {
      auto value = getDomainValue(i);
      return getParentProcessor()->scevForValue(value);
    }


    isl::Id getDomainId(unsigned i) LLVM_OVERRIDE {
      auto loop = stmt->getLoopForDimension(i);
      return getParentProcessor()->getIdForLoop(loop);
    }


    isl::Aff getDomainAff(unsigned i) LLVM_OVERRIDE {
      // There are two possibilities on what to return:
      // 1. An aff that maps from the domain space. The result aff is equal to the i's isl_dim_in
      // 2. An aff without domain space. The result is the isl::Id that represent the SCEV

      auto domainSpace = enwrap(stmt->getDomainSpace());
      return domainSpace.createAffOnVar(i);

      auto paramsSpace = getParentProcessor()->getParamsSpace();
      auto id = getDomainId(i);
      auto pos = paramsSpace.findDimById(isl_dim_param, id);
      return paramsSpace.createVarAff(isl_dim_param, pos);
    }


    std::vector<llvm::Value *> getDomainValues() LLVM_OVERRIDE {
      auto domain = getDomain();
      auto nDims = domain.getDimCount();

      std::vector<llvm::Value *> result;
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto iv = getDomainValue(i);
        result.push_back(iv);
      }

      return result;
    }


    isl::MultiAff getDomainMultiAff() LLVM_OVERRIDE {
      auto domain = getDomain();
      auto nDims = domain.getDimCount();

      auto result = domain.getSpace().mapsTo(domain.getSpace()).createZeroMultiAff();
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto aff = getDomainAff(i);
        result.setAff_inplace(i, aff);
      }

      return result;
    }


    isl::Space getDomainSpace() {
      return this->getDomain().getSpace();
    }


    isl::MultiAff getClusterMultiAff() LLVM_OVERRIDE {
      auto clusterShape = getClusterShape();
      auto nClusterDims = clusterShape.getDimCount();
      auto domainSpace = getDomainSpace();

      auto scopCtx = getScopProcessor();
      auto aff = scopCtx->getCurrentNodeCoordinate();

      auto resultSpace = isl::Space::createMapFromDomainAndRange(domainSpace, aff.getRangeSpace());
      auto translateSpace = isl::Space::createMapFromDomainAndRange(domainSpace, aff.getDomainSpace());

      auto result = aff.pullback(translateSpace.createZeroMultiAff());
      return result;
    }


    void dump() const LLVM_OVERRIDE;


    // isValid
    bool isFieldAccess() const LLVM_OVERRIDE {
      assert(!!fvar == !!fmemacc);
      return fvar != nullptr;
    }


    FieldVariable *getFieldVariable() const LLVM_OVERRIDE {
      assert(fvar);
      return fvar;
    }


    FieldType *getFieldType() const LLVM_OVERRIDE { 
      return fvar->getFieldType();
    }


    // getPollyMemoryAccess
    polly::MemoryAccess *getFieldMemoryAccess() const {
      assert(isFieldAccess());
      return fmemacc;
    }



  public:
#if 0
    isl::MultiPwAff MollyFieldAccess::getAffineAccess() {
      auto se = findOrRunAnalysis<ScalarEvolution>();
      SmallVector<llvm::Value*,4> coords;
      facc. getCoordinates(coords);
      auto scopStmt = getStmt();

      auto iterDomain = molly::getIterationDomain(getStmt());
      auto indexDomain = getFieldType()->getGlobalIndexset();
      auto result = isl::Space::createMapFromDomainAndRange(iterDomain.getSpace(), indexDomain.getSpace()).createZeroMultiPwAff();
      auto i = 0;
      for (auto it = coords.begin(), end = coords.end(); it!=end; ++it) {
        auto coordVal = *it;
        auto coordSCEV = se->getSCEV(coordVal);

        auto aff = convertScEvToAffine(scopStmt, coordSCEV);
        result.setPwAff_inplace(i, aff.move());
        i+=1;
      }


      return result;
    }
#endif

    isl::Map/*iteration coord -> field coord*/ getAccessRelation() const LLVM_OVERRIDE {
      assert(isFieldAccess());
      assert(fmemacc);
      auto fvar = getFieldVariable();
      auto rel = isl::enwrap(fmemacc->getAccessRelation());
      rel.setOutTupleId_inplace(fvar->getTupleId());//TODO: This should have been done when Molly detects SCoPs
      return rel;
    }


    isl::Set getAccessedRegion() {
      auto result = getAccessRelation().getRange();
      assert(result.getSpace() == getIndexsetSpace());
      return result;
    }

    isl::Space getIndexsetSpace() {
      auto memacc = getFieldMemoryAccess();
      assert(memacc);
      auto map = isl::enwrap(memacc->getAccessRelation());
      return map.getRangeSpace();
    }

    isl::Id getAccessTupleId() const {
      auto memacc = getFieldMemoryAccess();
      assert(memacc);
      return enwrap(memacc->getTupleId());
    }

    isl::Map getAccessScattering() const {
      return getScattering();
    }

    isl::PwMultiAff getHomeAff() const {
      auto tyHomeAff = getFieldType() ->getHomeAff();
      tyHomeAff.setTupleId_inplace(isl_dim_in, getAccessRelation().getOutTupleId() );
      return tyHomeAff;
    }


    llvm::StoreInst *getLoadUse() const {
      auto ld = facc.getLoadInst();

      // An access within a Scop should have been isolated into its own ScopStmt by MollyPassManager::isloateFieldAccesses
      // and  IndependentBlocks made independent by writing the result to a local variable that is picked up by those ScopStmt that need it
      assert(isFieldAccess());
      assert(ld->getNumUses() == 1);

      auto use = *ld->use_begin();
      return cast<StoreInst>(use);
    }


    void validate() const LLVM_OVERRIDE {
#ifndef NDEBUG
      assert(stmt);

      auto scop = getParent();
      auto clusterConf = pm->getClusterConfig();

      auto domain = getDomain();
      auto domainSpace = domain.getSpace();
      assert(domain.hasTupleId());

      auto scatter = getScattering();
      assert(scatter.getSpace().matchesMapSpace(domainSpace, getScatterTuple(scop)));
      assert(scatter.getDomain().isSupersetOf(domain));

      if (hasWhere()) {
        auto where = getWhere();
        assert(where.isValid());
        auto clusterSpace = clusterConf->getClusterSpace();
        assert(where.getSpace().matchesMapSpace(domainSpace, clusterSpace));
        assert(where.getDomain().isSupersetOf(domain));
      }

      for (auto it = stmt->memacc_begin(), end = stmt->memacc_end(); it!=end; ++it) {
        auto memacc = *it;
        auto accrel = enwrap(memacc->getAccessRelation());
        assert(accrel.getDomainSpace().matchesSpace(domainSpace));
      }

      if (isFieldAccess()) {
        auto fvar = getFieldVariable();
        assert(fvar);
        auto fty = fvar->getFieldType();
        assert(fty);
        auto indexsetSpace = fty->getLogicalIndexsetSpace();
        auto accessSpace = fvar->getAccessSpace();

        auto accrel = getAccessRelation();
        assert(accrel.getSpace().matchesMapSpace(domainSpace, accessSpace));

        //TODO: Check that nothing notable is going on except the access to a field
      }
#endif
    }


    llvm::LLVMContext &getLLVMContext() const LLVM_OVERRIDE {
      return pm->getLLVMContext();
    }


    isl::Ctx *getIslContext() const LLVM_OVERRIDE {
      return enwrap(stmt->getIslCtx());
    }


    bool isReadAccess() const LLVM_OVERRIDE {
      assert(isFieldAccess());
      return fmemacc->isRead();
    }


    bool isWriteAccess() const LLVM_OVERRIDE {
      assert(isFieldAccess());
      return fmemacc->isWrite();
    }


    molly::MollyPassManager *getPassManager() LLVM_OVERRIDE {
      return pm;
    }


    molly::MollyScopProcessor *getScopProcessor() const LLVM_OVERRIDE {
      return getParentProcessor();
    }

   const llvm::Region *getRegion() LLVM_OVERRIDE {
      return  stmt->getRegion();
    }


    llvm::Instruction *getAccessor() LLVM_OVERRIDE {
      assert(isFieldAccess());
      return facc.getAccessor();
    }


    llvm::LoadInst *getLoadAccessor() LLVM_OVERRIDE {
      assert(isReadAccess());
      return cast<LoadInst>(getAccessor());
    }


    llvm::StoreInst *getStoreAccessor() LLVM_OVERRIDE {
      assert(isWriteAccess());
      return cast<StoreInst>(getAccessor());
    }

    llvm::Value *getAccessedCoordinate(unsigned i) LLVM_OVERRIDE {
      assert(isFieldAccess());
      return facc.getCoordinate(i);
    }

    /// Compared to getAccessRelation(), this returns just the one location that is accessed
    /// Hence, it won't work if there is a MAY access
    isl::MultiPwAff getAccessed() LLVM_OVERRIDE {
      assert(isFieldAccess());

      auto space = getAccessRelation().getSpace();
      auto result = space.createZeroMultiPwAff();
      auto nDims = result.getOutDimCount();
      for (auto i = nDims-nDims; i < nDims ; i+=1) {
        auto scev = getParentProcessor()->scevForValue(getAccessedCoordinate(i));
        auto aff = enwrap(polly::affinatePwAff(stmt, scev)).setInTupleId(space.getInTupleId());
        result.setPwAff_inplace(i, aff);
      }
      return result;
    }


    void addMemoryAccess(polly::MemoryAccess::AccessType type, const llvm::Value *base, isl::Map accessRelation, llvm::Instruction *accInstr) LLVM_OVERRIDE {
      stmt->addAccess(type, base, accessRelation.take(), accInstr);
    }

  }; // class MollyScopStmtProcessorImpl

  void MollyScopStmtProcessorImpl::dump() const
  {
    dbgs() << "ScopStmtProcessorImpl:\n";
    if (stmt)
      stmt->dump();
  }

} // namespace 


MollyScopStmtProcessor *MollyScopStmtProcessor::create(MollyPassManager *pm, polly::ScopStmt *stmt) {
  return new MollyScopStmtProcessorImpl(pm, stmt);
}
