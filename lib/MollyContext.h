#ifndef MOLLY_MOLLYCONTEXT_H
#define MOLLY_MOLLYCONTEXT_H 1

#include "islpp/Ctx.h"

#include <cassert>


namespace llvm {
  class LLVMContext;
}

namespace molly {
  class IslCtx;

  class MollyContext {
  private:
    llvm::LLVMContext *llvmContext;
    isl::Ctx islCtx;

  protected:
    MollyContext(llvm::LLVMContext *llvmContext) {
      this->llvmContext = llvmContext;
    }

  public:
    static MollyContext *create(llvm::LLVMContext *llvmContext) {
      return new MollyContext(llvmContext);
    }

    void setLLVMContext(llvm::LLVMContext *llvmContext) { assert(!this->llvmContext ||  this->llvmContext == llvmContext); this->llvmContext = llvmContext; }
    llvm::LLVMContext *getLLVMContext() { return llvmContext; }
    isl::Ctx *getIslContext() { return &islCtx; }

  }; // class MollyContext
} // namespace molly
#endif /* MOLLY_MOLLYCONTEXT_H */