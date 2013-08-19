#include "AffineMapping.h"

#include <llvm/ADT/ArrayRef.h>
#include "IslExprBuilder.h"
#include <llvm/IR/Module.h>
#include "islpp/Set.h"
#include "islpp/PwAff.h"
#include <polly/CodeGen/CodeGeneration.h>

using namespace llvm;
using namespace molly;
using namespace polly;
using namespace std;
using isl::enwrap;


isl::Set AffineMapping:: getIndexset() const {
  return mapping.getDomain();
}


llvm::Value *AffineMapping::codegen(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value*> coords) {
  // FIXME: coords bot used
  // TODO: This is piecewise, can make SCopStmts instead
  assert(!"To implement");
  return nullptr;
  //return buildIslAff(builder.GetInsertPoint(), mapping, params);
}
