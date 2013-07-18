#ifndef MOLLY_ISLEXPRBUILDER_H
#define MOLLY_ISLEXPRBUILDER_H

#include <llvm/IR/IRBuilder.h>
#include "islpp/Islfwd.h"
#include <isl/id.h>

namespace molly {
  llvm::Value* buildIslAff(llvm::IRBuilder<> &builder, const isl::Aff &aff, std::map<isl_id *, llvm::Value *> &values);
} // namespace molly
#endif /* MOLLY_ISLEXPRBUILDER_H */
