#ifndef MOLLY_ISLCTX_H
#define MOLLY_ISLCTX_H 1

#include <llvm/Support/Compiler.h>


struct isl_ctx;

namespace molly {
  class IslSet;
  class IslSpace;

  class IslCtx {
    IslCtx(const IslCtx &) LLVM_DELETED_FUNCTION;
    const IslCtx &operator=(const IslCtx &) LLVM_DELETED_FUNCTION;

  private:
    isl_ctx *ctx;

  public:
    IslCtx();
    ~IslCtx();

    IslSpace createSpace(unsigned nparam, unsigned dim);
  };

}

#endif /* MOLLY_ISLCTX_H */
