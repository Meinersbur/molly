#ifndef ISLPP_CTX_H
#define ISLPP_CTX_H

#include "islpp_common.h"
#include <isl/options.h>

//#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Compiler.h>
#include <cassert>
#include "Union.h"

struct isl_ctx;

namespace llvm {
  template<typename T> class OwningPtr;
} // namespace llvm

namespace isl {
  class Set;
  class Map;
  class Space;
  class LocalSpace;
  class BasicSet;
  class Printer;
  class BasicMap;
  class Aff;
  class Id;
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
#define Ctx Ctx LLVM_FINAL
  class Ctx {
#undef Ctx
  private:
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
    Space createMapSpace(unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/);
    Space createMapSpace(Space &&domain, Space &&range);
    Space createMapSpace(Space &&domain, unsigned n_out);
    Space createMapSpace(const Space &domain, unsigned n_out);
    Space createMapSpace(unsigned n_in, Space &&range);
    Space createMapSpace(unsigned n_in, const Space &range);

    Space createParamsSpace(unsigned nparam);
    Space createSetSpace(unsigned nparam, unsigned dim);
#pragma endregion


#pragma region Create affine expressions
    Aff createZeroAff(LocalSpace &&space);
#pragma endregion

    //#pragma region Create local spaces
    //     LocalSpace createSetLocalSpace(unsigned nparam/*params*/, unsigned n, unsigned divs);
    //     LocalSpace createMapLocalSpace(unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/, unsigned divs);
    //#pragma endregion


#pragma region Create BasicSets
    BasicSet createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths);
    BasicSet readBasicSet(const char *str);
#pragma endregion


#pragma region Create Maps
    Map createMap(unsigned nparam, unsigned in, unsigned out, int n/*number of BasicMaps*/, unsigned flags = 0);
    Map createEmptyMap(Space &&space);
    Map createEmptyMap(Space &&domainSpace, Space &&rangeSpace);
    Map createEmptyMap(const Space &domainSpace, Space &&rangeSpace);
    Map createEmptyMap(const BasicSet &domain, Space &&rangeSpace);
    Map createEmptyMap(const BasicSet &domain, const Set &range);
    Map createEmptyMap(Set &&domain, Set &&range);
    Map createUniverseMap(Space &&space);
    Map createAlltoallMap(Set &&domain, Set &&range);
    Map createAlltoallMap(const Set &domain, Set &&range) ;
    Map createAlltoallMap(Set &&domain, const Set &range);
    Map createAlltoallMap(const Set &domain, const Set &range);
#pragma endregion


#pragma region Create BasicMaps
    BasicMap createBasicMap(unsigned nparam, unsigned in, unsigned out, unsigned extra, unsigned n_eq, unsigned n_ineq);
    BasicMap createUniverseBasicMap(Space &&space);
#pragma endregion


    UnionMap createEmptyUnionMap();

    Id createId(const char *name = nullptr, void *user = nullptr);
  }; // class Ctx

  static inline Ctx *enwrap(isl_ctx *ctx) { return Ctx::wrap(ctx); }

} // namespace isl
#endif /* ISLPP_CTX_H */
