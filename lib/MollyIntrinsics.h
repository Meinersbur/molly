#ifndef MOLLY_MOLLYINTRINSICS_H
#define MOLLY_MOLLYINTRINSICS_H

#include "LLVMfwd.h"
//#include <llvm/IR/IRBuilder.h>

namespace molly {
  class FieldVariable;
  class CommunicationBuffer;
} // namespace molly


namespace molly {
  llvm::CallInst *callLocalPtrIntrinsic(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Instruction *insertBefore = nullptr);

  //llvm::CallInst *callCombufSend(, llvm::Instruction *insertBefore = nullptr)


  llvm::Value *codegenReadLocal(DefaultIRBuilder &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);
  void codegenWriteLocal(DefaultIRBuilder &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Value *val);

  void codegenSendBuf(DefaultIRBuilder &builder, molly::CommunicationBuffer *buf, llvm::Value *dstCoord) ;
} // namespace molly
#endif /* MOLLY_MOLLYINTRINSICS_H */
