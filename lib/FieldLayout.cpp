#include "FieldLayout.h"

#include "RectangularMapping.h"
#include "islpp/PwMultiAff.h"
//#include <llvm/ADT/SmallVector.h>
//#include "FieldType.h"
//#include <llvm/IR/Type.h>
//#include <llvm/IR/DerivedTypes.h>
//#include "llvm/IR/Function.h"
//#include "llvm/ADT/Twine.h"
//#include "Codegen.h"

using namespace molly;
using namespace llvm;


FieldLayout::~FieldLayout() {
}


llvm::Value *FieldLayout::codegenLocalIndex(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator, isl::MultiPwAff coords) {
  assert(linearizer);
  return linearizer->codegenIndex(codegen, domaintranslator, coords);
}


llvm::Value *FieldLayout::codegenLocalSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator) {
  assert(linearizer);
  return linearizer->codegenSize(codegen, domaintranslator);
}


