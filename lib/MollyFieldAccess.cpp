// This file contains the implementatations of functions that require the LLVMMolly project being linked
#include "MollyFieldAccess.h"

#include "MollyContext.h"
#include "FieldVariable.h"
#include "islpp/Aff.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"

using namespace llvm;
using namespace molly;


MollyFieldAccess MollyFieldAccess::fromAccessInstruction(llvm::Instruction *instr) {
  auto pollyFieldAcc = polly::FieldAccess::fromAccessInstruction(instr);
  MollyFieldAccess mollyFieldAcc(pollyFieldAcc);
  return mollyFieldAcc;
}


FieldType *MollyFieldAccess::getFieldType() {
  return fieldvar->getFieldType();
}


isl::Space MollyFieldAccess::getLogicalSpace(isl::Ctx* ctx) {
  return isl::Space::wrap(isl_getLogicalSpace(ctx->keep()));
}
