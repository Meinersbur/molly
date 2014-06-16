#include "MollyScopStmtProcessor.h"

#include "MollyPassManager.h"
#include "MollyUtils.h"
#include "ScopUtils.h"
#include "ClusterConfig.h"
#include "MollyScopProcessor.h"
#include "Codegen.h"
#include "MollyRegionProcessor.h"
#include "MollyFieldAccess.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include "ScopUtils.h"
#include "FieldLayout.h"

#include "islpp/Set.h"
#include "islpp/Map.h"

#include "polly/ScopInfo.h"
#include "polly/FieldAccess.h"
#include "polly/Accesses.h"

#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>



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
    //FieldAccess facc;
    FieldVariable *fvar;

  public:
    MollyScopStmtProcessorImpl(MollyPassManager *pm, ScopStmt *stmt) : MollyScopStmtProcessor(this), pm(pm), stmt(stmt), fmemacc(nullptr), fvar(nullptr) {
      assert(pm);
      assert(stmt);

      llvm::Value *fptr;
      for (auto it = stmt->begin(), end = stmt->end(); it != end; ++it) {
        auto memacc = *it;
        auto acc = Access::fromMemoryAccess(memacc);

        if (!acc.isFieldAccess())
          continue;

        assert(!fmemacc && "Must be at most one field access per ScopStmt");
        fmemacc = memacc;
        fptr = acc.getFieldPtr();
      }

      if (fmemacc) {
        //auto fptr = facc.getFieldPtr();
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
      for (auto i = nDims - nDims; i < nDims; i += 1) {
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

    isl::Set getDomain() const override {
      return enwrap(stmt->getDomain());
    }

    isl::Space getDomainSpace() const override {
      return getDomain().getSpace();
    }

    isl::Set getDomainWithNamedDims() const override {
      auto scopCtx = getScopProcessor();
      auto domain = getDomain();

      auto nDims = domain.getDimCount();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        //if (!domain.hasDimId(i)) { /* Domains might be constructed from other means, like iterating over node coords; in this case, its id will be the cluster's rankdim id, which we do not want */
        auto loop = stmt->getLoopForDimension(i);
        auto id = scopCtx->getIdForLoop(loop);
        domain.setDimId_inplace(i, id);
        //}
      }
      return domain;
    }
    isl::Map getScattering() const override {
      return enwrap(stmt->getScattering());
    }
    isl::PwMultiAff getScatteringAff() const override {
      return getScattering().toPwMultiAff();
    }
    bool hasWhere() const {
      return enwrap(stmt->getWhereMap()).isValid();
    }
    isl::Map getWhere() const override {
      return enwrap(stmt->getWhereMap());
    }

    void setWhere(isl::Map instances) {
      assert(matchesSpace(getDomainSpace(), instances.getDomainSpace()));
      stmt->setWhereMap(instances.take());
    }
    void addWhere(isl::Map newInstances) {
      assert(hasWhere());
      auto oldInstances = getWhere();
      if (newInstances <= oldInstances) 
        return; // no effect
      setWhere(unite(oldInstances, newInstances).removeRedundancies_consume().coalesce_consume());
    }


    isl::Map getInstances() const override {
      return enwrap(stmt->getWhereMap()).intersectDomain(getDomain());
    }

    BasicBlock *getBasicBlock() override {
      return stmt->getBasicBlock();
    }

    polly::ScopStmt *getStmt() override {
      return stmt;
    }

    llvm::Pass *asPass() override {
      return getParentProcessor()->asPass();
    }


    MollyCodeGenerator makeCodegen() override {
      auto term = getBasicBlock()->getTerminator();
      return MollyCodeGenerator(this, term);
    }


    MollyCodeGenerator makeCodegen(llvm::Instruction *insertBefore) override {
      assert(insertBefore);
      return MollyCodeGenerator(this, insertBefore);
    }


    StmtEditor getEditor() override {
      assert(stmt);
      return StmtEditor::create(stmt, asPass());
    }


    void applyWhere() override {
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
      for (auto i = nCoords - nCoords; i < nCoords; i += 1) {
        // auto coordValue = coordValues[i];
        auto scev = coordValues[i];
        auto id = enwrap(scop->getIdForParam(scev));
        coordMatches.alignParams_inplace(pm->getIslContext()->createParamsSpace(1).setParamDimId(0, id)); // Add the param if not exists yet
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


    std::map<isl_id *, Value *> &getIdToValueMap() override {
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
      for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
        auto id = paramsSpace.getParamDimId(i);
      }

      // 3. The loop induction variables
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto iv = getDomainValue(i);
        auto id = getDomainId(i);
        assert(id.isValid());
        idToValue[id.keep()] = iv;
      }

      return idToValue;
    }


    llvm::Value *getDomainValue(unsigned i) override {
      return const_cast<PHINode *>(stmt->getInductionVariableForDimension(i));
    }


    const llvm::SCEV *getDomainSCEV(unsigned i) override {
      auto value = getDomainValue(i);
      return getParentProcessor()->scevForValue(value);
    }


    isl::Id getDomainId(unsigned i) override {
      auto loop = stmt->getLoopForDimension(i);
      return getParentProcessor()->getIdForLoop(loop);
    }


    isl::Aff getDomainAff(unsigned i) override {
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


    std::vector<llvm::Value *> getDomainValues() override {
      auto domain = getDomain();
      auto nDims = domain.getDimCount();

      std::vector<llvm::Value *> result;
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto iv = getDomainValue(i);
        result.push_back(iv);
      }

      return result;
    }


    isl::MultiAff getDomainMultiAff() override {
      auto domain = getDomain();
      auto nDims = domain.getDimCount();

      auto result = domain.getSpace().mapsTo(domain.getSpace()).createZeroMultiAff();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto aff = getDomainAff(i);
        result.setAff_inplace(i, aff);
      }

      return result;
    }


    isl::Space getDomainSpace() {
      return this->getDomain().getSpace();
    }


    isl::MultiAff getClusterMultiAff() override {
      //auto clusterShape = getClusterShape();
      //auto nClusterDims = clusterShape.getDimCount();
      auto domainSpace = getDomainSpace();

      auto scopCtx = getScopProcessor();
      auto aff = scopCtx->getCurrentNodeCoordinate();

      //auto resultSpace = isl::Space::createMapFromDomainAndRange(domainSpace, aff.getRangeSpace());
      auto translateSpace = isl::Space::createMapFromDomainAndRange(domainSpace, aff.getDomainSpace());

      auto result = aff.pullback(translateSpace.createZeroMultiAff());
      return result;
    }


    isl::PwMultiAff getClusterPwMultiAff() override {
      //auto clusterShape = getClusterShape();
      auto domain = getDomain();
      auto result = getClusterMultiAff().toPwMultiAff();
      result.intersectDomain_inplace(domain);
      return result;
    }



    void dump() const override;


    // isValid
    bool isFieldAccess() const override {
      assert(!!fvar == !!fmemacc);
      return fvar != nullptr;
    }


    FieldVariable *getFieldVariable() const override {
      assert(fvar);
      return fvar;
    }


    FieldType *getFieldType() const override {
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

        auto aff = affinatePwAff(scopStmt, coordSCEV);
        result.setPwAff_inplace(i, aff.move());
        i+=1;
      }


      return result;
    }
