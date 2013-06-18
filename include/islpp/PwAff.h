#ifndef ISLPP_PWAFF_H
#define ISLPP_PWAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include <cassert>
#include <string>
#include <functional>
#include <isl/space.h> // enum isl_dim_type;

struct isl_pw_aff;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class Space;
  class Aff;
  class Set;
  class LocalSpace;
  class Id;
  class Int;
} // namespace isl


namespace isl {
  template<>
  class Pw<Aff> LLVM_FINAL {
#ifndef NDEBUG
    std::string _printed;
#endif

#pragma region Low-level
  private:
    isl_pw_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_pw_aff *take() { assert(aff); isl_pw_aff *result = aff; aff = nullptr; return result; }
    isl_pw_aff *takeCopy() const;
    isl_pw_aff *keep() const { return aff; }
  protected:
    void give(isl_pw_aff *aff);

  public:
    static PwAff wrap(isl_pw_aff *aff) { PwAff result; result.give(aff); return result; }
#pragma endregion

  public:
    Pw(void) : aff(nullptr) {}
    /* implicit */ Pw(const PwAff &that) : aff(nullptr) { give(that.takeCopy()); }
    /* implicit */ Pw(PwAff &&that) : aff(nullptr) { give(that.take()); }
    ~Pw(void);

    const PwAff &operator=(const PwAff &that) { give(that.takeCopy()); return *this; }
    const PwAff &operator=(PwAff &&that) { give(that.take()); return *this; }

#pragma region Creational
    static PwAff createFromAff(Aff &&aff); 
    static PwAff createEmpty(Space &&space);
    static PwAff create(Set &&set, Aff &&aff);
    static PwAff createZeroOnDomain(LocalSpace &&space);
    static PwAff createVarOnDomain(LocalSpace &&ls, isl_dim_type type, unsigned pos);
    static PwAff createIndicatorFunction(Set &&set);

    static PwAff readFromStr(Ctx *ctx, const char *str);

    PwAff copy() const { return PwAff::wrap(takeCopy()); }
    PwAff &&move() { return std::move(*this); }
#pragma endregion

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
#pragma endregion

    Ctx *getCtx() const;
    Space getDomainSpace() const;
    Space getSpace() const;

    const char *getDimName(isl_dim_type type, unsigned pos) const;
    bool hasDimId(isl_dim_type type, unsigned pos) const;
    Id getDimId(isl_dim_type type, unsigned pos) const;
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);

    bool isEmpty() const;

    unsigned dim(isl_dim_type type) const;
    bool involvesDim(isl_dim_type type, unsigned first, unsigned n) const;
    bool isCst() const;

    void alignParams(Space &&model);

    Id getTupleId(isl_dim_type type);
    void setTupleId(isl_dim_type type, Id &&id);

    void neg();
    void ceil();
    void floor();
    void mod(const Int &mod);

    void intersectParams(Set &&set);
    void intersetDomain(Set &&set);

    void scale(const Int &f);
    void scaleDown(const Int &f);

    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);

    void coalesce();
    void gist(Set &&context);
    void gistParams(Set &&context);

    void pullback(MultiAff &&ma);
    void pullback(PwMultiAff &&pma);

    int nPiece() const;
    bool foreachPeace(std::function<bool(Set,Aff)> fn) const;
  }; // class PwAff


  bool plainIsEqual(PwAff pwaff1, PwAff pwaff2);

  PwAff unionMin(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff unionMax(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff unionAdd(PwAff &&pwaff1, PwAff &&pwaff2);

  Set domain(PwAff &&pwaff);

  PwAff min(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff max(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff mul(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff div(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff add(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff sub(PwAff &&pwaff1, PwAff &&pwaff2);

  PwAff tdivQ(PwAff &&pa1, PwAff &&pa2);
  PwAff tdivR(PwAff &&pa1, PwAff &&pa2);

  PwAff cond(PwAff &&cond, PwAff &&pwaff_true, PwAff &&pwaff_false);

  /// Return a set containing those elements in the domain of pwaff where it is non-negative.
  Set nonnegSet(PwAff &pwaff); 
  Set zeroSet(PwAff &pwaff); // alternative name: preimageOfZero
  Set nonXeroSet(PwAff &pwaff);

  Set eqSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set neSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set leSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set ltSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set geSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set gtSet(PwAff &&pwaff1, PwAff &&pwaff2);

  static inline PwAff operator-(PwAff &&lhs, PwAff &&rhs) { return sub(lhs.move(),rhs.move()); }
  static inline PwAff operator-(const PwAff &lhs, PwAff &&rhs) { return sub(lhs.copy(),rhs.move()); }
  static inline PwAff operator-(PwAff &&lhs, const PwAff &rhs) { return sub(lhs.move(),rhs.copy()); }
  static inline PwAff operator-(const PwAff &lhs, const PwAff &rhs) { return sub(lhs.copy(),rhs.copy()); }

} // namespace isl
#endif /* ISLPP_PWAFF_H */
