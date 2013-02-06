#ifndef ISLPP_CTX_H
#define ISLPP_CTX_H

#include <isl/options.h>

//#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Compiler.h>
#include <cassert>

struct isl_ctx;

namespace isl {
  class Set;
  class Space;
  class BasicSet;
} // namespace isl


namespace isl {
  enum OnErrorEnum {
    OnErrorWarn = ISL_ON_ERROR_WARN,
    OnErrorContinue = ISL_ON_ERROR_CONTINUE,
    OnErrorAbort = ISL_ON_ERROR_ABORT
  };
  namespace OnError {
    static const OnErrorEnum Warn =  OnErrorWarn;
    static const OnErrorEnum Continue =  OnErrorContinue;
    static const OnErrorEnum Abort =  OnErrorAbort;
  };

  /// All manipulations of integer sets and relations occur within the context of an isl_ctx. A given isl_ctx can only be used within a single thread. All arguments of a function are required to have been allocated within the same context. There are currently no functions available for moving an object from one isl_ctx to another isl_ctx. This means that there is currently no way of safely moving an object from one thread to another, unless the whole isl_ctx is moved.
  class Ctx {
    friend class Set;
    friend class Id;

  private:
    isl_ctx *ctx;
    //llvm::PointerIntPair<isl_ctx*,1, bool> ctx;

    Ctx(const Ctx &) LLVM_DELETED_FUNCTION;
    const Ctx &operator=(const Ctx &) LLVM_DELETED_FUNCTION;

#pragma region Low-Level
  public:
    isl_ctx *take() { assert(ctx); isl_ctx *result = ctx; ctx = nullptr; return result; }
    isl_ctx *keep() const { assert(ctx); return ctx; }
    void give(isl_ctx *ctx) { assert(!this->ctx); this->ctx = ctx; }
#pragma endregion

  public:
    /// @brief Creates a new ISL context
    /* implicit */ Ctx();
    ~Ctx();

    isl_error getLastError() const;
    void resetLastError();
    void setOnError(OnErrorEnum val);
    OnErrorEnum getOnError() const;

    Space createSpace(unsigned nparam, unsigned dim);
    //IslBasicSet createSet(const IslSpace &space);
    //IslBasicSet createSet(IslSpace &&space);

    BasicSet createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths);
  }; // class Ctx



  class OwningCtx : public Ctx {
  }; // class OwningCtx


  class ForeignCtx : public Ctx {
  }; // class ForeignCtx


} // namespace isl
#endif /* ISLPP_CTX_H */