#endif

    isl::Map/*iteration coord -> field coord*/ getAccessRelation() const override {
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

    isl::Space getIndexsetSpace() const {
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

    //isl::PwMultiAff getHomeAff() const {
    //  auto layout = getFieldType()->getDefaultLayout();
    //  auto tyHomeAff = layout->getHomeAff();
    //  tyHomeAff.setTupleId_inplace(isl_dim_in, getAccessRelation().getOutTupleId() );
    //  return tyHomeAff;
    //}

#if 0
    llvm::StoreInst *getLoadUse() const {
      auto ld = facc.getLoadInst();

      // An access within a Scop should have been isolated into its own ScopStmt by MollyPassManager::isloateFieldAccesses
      // and  IndependentBlocks made independent by writing the result to a local variable that is picked up by those ScopStmt that need it
      assert(isFieldAccess());
      assert(ld->getNumUses() == 1);

      auto use = *ld->use_begin();
      return cast<StoreInst>(use);
    }
#endif

    void validate() const override {
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
        //assert(where.domain().isSupersetOf(domain)); // TODO: Where.domain()  may include additional restrictions of some isl_dim_param which renders them not-equal
      }

      for (auto it = stmt->begin(), end = stmt->end(); it != end; ++it) {
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


    llvm::LLVMContext &getLLVMContext() const override {
      return pm->getLLVMContext();
    }


    isl::Ctx *getIslContext() const override {
      return enwrap(stmt->getIslCtx());
    }


    bool isReadAccess() const override {
      assert(isFieldAccess());
      return fmemacc->isRead();
    }


    bool isWriteAccess() const override {
      assert(isFieldAccess());
      return fmemacc->isWrite();
    }


    molly::MollyPassManager *getPassManager() override {
      return pm;
    }


    molly::MollyScopProcessor *getScopProcessor() const override {
      return getParentProcessor();
    }


    const llvm::Region *getRegion() override {
      return  stmt->getRegion();
    }


    llvm::Instruction *getAccessor() override {
      assert(isFieldAccess());
      auto acc = Access::fromMemoryAccess(fmemacc);
      assert(acc.isFieldAccess());
      return acc.getInstruction();
    }


    llvm::Instruction *getLoadAccessor() override {
      assert(isReadAccess());
      return getAccessor();
    }


    llvm::Instruction *getStoreAccessor() override {
      assert(isWriteAccess());
      return getAccessor();
    }


    llvm::Value *getAccessPtr() override {
      assert(fmemacc);
      auto acc = Access::fromMemoryAccess(fmemacc);
      return acc.getTypedPtr();
    }


    llvm::Value *getAccessedCoordinate(unsigned i) override {
      assert(isFieldAccess());
      auto acc = Access::fromMemoryAccess(fmemacc);
      assert(acc.isFieldAccess());
      return acc.getCoordinate(i);
    }


    Access getAccess() override {
      assert(isFieldAccess());
      return Access::fromMemoryAccess(fmemacc);
    }


    /// If this is a field access stmt, returns the alloca the load result is stored/the value to store is taken from
    llvm::AllocaInst *getAccessStackStoragePtr() override {
      assert(isFieldAccess());
      auto acc = Access::fromMemoryAccess(fmemacc);
      if (isReadAccess()) {
        auto ptr = acc.getReadResultPtr();
        if (!ptr) {
          auto val = acc.getReadResultRegister();
          ptr = getStackStoragePtr(val);
        }
        return cast<AllocaInst>(ptr);
      } else {
        assert(isWriteAccess());
        auto ptr = acc.getWrittenValuePtr();
        if (!ptr) {
          auto val = acc.getWrittenValueRegister();
          ptr = getStackStoragePtr(val);
        }
        return cast_or_null<AllocaInst>(ptr);
      }
    }

    
    AnnotatedPtr getAccessStackStorageAnnPtr() override {
      assert(isFieldAccess());
      auto acc = Access::fromMemoryAccess(fmemacc);
      if (isReadAccess()) {
        auto ptr = acc.getReadResultPtr();
        if (!ptr) {
          auto val = acc.getReadResultRegister();
          ptr = getStackStoragePtr(val);
        }
        return AnnotatedPtr::createScalarPtr(ptr, getDomainSpace());
      } else {
        assert(isWriteAccess());
        auto ptr = acc.getWrittenValuePtr();
        if (!ptr) {
          auto val = acc.getWrittenValueRegister();
          ptr = getStackStoragePtr(val);
          if (!ptr) {
            // This is a problem; the value has been moved into the BB, so no stack storage necessary
           return AnnotatedPtr::createRegister(val);
          }
        }
        return AnnotatedPtr::createScalarPtr(ptr, getDomainSpace());
      }
    }


    bool isDependent(llvm::Value *val) {
      // Something that does not relate to scopes (Constants, GlobalVariables)
      auto instr = dyn_cast<Instruction>(val);
      if (!instr)
        return false;

      // Something within this statement is also non-dependent of the SCoP around
      auto bb = getBasicBlock();
      if (instr->getParent() == bb)
        return false;

      // Something outside the SCoP, but inside the function is also not dependent
      auto scopCtx = getScopProcessor();
      auto scopRegion = scopCtx->getRegion();
      if (!scopRegion->contains(instr))
        return false;

      // As special exception, something that dominates all ScopStmts also is not concerned by shuffling the ScopStmts' order
      // This is meant for AllocaInsts we put there
      //if (instr->getParent() == scopRegion->getEntry())
      //  return false;

      return true;
    }


    /// Find the memory on the stack this value is stored between ScopStmts
    /// For non-canSynthesize it is guaranteed to exist by IndependentBlocks pass
    /// Return nullptr if no memory has been allocated as temporary storage
    llvm::AllocaInst *getStackStoragePtr(llvm::Value *val) override {
      val = val->stripPointerCasts();
      if (auto alloca = dyn_cast<AllocaInst>(val)) {
        // It is a alloca itself, using aggregate semantics 
        return alloca;
      }

      // Is this value loading the scalar; if yes, it's scalar location is obviously where it has been loaded from
      if (auto load = dyn_cast<LoadInst>(val)) {
        auto ptr = load->getPointerOperand();
        if (auto alloca = dyn_cast<AllocaInst>(ptr))
          if (alloca->getAllocatedType() == val->getType())
            return alloca;
      }

      // Look where IndependentBlocks stored the val
      for (auto itUse = val->user_begin(), endUse = val->user_end(); itUse != endUse; ++itUse) {
        auto useInstr = *itUse;
        if (!isa<StoreInst>(useInstr))
          continue;
        if (itUse.getOperandNo() == StoreInst::getPointerOperandIndex())
          continue; // Must be the value operand, not the target ptr

        auto store = cast<StoreInst>(useInstr);
        if (store->getParent() != getBasicBlock())
          continue;
        auto ptr = store->getPointerOperand();
        if (auto alloca = dyn_cast<AllocaInst>(ptr)) {
          if (alloca->getAllocatedType() == val->getType())
            return alloca;
        }
      }
      return nullptr;
    }


    /// Compared to getAccessRelation(), this returns just the one location that is accessed
    /// Hence, it won't work if there is a MAY access
    isl::MultiPwAff getAccessed() override {
      assert(isFieldAccess());

      auto space = getAccessRelation().getSpace();
      auto result = space.createZeroMultiPwAff();
      auto nDims = result.getOutDimCount();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto scev = getParentProcessor()->scevForValue(getAccessedCoordinate(i));
        auto aff = enwrap(polly::affinatePwAff(stmt, scev)).setInTupleId(space.getInTupleId());
        result.setPwAff_inplace(i, aff);
      }
      return result;
    }


    void addMemoryAccess(polly::MemoryAccess::AccessType type, const llvm::Value *base, isl::Map accessRelation, llvm::Instruction *accInstr) override {
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
