// This file contains the implementatations of functions that require the LLVMMolly project being linked
#include <polly/MollyFieldAccess.h>

#include "MollyContext.h"
#include "FieldVariable.h"
#include "islpp/Aff.h"
#include "islpp/Ctx.h"
#include "islpp/Space.h"

using namespace llvm;
using namespace polly;
using namespace molly;



FieldType *FieldAccess::getFieldType() {
  return fieldvar->getFieldType();
}


isl::Space FieldAccess::getLogicalSpace(isl::Ctx* ctx) {
  return isl::Space::wrap(isl_getLogicalSpace(ctx->keep()));
}
