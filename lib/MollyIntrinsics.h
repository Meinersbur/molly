#ifndef MOLLY_MOLLYINTRINSICS_H
#define MOLLY_MOLLYINTRINSICS_H

#include "LLVMfwd.h"
#include <llvm/IR/IRBuilder.h>

namespace molly {
  class FieldVariable;
} // namespace molly


namespace molly {

  llvm::Value *codegenReadLocal(llvm::IRBuilder<> &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);
  void codegenWriteLocal(llvm::IRBuilder<> &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Value *val);

} // namespace molly
#endif /* MOLLY_MOLLYINTRINSICS_H */
