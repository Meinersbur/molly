#ifndef ISLPP_CTX_H
#define ISLPP_CTX_H

#include "islpp_common.h"
#include <isl/options.h>

//#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Compiler.h>
#include <cassert>

struct isl_ctx;

namespace llvm {
  template<typename T> class OwningPtr;
} // namespace llvm

namespace isl {
  class Set;
  class Space;
  class BasicSet;
  class Printer;
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
  }

  template <typename T>
  class Owning;

  /// All manipulations of integer sets and relations occur within the context of an isl_ctx. A given isl_ctx can only be used within a single thread. All arguments of a function are required to have been allocated within the same context. There are currently no functions available for moving an object from one isl_ctx to another isl_ctx. This means that there is currently no way of safely moving an object from one thread to another, unless the whole isl_ctx is moved.
  class Ctx {


  private:
    //isl_ctx *ctx;
    //llvm::PointerIntPair<isl_ctx*,1, bool> ctx;

    Ctx() LLVM_DELETED_FUNCTION;
    Ctx(const Ctx &) LLVM_DELETED_FUNCTION;
    const Ctx &operator=(const Ctx &) LLVM_DELETED_FUNCTION;

#pragma region Low-Level
  public:
    isl_ctx *keep() const { assert(this); return const_cast<isl_ctx*>(reinterpret_cast<const isl_ctx*>(this)); }

    static Ctx *wrap(isl_ctx *ctx) { return reinterpret_cast<Ctx*>(ctx); }
#pragma endregion

  public:
    void operator delete(void*);
    ~Ctx();

   static Ctx *create();

#pragma region Error handling
       isl_error getLastError() const;
    void resetLastError();
    void setOnError(OnErrorEnum val);
    OnErrorEnum getOnError() const;
#pragma endregion

#pragma region Create printers
    Printer createPrinterToFile(FILE *file);
    Printer createPrinterToFile(const char *filename);
    Printer createPrinterToStr();
#pragma endregion

#pragma region Create spaces
        Space createSpace(unsigned nparam, unsigned dim);
#pragma endregion

#pragma region Create BasicSets
         BasicSet createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths);
          BasicSet readBasicSet(const char *str);
#pragma endregion
  }; // class Ctx
} // namespace isl
#endif /* ISLPP_CTX_H */
