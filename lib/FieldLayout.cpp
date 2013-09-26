#include "FieldLayout.h"

#include "RectangularMapping.h"
#include "islpp/PwMultiAff.h"

using namespace molly;


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
