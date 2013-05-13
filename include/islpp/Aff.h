#ifndef ISLPP_AFF_H
#define ISLPP_AFF_H

#include "islpp_common.h"
#include <cassert>
#include <string>
#include <isl/space.h> // enum isl_dim_type;

struct isl_aff;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class LocalSpace;
  class Space;
  class Int;
  class Id;
  class Set;
  class BasicSet;
  class Aff;

  template<typename T> class Multi;
  template<> class Multi<Aff>;
} // namespace isl


namespace isl {

  class Aff {
#pragma region Low-level
  private:
    isl_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_aff *take() { assert(aff); isl_aff *result = aff; aff = nullptr; return result; }
    isl_aff *takeCopy() const;
    isl_aff *keep() const { return aff; }
  protected:
    void give(isl_aff *aff);

  public:
    static Aff wrap(isl_aff *aff) { Aff result; result.give(aff); return result; }
#pragma endregion

  public:
    Aff(void) : aff(nullptr) {}
    /* implicit */ Aff(const Aff &that) : aff(that.takeCopy()) {}
    /* implicit */ Aff(Aff &&that) : aff(that.take()) { }
    ~Aff(void);

    const Aff &operator=(const Aff &that) { give(that.takeCopy()); return *this; }
    const Aff &operator=(Aff &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Aff createZeroOnDomain(LocalSpace &&space);
    static Aff createVarOnDomain(LocalSpace &&space, isl_dim_type type, unsigned pos);

    static Aff readFromString(Ctx *ctx, const char *str);
#pragma endregion

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion

    Ctx *getCtx() const;
    int dim(isl_dim_type type) const;
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const;
    Space getDomainSpace() const;
    Space getSpace() const;
    LocalSpace getDomainLocalSpace() const;
    LocalSpace getLocalSpace() const;

    const char *getDimName( isl_dim_type type, unsigned pos) const;
    Int getConstant() const;
    Int getCoefficient(isl_dim_type type, unsigned pos) const;
    Int getDenominator() const;

    void setConstant(const Int &);
    void setCoefficient(isl_dim_type type, unsigned pos, int);
    void setCoefficient(isl_dim_type type, unsigned pos, const Int &);
    void setDenominator(const Int &);

    void addConstant(const Int &);
    void addConstant(int);
    void addConstantNum(const Int &);
    void addConstantNum(int);
    void addCoefficient(isl_dim_type type, unsigned pos, int);
    void addCoefficient(isl_dim_type type, unsigned pos, const Int &);

    bool isCst() const;

    void setDimName(isl_dim_type type, unsigned pos, const char *s);
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);

    bool isPlainZero() const;
    Aff getDiv(int pos) const;

    void neg();
    void ceil();
    void floor();
    void mod(const Int &mod);

    void scale(const Int &f);
    void scaleDown(const Int &f);
    void scaleDown(unsigned f);

    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);

    void projectDomainOnParams();

    void alignParams(Space &&model);
    void gist(Set &&context);
    void gistParams(Set &&context);

    void pullbackMultiAff(Multi<Aff> &&);
  }; // class Aff

  bool isPlainEqual(const Aff &aff1, const Aff &aff2);
  Aff mul( Aff &&aff1, Aff &&aff2);
  Aff div( Aff &&aff1, Aff &&aff2);
  Aff add( Aff &&aff1, Aff &&aff2);
  //Aff sub( Aff &&aff1, Aff &&aff2);


  BasicSet zeroBasicSet(Aff &&aff);
  BasicSet negBasicSet(Aff &&aff);
  BasicSet leBasicSet(Aff &aff1, Aff &aff2);
  BasicSet gtBasicSet(Aff &aff1, Aff &aff2);


} // namespace isl
#endif /* ISLPP_AFF_H */
