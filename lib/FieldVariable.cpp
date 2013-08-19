#include "FieldVariable.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <assert.h>

using namespace llvm;
using namespace molly;


FieldVariable::FieldVariable( llvm::GlobalVariable *variable, FieldType *fieldTy) 
  : variable(variable), fieldTy(fieldTy) {
  assert(variable);
  assert(fieldTy);
}


void FieldVariable::dump() {
  // Nothing to dump yet
}
