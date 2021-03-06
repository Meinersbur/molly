#ifndef ISLPP_PWAFF_H
#define ISLPP_PWAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include "Id.h"
#include "Obj.h"
#include "Spacelike.h"
#include "Ctx.h"
#include "Space.h"
#include "Set.h"

#include <isl/space.h> // enum isl_dim_type;
#include <isl/aff.h>

#include <cassert>
#include <string>
#include <functional>

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
  class Map;
} // namespace isl


namespace isl {

  template<>
  class Pw<Aff> : public Obj<PwAff, isl_pw_aff>, public Spacelike<PwAff>{

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_pw_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_pw_aff_copy(keepOrNull()); }

  public:
    Pw() { }
    //static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    /* implicit */ Pw(const ObjTy &that) : Obj(that) { }
    /* implicit */ Pw(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_pw_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_pw_aff_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS Space getSpacelike() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_pw_aff_get_space(keep())); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return true; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_pw_aff_dim(keep(), type); }
      //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_pw_aff_find_dim_by_id(keep(), type, id.keep()); }

      //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_pw_aff_has_tuple_name(keep(), type)); }
      //ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_pw_aff_get_tuple_name(keep(), type); }
      //ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_pw_aff_set_tuple_name(take(), type, s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_pw_aff_has_tuple_id(keep(), type)); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_pw_aff_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_set_tuple_id(take(), type, id.take())); }
      //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_pw_aff_reset_tuple_id(take(), type)); }

      //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_pw_aff_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_pw_aff_get_dim_name(keep(), type, pos); }
      //ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_pw_aff_set_dim_name(take(), type, pos, s)); }
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_pw_aff_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_pw_aff_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_pw_aff_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_drop_dims(take(), type, first, count)); }
#pragma endregion


#pragma region Creational
    static PwAff createFromAff(Aff &&aff);
    static PwAff createEmpty(Space &&space);
    static PwAff create(Set set, Aff aff) { return PwAff::enwrap(isl_pw_aff_alloc(set.take(), aff.take())); }
    static PwAff createZeroOnDomain(LocalSpace &&space);
    static PwAff createVarOnDomain(LocalSpace &&ls, isl_dim_type type, unsigned pos);
    static PwAff createIndicatorFunction(Set &&set);

    static PwAff readFromStr(Ctx *ctx, const char *str);
#pragma endregion


#pragma region Conversion
    // from Aff
    Pw(const Aff &aff) : Obj(isl_pw_aff_from_aff(aff.takeCopy())) {}
    Pw(Aff &&aff) : Obj(isl_pw_aff_from_aff(aff.take())) {}
    const PwAff &operator=(const Aff &aff) LLVM_LVALUE_FUNCTION{ give(isl_pw_aff_from_aff(aff.takeCopy())); return *this; }
    const PwAff &operator=(Aff &&aff) LLVM_LVALUE_FUNCTION{ give(isl_pw_aff_from_aff(aff.take())); return *this; }

    Map toMap() const;

    //Expr toExpr() const;

    MultiPwAff toMultiPwAff() ISLPP_EXSITU_FUNCTION;

    PwMultiAff toPwMultiAff() ISLPP_EXSITU_FUNCTION;
