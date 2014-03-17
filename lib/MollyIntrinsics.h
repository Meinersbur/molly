#ifndef MOLLY_MOLLYINTRINSICS_H
#define MOLLY_MOLLYINTRINSICS_H

#include "LLVMfwd.h"
#include <llvm/IR/IntrinsicInst.h>

namespace molly {
  class FieldVariable;
  class CommunicationBuffer;
} // namespace molly


namespace molly {
  class MollyModInst : public llvm::IntrinsicInst {
  public:
    llvm::Value *getDivident() const { 
      return getArgOperand(0);
    }

    // Must be a positive constant
    llvm::Value *getDivisor() const {
      return getArgOperand(1);
    }

    // Methods for support type inquiry through isa, cast, and dyn_cast:
    static inline bool classof(const IntrinsicInst *I) {
      return I->getIntrinsicID() == llvm::Intrinsic::molly_mod;
    }
    static inline bool classof(const Value *V) {
      return llvm::isa<llvm::IntrinsicInst>(V) && classof(llvm::cast<llvm::IntrinsicInst>(V));
    }
  }; // class MollyModInst


  llvm::CallInst *callLocalPtrIntrinsic(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Instruction *insertBefore = nullptr);

  //llvm::CallInst *callCombufSend(, llvm::Instruction *insertBefore = nullptr)


  llvm::Value *codegenReadLocal(DefaultIRBuilder &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);
  void codegenWriteLocal(DefaultIRBuilder &builder, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Value *val);

  void codegenSendBuf(DefaultIRBuilder &builder, molly::CommunicationBuffer *buf, llvm::Value *dstCoord) ;
} // namespace molly
#endif /* MOLLY_MOLLYINTRINSICS_H */
