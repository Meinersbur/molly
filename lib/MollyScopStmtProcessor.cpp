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
    MollyScopStmtProcessorImpl(MollyPassManager *pm, ScopStmt *stmt) : pm(pm), stmt(stmt), fmemacc(nullptr), fvar(nullptr) {
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
    Scop *getParent() { return stmt->getParent(); }
    MollyScopProcessor *getParentProcessor() { return pm->getScopContext(getParent()); }

    isl::Set getDomain() const {  
      return enwrap(stmt->getDomain());
    }
    isl::Map getScattering() const {
      auto result = enwrap(stmt->getScattering());
      assert(isSubset(getDomain(), result.getDomain()));
      return result;
    }
    isl::Map getWhere() {
      auto result = enwrap(stmt->getWhereMap());
      assert(result.getDomain().isSupersetOf(getDomain()));
      return result;
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

#if 0
  private:
    std::map<isl_id *, llvm::Value *> valueMap;
  public:
    std::map<isl_id *, llvm::Value *> *getValueMap() {
      if (!valueMap.empty())
        return &valueMap;

      // 1. The parent SCoP's context/parameters
      auto parentMap = getParentProcessor()->getValueMap();
      for (auto i : *parentMap) {
        valueMap[i.first] = i.second; 
      }

      // 2. The statements domain induction variables
      auto domain = getDomain();
      auto nDims = domain.getDimCount();
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto id = domain.getDimId(i);
        valueMap[id.keep()] = const_cast<PHINode*>(stmt->getInductionVariableForDimension(i));
      }

      return &valueMap;
    }
#endif

    MollyCodeGenerator makeCodegen() LLVM_OVERRIDE {
      return MollyCodeGenerator(this, getBasicBlock()->getTerminator());
    }


    void applyWhere() {
      auto scop = stmt->getParent();
      auto func = getParentFunction(stmt);
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
      for (auto i = 0; i < nCoords; i+=1) {
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
      return pm->findOrRunAnalysis<Analysis>(nullptr, getParentRegion(stmt));
    }


    std::map<isl_id *, Value *> &getIdToValueMap() LLVM_OVERRIDE {
      auto scop = getParent();
      auto SE = findOrRunAnalysis<ScalarEvolution>();

      auto &validParams = stmt->getValidParams();
      auto domain = getDomain();
      auto nDims = domain.getDimCount();
      auto expectedIds = nDims + validParams.size(); 
      assert(idToValue.size() <= expectedIds);
      if (idToValue.size() == expectedIds)
        return idToValue;

      // 1. Valid parameters of this ScopStmt
      for (auto &param : validParams) {
        getValueOf(param);
      }

      // 2. The loop induction variables
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto iv = const_cast<PHINode *>(stmt->getInductionVariableForDimension(i));
        auto id = domain.getDimId(i);
        assert(id.isValid());
        idToValue[id.keep()] = iv;
      }

      return idToValue;
    }

    std::vector<llvm::Value *> getDomainValues() LLVM_OVERRIDE {
      auto domain = getDomain();
      auto nDims = domain.getDimCount();

      std::vector<llvm::Value *> result;
      for (auto i = nDims-nDims; i < nDims; i+=1) {
        auto iv = const_cast<PHINode *>(stmt->getInductionVariableForDimension(i));
        result.push_back(iv);
      }

      return result;
    }


    void dump() LLVM_OVERRIDE {
      dbgs() << "ScopStmtProcessorImpl:\n";
      if (stmt)
        stmt->dump();
    }


    // isValid
    bool isFieldAccess() const LLVM_OVERRIDE {
      assert(!!fvar == !!fmemacc);
      return fvar != nullptr;
    }


    FieldVariable *getFieldVariable() const {
      assert(fvar);
      return fvar;
    }


    FieldType *getFieldType() const { 
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

    isl::Map/*iteration coord -> field coord*/ getAccessRelation() const {
      assert(isFieldAccess());
      auto fty = getFieldType();
      auto ftyTuple = fty->getIndexsetTuple();
      auto rel = isl::enwrap(fmemacc->getAccessRelation());
      rel.setOutTupleId_inplace(ftyTuple);//TODO: This should have been done when Molly detects SCoPs
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

  }; // class MollyScopStmtProcessorImpl
} // namespace 


MollyScopStmtProcessor *MollyScopStmtProcessor::create(MollyPassManager *pm, polly::ScopStmt *stmt) {
  return new MollyScopStmtProcessorImpl(pm, stmt);
}