#pragma endregion


    Space getDomainSpace() const;
    ISLPP_PROJECTION_ATTRS Space getRangeSpace() ISLPP_PROJECTION_FUNCTION{ return getSpace().range(); }
    bool isEmpty() const;


    bool involvesDim(isl_dim_type type, unsigned first, unsigned n) const;
    bool isCst() const;

    void alignParams(Space &&model);


    void neg();
    void ceil();
    void floor();
    void mod(const Int &mod);

    ISLPP_EXSITU_ATTRS  PwAff mod(Val divisor) ISLPP_EXSITU_FUNCTION;

    void intersectParams(Set &&set);
    ISLPP_EXSITU_ATTRS PwAff intersectDomain(Set set) ISLPP_EXSITU_FUNCTION{ return PwAff::enwrap(isl_pw_aff_intersect_domain(takeCopy(), set.take())); }
    ISLPP_INPLACE_ATTRS void intersectDomain_inplace(Set set) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_intersect_domain(take(), set.take())); }
    ISLPP_CONSUME_ATTRS PwAff intersectDomain_consume(Set set) ISLPP_CONSUME_FUNCTION{ return PwAff::enwrap(isl_pw_aff_intersect_domain(take(), set.take())); }

    void scale(const Int &f);
    void scaleDown(const Int &f);

    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);

    ISLPP_INPLACE_ATTRS void coalesce_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void gist_inplace(Set context)ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_gist(take(), context.take())); }
    ISLPP_EXSITU_ATTRS PwAff gist(Set context) ISLPP_EXSITU_FUNCTION{ return PwAff::enwrap(isl_pw_aff_gist(takeCopy(), context.take())); }

    void gistParams(Set &&context);

    //PwAff addPiece(const Set &set, const Aff &aff) ISLPP_EXSITU_QUALIFIER { return PwAff::enwrap(isl_pw_aff_add_piece(takeCopy(), set.takeCopy(), aff.takeCopy())); }

    PwAff pullback(const MultiAff &maff) const;
    void pullback_inplace(const MultiAff &maff) ISLPP_INPLACE_FUNCTION;
    PwAff pullback(const PwMultiAff &pmaff) const;
    void pullback_inplace(const PwMultiAff &pma) ISLPP_INPLACE_FUNCTION;
    //PwAff pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER;
    //void pullback_inplace(const MultiPwAff &mpa) ISLPP_INPLACE_QUALIFIER;

    int nPiece() const;
    ISLPP_PROJECTION_ATTRS size_t nPieces() ISLPP_PROJECTION_FUNCTION;
    bool foreachPiece(std::function<bool(Set, Aff)> fn) const;
    std::vector<std::pair<Set, Aff>> getPieces() const;

    /// Returns one of the pieces, which one is not defined
    /// Most useful if nPiece()==1
    /// Do not call when nPiece()==0
    ISLPP_EXSITU_ATTRS std::pair<Set, Aff> anyPiece() ISLPP_EXSITU_FUNCTION;

    //void addDisjoint_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_add_disjoint(  )); }

    void unionMin_inplace(const PwAff &pwaff2) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_union_min(take(), pwaff2.takeCopy())); }
    PwAff unionMin(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_min(takeCopy(), pwaff2.takeCopy())); }

    void unionMax_inplace(const PwAff &pwaff2) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_union_max(take(), pwaff2.takeCopy())); }
    PwAff unionMax(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_max(takeCopy(), pwaff2.takeCopy())); }

    void unionAdd_inplace(const PwAff &pwaff2) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_union_add(take(), pwaff2.takeCopy())); }
    PwAff unionAdd(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_add(takeCopy(), pwaff2.takeCopy())); }

    // FIXME: getDomain()?
    Set domain() const { return Set::enwrap(isl_pw_aff_domain(takeCopy())); }
    Set getDomain() const { return Set::enwrap(isl_pw_aff_domain(takeCopy())); }

    void mul_inplace(const PwAff &multiplier) ISLPP_INPLACE_FUNCTION{ give(isl_pw_aff_mul(take(), multiplier.takeCopy())); }
    PwAff mul(const PwAff &multiplier) const { return PwAff::enwrap(isl_pw_aff_mul(takeCopy(), multiplier.takeCopy())); }

    ISLPP_EXSITU_ATTRS Aff singletonAff() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS PwAff resetDimIds(isl_dim_type type) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.resetDimIds_inplace(type); return result; }
      ISLPP_INPLACE_ATTRS void resetDimIds_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{
      cast_inplace(getSpace().resetDimIds(type));
    }

    ISLPP_EXSITU_ATTRS PwAff cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ obj_give(cast(std::move(space))); }

    ISLPP_EXSITU_ATTRS PwAff castDomain(Space domainSpace) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.castDomain_inplace(domainSpace); return result; }
    ISLPP_INPLACE_ATTRS void castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION;

    void printExplicit(llvm::raw_ostream &os, int maxElts = 8) const;
    void dumpExplicit(int maxElts) const;
    void dumpExplicit() const; // In order do be callable without arguments from debugger
    std::string toStringExplicit(int maxElts = 8);

