// This file contains the implementatations of functions that require the LLVMMolly project being linked
#include "MollyFieldAccess.h"

#include "FieldVariable.h"
#include "ScopUtils.h"
#include "FieldType.h"
#include "FieldLayout.h"

#include "islpp/Aff.h"
#include "islpp/MultiAff.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Map.h"

#include <llvm/Analysis/ScalarEvolution.h>

using std::move;
using namespace llvm;
using namespace molly;
using isl::enwrap;

#if 0
void MollyFieldAccess::augmentFieldDetection(FieldDetectionAnalysis *fields) {
  assert(fields);
  if (isNull())
    return;

  if (!fieldvar) {
    auto base = getBaseField();
    auto globalbase = dyn_cast<GlobalVariable>(base);
    assert(globalbase && "Currently only global fields supported");
    auto gvar = fields->getFieldVariable(globalbase);
    this->fieldvar = gvar;
  }
}
#endif

void MollyFieldAccess::loadFromMemoryAccess(polly::MemoryAccess *acc, FieldVariable * fvar) {
  assert(acc);
  loadFromInstruction(const_cast<Instruction*>(acc->getAccessInstruction()));
  this->scopAccess = acc;
  this->scopStmt = acc->getStatement();
  this->fieldvar = fvar;
}


void MollyFieldAccess::loadFromScopStmt(polly::ScopStmt *stmt, FieldVariable *fvar) {
  assert(stmt);
  bool found = false;
  for (auto it = stmt->memacc_begin(), end = stmt->memacc_end(); it!=end; ++it) {
    auto memacc = *it;
    loadFromMemoryAccess(memacc, fvar);
    if (isValid()) { // There must be only one MemoryAccess to a field 
      assert( this->scopStmt == stmt);
      return;
    }
  }
}


MollyFieldAccess MollyFieldAccess::fromAccessInstruction(llvm::Instruction *instr,FieldVariable * fvar) {
  MollyFieldAccess result;
  result.loadFromInstruction(instr, fvar);
  return result;
}


MollyFieldAccess MollyFieldAccess::fromMemoryAccess(polly::MemoryAccess *acc, FieldVariable *fvar) {
  MollyFieldAccess result;
  result.loadFromMemoryAccess (acc, fvar);
  return result;
}


MollyFieldAccess MollyFieldAccess::fromScopStmt(polly::ScopStmt *stmt, FieldVariable *fvar) {
  MollyFieldAccess result;
  result.loadFromScopStmt (stmt, fvar);
  return result;
}

#if 0
MollyFieldAccess  MollyFieldAccess::create(llvm::Instruction *instr, polly::MemoryAccess *acc, FieldVariable *fvar) {
  auto result = fromAccessInstruction(instr);
  result.scopAccess = acc;
  result.fieldvar = fvar;
  return result;
}


MollyFieldAccess MollyFieldAccess::fromMemoryAccess(polly::MemoryAccess *acc, FieldDetectionAnalysis *fields) {
  assert(acc);
  auto instr = const_cast<Instruction*>(acc->getAccessInstruction());
  if (instr) {
    auto result = fromAccessInstruction(instr);
    result.scopAccess = acc;
    //result.augmentMemoryAccess(acc);

    if (!result.fieldvar) {
      auto base = result.getBaseField();
      auto globalbase = dyn_cast<GlobalVariable>(base);
      assert(globalbase && "Currently only global fields supported");
      auto gvar = fields->getFieldVariable(globalbase);
      result.fieldvar = gvar;
    }

    return result;
  } else {
    // This is a pro- or epilogue dummy stmt
    MollyFieldAccess result;
    result.fieldvar = acc->getFieldVariable();
    assert(result.fieldvar);
    result.reads = acc->getAccessType() | polly::MemoryAccess::READ;
    result.writes = (acc->getAccessType() | polly::MemoryAccess::MAY_WRITE) || (acc->getAccessType() | polly::MemoryAccess::MUST_WRITE);
    result.scopAccess = acc;

    if (!result.fieldvar) {
      auto base = result.getBaseField();
      auto globalbase = dyn_cast<GlobalVariable>(base);
      assert(globalbase && "Currently only global fields supported");
      auto gvar = fields->getFieldVariable(globalbase);
      result.fieldvar = gvar;
    }

    return result;
  }
}
#endif


