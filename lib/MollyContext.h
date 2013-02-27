#ifndef MOLLY_MOLLYCONTEXT_H
#define MOLLY_MOLLYCONTEXT_H 1

#include "islpp/Ctx.h"

#include <llvm/ADT/OwningPtr.h>
#include <cassert>


namespace llvm {
  class LLVMContext;
  class Module;
} // namespace llvm

namespace molly {
 class IslCtx;
} // namespace molly


namespace molly {
  class MollyContext {
  private:
    llvm::LLVMContext *llvmContext;
    llvm::OwningPtr<isl::Ctx> islCtx;
    //llvm::Module *module; 

  protected:
    MollyContext(llvm::LLVMContext *llvmContext) {
      this->llvmContext = llvmContext;
      islCtx.reset(isl::Ctx::create()); //TODO: We really should get the same isl_ctx from Polly, stored in the polly::ScopInfo pass
    }

  public:
    static MollyContext *create(llvm::LLVMContext *llvmContext) {
      return new MollyContext(llvmContext);
    }

    void setLLVMContext(llvm::LLVMContext *llvmContext) { assert(!this->llvmContext ||  this->llvmContext == llvmContext); this->llvmContext = llvmContext; }
    llvm::LLVMContext *getLLVMContext() { return llvmContext; }
    isl::Ctx *getIslContext() { return islCtx.get(); }
    //llvm::Module *getModule() { return module; }

  }; // class MollyContext
} // namespace molly
#endif /* MOLLY_MOLLYCONTEXT_H */