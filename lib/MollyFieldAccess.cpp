// This file contains the implementatations of functions that require the LLVMMolly project being linked
#include "MollyFieldAccess.h"

#include "MollyContext.h"
#include "FieldVariable.h"
#include "islpp/Aff.h"
#include "islpp/MultiAff.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Map.h"

using namespace llvm;
using namespace molly;


MollyFieldAccess MollyFieldAccess::fromAccessInstruction(llvm::Instruction *instr) {
  auto pollyFieldAcc = polly::FieldAccess::fromAccessInstruction(instr);
  MollyFieldAccess mollyFieldAcc(pollyFieldAcc);
  return mollyFieldAcc;
}


MollyFieldAccess MollyFieldAccess::fromMemoryAccess(polly::MemoryAccess *acc) {
  auto instr = const_cast<Instruction*>(acc->getAccessInstruction());
  auto result = fromAccessInstruction(instr);
  result.augmentMemoryAccess(acc);
  return result;
}


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


FieldType *MollyFieldAccess::getFieldType() {
  return fieldvar->getFieldType();
}


isl::Space MollyFieldAccess::getLogicalSpace(isl::Ctx* ctx) {
  return isl::Space::wrap(isl_getLogicalSpace(ctx->keep()));
}


polly::MemoryAccess *MollyFieldAccess::getPollyMemoryAccess() {
  assert(scopAccess && "Need to augment the access using SCoP");
  return scopAccess;
}


polly::ScopStmt *MollyFieldAccess::getPollyScopStmt() {
  assert(scopAccess && "Need to augment the access using SCoP");
  auto result = scopAccess->getStatement();
  assert(result);
  return result;
}


isl::MultiAff MollyFieldAccess::getAffineAccess() {
  SmallVector<llvm::Value*,4> coords;
  getCoordinates(coords);

  isl::MultiAff result;

  for (auto it = coords.begin(), end = coords.end(); it!=end; ++it) {
    auto coord = *it;

    isl::Aff aff;

    result.append(std::move(aff));
  }

  return result;
}


isl::Map MollyFieldAccess::getAccessedRegion() {
  isl::Map result;
  return result;
}