#if 0
void MollyFieldAccess::augmentMemoryAccess(polly::MemoryAccess *acc) {
  assert(acc);
  assert(acc->getAccessInstruction() == getAccessor());
  this->scopAccess = acc;
}


void MollyFieldAccess::augmentFieldVariable(FieldVariable *fieldvar) {
  assert(fieldvar);
  assert(!this->fieldvar || this->fieldvar == fieldvar);
  this->fieldvar = fieldvar;
}
#endif

FieldType *MollyFieldAccess::getFieldType() const { 
  assert(fieldvar);
  return fieldvar->getFieldType();
}


isl::Space MollyFieldAccess::getLogicalSpace(isl::Ctx* ctx) {
  return isl::Space::enwrap(isl_getLogicalSpace(ctx->keep()));
}


polly::MemoryAccess *MollyFieldAccess::getPollyMemoryAccess() const {
  assert(scopAccess && "Need to augment the access using SCoP");
  return scopAccess;
}


polly::ScopStmt *MollyFieldAccess::getPollyScopStmt() const {
  assert(scopAccess && "Need to augment the access using SCoP");
  auto result = scopAccess->getStatement();
  assert(result);
  return result;
}


isl::MultiPwAff MollyFieldAccess::getAffineAccess(llvm::ScalarEvolution *se) {
  SmallVector<llvm::Value*,4> coords;
  getCoordinates(coords);
  auto scopStmt = getPollyScopStmt();

  auto iterDomain = molly::getIterationDomain(getPollyScopStmt());
  auto indexDomain = getFieldType()->getGlobalIndexset();
  auto result = isl::Space::createMapFromDomainAndRange(iterDomain.getSpace(), indexDomain.getSpace()).createZeroMultiPwAff();
  auto i = 0;
  for (auto it = coords.begin(), end = coords.end(); it!=end; ++it) {
    auto coordVal = *it;
    auto coordSCEV = se->getSCEV(coordVal);

    auto aff = isl::enwrap(affinatePwAff(scopStmt, coordSCEV));
    result.setPwAff_inplace(i, aff);
    i+=1;
  }

  return result;
}


isl::Map MollyFieldAccess::getAccessRelation() const {
  assert(scopAccess);
  auto fvar = getFieldVariable();
  auto rel = isl::enwrap(scopAccess->getAccessRelation());
  rel.setOutTupleId_inplace(fvar->getTupleId());//TODO: This should have been done when Molly detects SCoPs
  return rel;
}


isl::Set MollyFieldAccess::getAccessedRegion() {
  auto result = getAccessRelation().getRange();
  assert(result.getSpace() == getIndexsetSpace());
  return result;
}


isl::Space MollyFieldAccess::getIndexsetSpace() {
  auto memacc = getPollyMemoryAccess();
  assert(memacc);
  auto map = isl::enwrap(memacc->getAccessRelation());
  return map.getRangeSpace();
}


isl::Id MollyFieldAccess::getAccessTupleId() const {
  auto memacc = getPollyMemoryAccess();
  assert(memacc);
  return enwrap(memacc->getTupleId());
}


isl::Map MollyFieldAccess::getAccessScattering() const {
  auto scattering = getScattering(const_cast<MollyFieldAccess*>(this)->getPollyScopStmt());
  scattering.setInTupleId_inplace(getAccessTupleId());
  return scattering;
}


isl::PwMultiAff MollyFieldAccess::getHomeAff() const {
  auto layout = getFieldVariable()->getLayout();
  auto tyHomeAff = layout->getHomeAff();
  tyHomeAff.setTupleId_inplace(isl_dim_in, getAccessRelation().getOutTupleId() );
  return tyHomeAff;
}


llvm::StoreInst *MollyFieldAccess::getLoadUse() const {
  auto ld = getLoadInst();

  // An access within a Scop should have been isolated into its own ScopStmt by MollyPassManager::isloateFieldAccesses
  // and  IndependentBlocks made independent by writing the result to a local variable that is picked up by those ScopStmt that need it
  assert( this->getPollyScopStmt() );
  assert(ld->getNumUses() == 1);

  auto use = *ld->use_begin();
  return cast<StoreInst>(use);
}