#ifndef NDEBUG
    std::string toString() const; // Just to be callable from debugger, inherited from isl::Obj otherwise
#endif

    ISLPP_EXSITU_ATTRS Map reverse() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS PwAff simplify() ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void simplify_inplace() ISLPP_INPLACE_FUNCTION{ give(simplify().take()); }


      /// Evaluate the complexity of the expression
      /// the complexity actually consists of two numbers: the number of pieces (disjunctions) and the number of conditions (conjunctions)
      /// Disjunction complexity is stored in the upper halfs, therefore conjunctions are only considered if the number of disjuctions are equal
      /// This is because most algorithms (intersections, isl_pw_multi_aff_from_multi_pw_aff, ...) are produce exponential number of pieces in the result
    ISLPP_PROJECTION_ATTRS uint64_t getComplexity() ISLPP_PROJECTION_FUNCTION;

    /// Evaluate the complexity of the expression
    /// OpComplexity is the estimated number of machine instructions required to evaluate the expression represented by this PwAff
    /// Possible optimizations are not considered
    /// Different pieces are chosen using select instructions (cmov), not branches
    ISLPP_PROJECTION_ATTRS uint64_t getOpComplexity() ISLPP_PROJECTION_FUNCTION;

    /// Assume that domain values that are not defined in any piece are never needed
    /// Undefined domain values might be added to other pieces, depending on what yields simpler expressions
    ISLPP_INPLACE_ATTRS void gistUndefined_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS PwAff gistUndefined() ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.gistUndefined_inplace(); return result; }
  }; // class PwAff


  static inline PwAff enwrap(__isl_take isl_pw_aff *obj) { return PwAff::enwrap(obj); }
  static inline PwAff enwrapCopy(__isl_keep isl_pw_aff *obj) { return PwAff::enwrapCopy(obj); }


  static inline bool plainIsEqual(const PwAff &pwaff1, const PwAff &pwaff2) { return checkBool(isl_pw_aff_plain_is_equal(pwaff1.keep(), pwaff2.keep())); }

  PwAff unionMin(PwAff pwaff1, PwAff pwaff2);
  PwAff unionMax(PwAff pwaff1, PwAff pwaff2);
  PwAff unionAdd(PwAff pwaff1, PwAff pwaff2);

  Set domain(PwAff &&pwaff);

  PwAff min(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff max(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff mul(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff div(PwAff &&pwaff1, PwAff &&pwaff2);

  static inline PwAff add(PwAff &&pwaff1, PwAff &&pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.take(), pwaff2.take())); }
  static inline PwAff add(PwAff &&pwaff1, const PwAff &pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.take(), pwaff2.takeCopy())); }
  static inline PwAff add(const PwAff &pwaff1, PwAff &&pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.takeCopy(), pwaff2.take())); }
  static inline PwAff add(const PwAff &pwaff1, const PwAff &pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.takeCopy(), pwaff2.takeCopy())); }
  PwAff add(PwAff &&pwaff1, int pwaff2);
  static inline PwAff add(const PwAff &lhs, int rhs) { return add(lhs.copy(), rhs); }
  static inline PwAff add(int lhs, PwAff && rhs) { return add(std::move(rhs), lhs); }
  static inline PwAff add(int lhs, const PwAff & rhs) { return add(rhs, lhs); }
  static inline PwAff operator+(PwAff &&pwaff1, PwAff &&pwaff2) { return add(std::move(pwaff1), std::move(pwaff2)); }
  static inline PwAff operator+(PwAff &&pwaff1, const PwAff &pwaff2) { return add(std::move(pwaff1), pwaff2); }
  static inline PwAff operator+(const PwAff &pwaff1, PwAff &&pwaff2) { return add(pwaff1, std::move(pwaff2)); }
  static inline PwAff operator+(const PwAff &pwaff1, const PwAff &pwaff2) { return add(pwaff1, pwaff2); }
  static inline PwAff operator+(PwAff &&lhs, int rhs) { return add(std::move(lhs), rhs); }
  static inline PwAff operator+(const PwAff &lhs, int rhs) { return add(lhs, rhs); }
  static inline PwAff operator+(int lhs, PwAff && rhs) { return add(lhs, std::move(rhs)); }
  static inline PwAff operator+(int lhs, const PwAff & rhs) { return add(lhs, rhs); }

  static inline PwAff &operator+=(PwAff &lhs, const PwAff &rhs) { lhs.give(isl_pw_aff_add(lhs.take(), rhs.takeCopy())); return lhs; }

  PwAff sub(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff sub(const PwAff &pwaff1, PwAff &&pwaff2);
  PwAff sub(PwAff &&pwaff1, const PwAff &pwaff2);
  PwAff sub(const PwAff &pwaff1, const PwAff &pwaff2);
  PwAff sub(PwAff &&lhs, int rhs);
  static inline PwAff sub(const PwAff &lhs, int rhs) { return sub(lhs, rhs); }
  static inline PwAff sub(int lhs, PwAff &&rhs){ return sub(lhs, std::move(rhs)); }
  static inline PwAff sub(int lhs, const PwAff & rhs) { return sub(lhs, rhs); }
  static inline PwAff operator-(PwAff &&pwaff1, PwAff &&pwaff2) { return sub(std::move(pwaff1), std::move(pwaff2)); }
  static inline PwAff operator-(PwAff &&pwaff1, const PwAff &pwaff2) { return sub(std::move(pwaff1), pwaff2); }
  static inline PwAff operator-(const PwAff &pwaff1, PwAff &&pwaff2) { return sub(pwaff1, std::move(pwaff2)); }
  static inline PwAff operator-(const PwAff &pwaff1, const PwAff &pwaff2) { return sub(pwaff1, pwaff2); }
  static inline PwAff operator-(PwAff &&lhs, int rhs) { return sub(std::move(lhs), rhs); }
  static inline PwAff operator-(const PwAff &lhs, int rhs) { return sub(lhs, rhs); }
  static inline PwAff operator-(int lhs, PwAff && rhs) { return sub(lhs, std::move(rhs)); }
  static inline PwAff operator-(int lhs, const PwAff & rhs) { return sub(lhs, rhs); }

  static inline PwAff operator-(const Aff &lhs, const PwAff &rhs) { return PwAff::enwrap(isl_pw_aff_sub(isl_pw_aff_from_aff(lhs.takeCopy()), rhs.takeCopy())); }

  static inline const PwAff operator*(const PwAff &lhs, const PwAff &rhs) { return PwAff::enwrap(isl_pw_aff_mul(lhs.takeCopy(), rhs.takeCopy())); }
  static inline const PwAff &operator*=(PwAff &lhs, const PwAff &rhs) { lhs.mul_inplace(rhs); return lhs; }
  static inline PwAff operator/(PwAff divident, Int divisor) {
    auto affdivisor = divident.getSpace().createConstantAff(divisor).toPwAff();
    return PwAff::enwrap(isl_pw_aff_div(divident.take(), affdivisor.take()));
  }


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

  static inline PwAff setTupleId(PwAff &&pwaff, isl_dim_type type, Id &&id) { return PwAff::enwrap(isl_pw_aff_set_tuple_id(pwaff.take(), type, id.take())); }
} // namespace isl
#endif /* ISLPP_PWAFF_H */
