#ifndef MOLLY_MOLLYCONTEXT_H
#define MOLLY_MOLLYCONTEXT_H 1

#include "IslCtx.h"

#include <assert.h>


namespace llvm {
  class LLVMContext;
}

namespace molly {
  class IslCtx;

  class MollyContext {
  private:
    llvm::LLVMContext *llvmContext;
    IslCtx islContext;

  protected:
    MollyContext(llvm::LLVMContext *llvmContext) {
      this->llvmContext = llvmContext;
    }

  public:
    static MollyContext *create(llvm::LLVMContext *llvmContext) {
      return new MollyContext(llvmContext);
    }

    llvm::LLVMContext *getLLVMContext() { return llvmContext; }
    IslCtx *getIslContext() { return &islContext; }

  };

}

#endif /* MOLLY_MOLLYCONTEXT_H */