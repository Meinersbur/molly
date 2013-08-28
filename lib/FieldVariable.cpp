#include "FieldVariable.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <assert.h>
#include <llvm/IR/GlobalVariable.h>
#include "islpp/Id.h"
#include "FieldType.h"
#include <llvm/ADT/SmallString.h>

using namespace llvm;
using namespace molly;


FieldVariable::FieldVariable(llvm::GlobalVariable *variable, FieldType *fieldTy) 
  : variable(variable), fieldTy(fieldTy) {
  assert(variable);
  assert(fieldTy);
}


isl::Ctx *FieldVariable::getIslContext() const {
  return fieldTy->getIslContext();
}


void FieldVariable::dump() {
  // Nothing to dump yet
}


isl::Id FieldVariable::getTupleId() {
#if 0
    llvm::SmallString<255> sstr;
    llvm::raw_svector_ostream os(sstr);

    os << "fvar_";
    auto varname = variable->getName();
    if (varname.size() > 0) {
      os << varname << "_";
    }
    os << variable;
#endif
    //TODO: Check for invalid caracters
    return getIslContext()->createId(Twine("fvar_") + variable->getName(), variable);
}


 isl::Space FieldVariable::getAccessSpace() {
   return getFieldType()->getIndexsetSpace().setSetTupleId(getTupleId());
 }
