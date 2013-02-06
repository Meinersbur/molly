#ifndef MOLLY_MOLLYCONTEXT_H
#define MOLLY_MOLLYCONTEXT_H 1

#include "islpp/Ctx.h"

#include <llvm/ADT/OwningPtr.h>
#include <cassert>


namespace llvm {
  class LLVMContext;
}

namespace molly {
  class IslCtx;

  class MollyContext {
  private:
    llvm::LLVMContext *llvmContext;
    llvm::OwningPtr<isl::Ctx> islCtx;

  protected:
    MollyContext(llvm::LLVMContext *llvmContext) {
      this->llvmContext = llvmContext;
      islCtx.reset(isl::Ctx::create());
    }

  public:
    static MollyContext *create(llvm::LLVMContext *llvmContext) {
      return new MollyContext(llvmContext);
    }

    void setLLVMContext(llvm::LLVMContext *llvmContext) { assert(!this->llvmContext ||  this->llvmContext == llvmContext); this->llvmContext = llvmContext; }
    llvm::LLVMContext *getLLVMContext() { return llvmContext; }
    isl::Ctx *getIslContext() { return islCtx.get(); }

  }; // class MollyContext
} // namespace molly
#endif /* MOLLY_MOLLYCONTEXT_H */