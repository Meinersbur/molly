#ifndef ISLPP_CTX_H
#define ISLPP_CTX_H

#include "islpp_common.h"
#include <isl/options.h>

//#include <llvm/ADT/PointerIntPair.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Compiler.h>
#include <cassert>
#include "Islfwd.h"

struct isl_ctx;

namespace llvm {
  template<typename T> class OwningPtr;
  class StringRef;
  class Twine;
} // namespace llvm


namespace isl {


  enum class OnErrorEnum {
    Warn = ISL_ON_ERROR_WARN,
    Continue = ISL_ON_ERROR_CONTINUE,
    Abort = ISL_ON_ERROR_ABORT
  };
  //namespace OnError {
  //  static const OnErrorEnum Warn = OnErrorWarn;
  //  static const OnErrorEnum Continue = OnErrorContinue;
  //  static const OnErrorEnum Abort = OnErrorAbort;
  //}
  static const OnErrorEnum xAbort = isl::OnErrorEnum::Abort;

  template <typename T>
  class Owning;

  /// All manipulations of integer sets and relations occur within the context of an isl_ctx. A given isl_ctx can only be used within a single thread. All arguments of a function are required to have been allocated within the same context. There are currently no functions available for moving an object from one isl_ctx to another isl_ctx. This means that there is currently no way of safely moving an object from one thread to another, unless the whole isl_ctx is moved.
  class Ctx {
  private:
    Ctx() LLVM_DELETED_FUNCTION;
    Ctx(const Ctx &) LLVM_DELETED_FUNCTION;
    const Ctx &operator=(const Ctx &) LLVM_DELETED_FUNCTION;

#pragma region Low-Level
  public:
    isl_ctx *keep() const { assert(this); return const_cast<isl_ctx*>(reinterpret_cast<const isl_ctx*>(this)); }

    static Ctx *enwrap(isl_ctx *ctx) { return reinterpret_cast<Ctx*>(ctx); }
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
    Space createMapSpace(unsigned n_in/*domain*/, unsigned n_out/*range*/);
    Space createMapSpace(Space &&domain, Space &&range);
    Space createMapSpace(Space domain, unsigned n_out);
    Space createMapSpace(unsigned n_in, Space &&range);
    Space createMapSpace(unsigned n_in, const Space &range);

    Space createParamsSpace(unsigned nparam);
    Space createSetSpace(unsigned nparam, unsigned dim);
    Space createSetSpace(unsigned dim);
#pragma endregion


#pragma region Create affine expressions
    Aff createZeroAff(LocalSpace &&space);
#pragma endregion

    //#pragma region Create local spaces
    //     LocalSpace createSetLocalSpace(unsigned nparam/*params*/, unsigned n, unsigned divs);
    //     LocalSpace createMapLocalSpace(unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/, unsigned divs);
    //#pragma endregion


#pragma region Create BasicSets
    BasicSet createRectangularSet(llvm::ArrayRef<unsigned> lengths);

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
    Map createAlltoallMap(const Set &domain, Set &&range);
    Map createAlltoallMap(Set &&domain, const Set &range);
    Map createAlltoallMap(const Set &domain, const Set &range);
    //Map createAlltoallMap(const Set &domain, const MultiPwAff &range);

    Map readMap(const char *str);
    Map readMap(const std::string &str);
#pragma endregion


#pragma region Create BasicMaps
    BasicMap createBasicMap(unsigned nparam, unsigned in, unsigned out, unsigned extra, unsigned n_eq, unsigned n_ineq);
    BasicMap createUniverseBasicMap(Space &&space);
#pragma endregion


    UnionMap createEmptyUnionMap();
    UnionSet createEmptyUnionSet();
    //AstBuild createAstBuild();
   


    Id createId(const char *name = nullptr, const void *user = nullptr);
      Id createId(const std::string &name, const void *user = nullptr);
    Id createId(llvm::StringRef name, const void *user = nullptr);
    Id createId(const llvm::Twine& name, const void *user = nullptr);
  }; // class Ctx

  static inline Ctx *enwrap(isl_ctx *ctx) { return Ctx::enwrap(ctx); }

} // namespace isl
#endif /* ISLPP_CTX_H */
