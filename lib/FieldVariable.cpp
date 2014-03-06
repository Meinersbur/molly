#include "FieldVariable.h"

#include "FieldType.h"
#include "FieldLayout.h"

#include "islpp/Id.h"
#include "islpp/Space.h"
#include "islpp/PwMultiAff.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/ADT/SmallString.h>

#include <assert.h>

using namespace llvm;
using namespace molly;


FieldVariable::FieldVariable(llvm::GlobalVariable *variable, FieldType *fieldTy) 
  : variable(variable), fieldTy(fieldTy) , defaultLayout(nullptr) {
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
  //TODO: Check for invalid characters
  return getIslContext()->createId(Twine("fvar_") + variable->getName(), variable);
}


isl::Space FieldVariable::getAccessSpace() {
  return getFieldType()->getIndexsetSpace().setSetTupleId(getTupleId());
}


FieldLayout * molly::FieldVariable::getLayout() {
  // Currently there is a layout per type
  // Future version may have a layout per variable or even dynamic at runtime with automatic conversion between them
  return fieldTy->getDefaultLayout();
}


llvm::Type * molly::FieldVariable::getEltType() {
  return fieldTy->getEltType();
}


llvm::Type * molly::FieldVariable::getEltPtrType() {
  return fieldTy->getEltPtrType();
}


isl::PwMultiAff molly::FieldVariable::getHomeAff() {
  auto layout = getLayout();
  return layout->getHomeAff().setInTupleId(getTupleId());
}


FieldLayout *molly::FieldVariable::getDefaultLayout() const {
  if (defaultLayout)
    return defaultLayout;
  return fieldTy->getDefaultLayout();
}
