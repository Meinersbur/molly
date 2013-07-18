// This file contains the implementatations of functions that require the LLVMMolly project being linked
#include "MollyFieldAccess.h"

#include "MollyContext.h"
#include "FieldVariable.h"
#include "islpp/Aff.h"
#include "islpp/MultiAff.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Map.h"
#include "SCEVAffinator.h"
#include "ScopUtils.h"
#include "FieldType.h"

#include <llvm/Analysis/ScalarEvolution.h>
#include "FieldDetection.h"

using std::move;
using namespace llvm;
using namespace molly;
using isl::enwrap;


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


MollyFieldAccess MollyFieldAccess::fromAccessInstruction(llvm::Instruction *instr) {
  auto pollyFieldAcc = polly::FieldAccess::fromAccessInstruction(instr);
  MollyFieldAccess mollyFieldAcc(pollyFieldAcc);
  return mollyFieldAcc;
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

FieldType *MollyFieldAccess::getFieldType() { 
  assert(fieldvar);
  return fieldvar->getFieldType();
}


isl::Space MollyFieldAccess::getLogicalSpace(isl::Ctx* ctx) {
  return isl::Space::wrap(isl_getLogicalSpace(ctx->keep()));
}


polly::MemoryAccess *MollyFieldAccess::getPollyMemoryAccess() const {
  assert(scopAccess && "Need to augment the access using SCoP");
  return scopAccess;
}


polly::ScopStmt *MollyFieldAccess::getPollyScopStmt() {
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

    auto aff = convertScEvToAffine(scopStmt, coordSCEV);
    result.setPwAff_inplace(i, aff.move());
    i+=1;
  }

  return result;
}


isl::Map MollyFieldAccess::getAccessRelation() {
  assert(scopAccess);
  return isl::enwrap(scopAccess->getAccessRelation());
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
