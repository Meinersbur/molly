#ifndef ISLPP_AFF_H
#define ISLPP_AFF_H

#include "islpp_common.h"
#include "islpp/Islfwd.h"
#include "Obj.h"
#include "Spacelike.h"
#include "Ctx.h"
#include "Space.h"
#include "LocalSpace.h"
#include "Int.h"
#include "SetSpace.h"

#include <isl/space.h> // enum isl_dim_type;
#include <isl/aff.h>
#include <isl/deprecated/aff_int.h>

#include <cassert>
#include <string>

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


#if 0
extern "C" {
  // not public, but we require it for a more sensical defintition of equality for affs with different local space
  __isl_give isl_aff *isl_aff_align_divs(__isl_take isl_aff *dst, __isl_keep isl_aff *src);
} // extern "C"
#endif


namespace isl {

  /// methods between isl::Aff and isl::AffExpr
  template<typename AffTy>
  class AffCommon {
  private:
    AffTy &getDerived() { return *static_cast<AffTy*>(this); }
    const AffTy &getDerived() const { return *static_cast<const AffTy*>(this); }

#pragma region isl::Obj
    friend class isl::Obj<AffTy, isl_aff>;
  protected:
    void release() { isl_aff_free(getDerived().takeOrNull()); }
    isl_aff *addref() const { return isl_aff_copy(getDerived().keepOrNull()); }

  public:
    Ctx *getCtx() const { return Ctx::enwrap(isl_aff_get_ctx(getDerived().keep())); }
    //void dump() const { isl_aff_dump(getDerived().keep()); }
#pragma endregion

  public:
    Int getConstant() const {
      Int result;
      isl_aff_get_constant(getDerived().keep(), result.change());
      return result;
    }
  }; // AffCommon


  Aff div(Aff &&, const Int &);

  class Aff : public Obj<Aff, isl_aff>, public Spacelike<Aff>, public AffCommon<Aff> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  public:
    Aff() { }

    /* implicit */ Aff(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Aff(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_aff_get_space(keep())); }
    LocalSpace getLocalSpace() const { return LocalSpace::enwrap(isl_aff_get_local_space(keep())); }
    LocalSpace getSpacelike() const { return getLocalSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_FUNCTION{ give(isl_aff_set_tuple_id(take(), type, id.take())); }
      //void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_FUNCTION {
      //  assert(type==isl_dim_in);
      //  cast_inplace(getSpace().setTupleId(type, std::move(id)));
      //}
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_FUNCTION{ give(isl_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_FUNCTION{ give(isl_aff_insert_dims(take(), type, pos, count)); }
      //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION{ give(isl_aff_drop_dims(take(), type, first, count)); }

    bool isMap() const { return true; }
    bool isSet() const { return false; }

    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_aff_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_aff_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_aff_has_tuple_id(keep(), type); }
    //const char *getTupleName(isl_dim_type type) const { return isl_aff_get_tuple_name(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_aff_get_tuple_id(keep(), type)); }
    //void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_aff_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_aff_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_aff_get_dim_name(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_aff_get_dim_id(keep(), type, pos)); }
    //void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_aff_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_FUNCTION{ give(isl_aff_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Conversion
    ISLPP_EXSITU_ATTRS PwAff toPwAff() ISLPP_EXSITU_FUNCTION;
    ISLPP_CONSUME_ATTRS PwAff toPwAff_consume() ISLPP_CONSUME_FUNCTION;

    ISLPP_EXSITU_ATTRS MultiAff toMultiAff() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS PwMultiAff toPwMultiAff() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS BasicMap toBasicMap() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS Map toMap() ISLPP_EXSITU_FUNCTION;
#pragma endregion


#pragma region Creational
    static Aff createZeroOnDomain(LocalSpace &&space);
    static Aff createVarOnDomain(LocalSpace &&space, isl_dim_type type, unsigned pos);

    static Aff readFromString(Ctx *ctx, const char *str);
#pragma endregion


#pragma region Printing
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


    //Ctx *getCtx() const;
    //int dim(isl_dim_type type) const;
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const;
    Space getDomainSpace() const;
    //Space getSpace() const;
    LocalSpace getDomainLocalSpace() const;
    //LocalSpace getLocalSpace() const;
    ISLPP_EXSITU_ATTRS Space getRangeSpace() ISLPP_EXSITU_FUNCTION{
      auto space = getSpace();
      if (space.isMapSpace())
        return space.getRangeSpace();
      return space;
    }

      //const char *getDimName( isl_dim_type type, unsigned pos) const;

    Int getCoefficient(isl_dim_type type, unsigned pos) const;
    ISLPP_PROJECTION_ATTRS Int getCoefficient(Dim dim) ISLPP_PROJECTION_FUNCTION{
      Int result;
      isl_aff_get_coefficient(takeCopy(), dim.getType(), dim.getPos(), result.change());
      result.updated();
      return result; // NRVO 
    }
    Int getDenominator() const;

    Aff setConstant(const Int &v) const { return Aff::enwrap(isl_aff_set_constant(takeCopy(), v.keep())); }
    void setConstant_inplace(const Int &) ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void setCoefficient_inplace(isl_dim_type type, unsigned pos, int) ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void setCoefficient_inplace(isl_dim_type type, unsigned pos, const Int &) ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void setDenominator_inplace(const Int &) ISLPP_INPLACE_FUNCTION;

    ISLPP_INPLACE_ATTRS void addConstant_inplace(const Int &)ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void addConstant_inplace(int)ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void addConstantNum_inplace(const Int &)ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void addConstantNum_inplace(int)ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void addCoefficient_inplace(isl_dim_type type, unsigned pos, int)ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS  void addCoefficient_inplace(isl_dim_type type, unsigned pos, const Int &)ISLPP_INPLACE_FUNCTION;

    bool isCst() const;

    //void setDimName(isl_dim_type type, unsigned pos, const char *s);
    //void setDimId(isl_dim_type type, unsigned pos, Id &&id);

    bool isPlainZero() const;
    Aff getDiv(int pos) const;

    ISLPP_EXSITU_ATTRS Aff neg() ISLPP_EXSITU_FUNCTION{ return Aff::enwrap(isl_aff_neg(takeCopy())); }
    ISLPP_INPLACE_ATTRS void neg_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_aff_neg(take())); }
    void ceil();
    ISLPP_EXSITU_ATTRS Aff floor() ISLPP_EXSITU_FUNCTION{ return Aff::enwrap(isl_aff_floor(takeCopy())); }
    ISLPP_INPLACE_ATTRS void floor_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_aff_floor(take())); }
    void mod(const Int &mod);

    Aff divBy(const Aff &divisor) const { return Aff::enwrap(isl_aff_div(takeCopy(), divisor.takeCopy())); }
    Aff divBy(Aff &&divisor) const { return Aff::enwrap(isl_aff_div(takeCopy(), divisor.take())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIERS
    Aff divBy(const Aff &divisor) && { return wrap(isl_aff_div(take(), divisor.takeCopy()));  }
    Aff divBy(Aff &&divisor) && { return wrap(isl_aff_div(take(), divisor.take()));  }
#endif

    Aff divBy(const Int &divisor) const { return div(this->copy(), divisor); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIERS
    Aff divBy(const Int &divisor) && { return return div(this->move(), divisor); }
#endif

    void scale(const Int &f);
    void scaleDown(const Int &f);
    void scaleDown(unsigned f);

    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);

    void projectDomainOnParams();

    void alignParams(Space &&model);
    ISLPP_INPLACE_ATTRS void gist_inplace(Set context) ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff gist(Set context) ISLPP_EXSITU_FUNCTION;
    void gistParams(Set &&context);

    Aff pullback(const MultiAff &maff) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.pullback_inplace(maff); return result; }
    void pullback_inplace(const MultiAff &) ISLPP_INPLACE_FUNCTION;
    PwAff pullback(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION;

    /// Would be ineffective
    //PwAff pullback(const MultiPwAff &pma) ISLPP_EXSITU_QUALIFIER;

    ISLPP_EXSITU_ATTRS Aff cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ obj_give(this->cast(isl::move(space))); }

    ISLPP_EXSITU_ATTRS Aff castDomain(Space domainSpace) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.castDomain_inplace(domainSpace); return result; }
    ISLPP_INPLACE_ATTRS void castDomain_inplace(Space space) ISLPP_INPLACE_FUNCTION;

    ISLPP_EXSITU_ATTRS Aff removeDivsUsingFloor()ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.removeDivsUsingFloor_inplace(); return result; }
    ISLPP_INPLACE_ATTRS void removeDivsUsingFloor_inplace() ISLPP_INPLACE_FUNCTION;

    ISLPP_EXSITU_ATTRS Aff removeDivsUsingCeil()ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.removeDivsUsingCeil_inplace(); return result; }
    ISLPP_INPLACE_ATTRS void removeDivsUsingCeil_inplace() ISLPP_INPLACE_FUNCTION;

    ISLPP_INPLACE_ATTRS const Aff & operator+=(Aff arg) ISLPP_INPLACE_FUNCTION{ give(isl_aff_add(take(), arg.take())); return *this; }
    ISLPP_INPLACE_ATTRS const Aff & operator-=(Aff arg) ISLPP_INPLACE_FUNCTION{ give(isl_aff_sub(take(), arg.take())); return *this; }
    ISLPP_INPLACE_ATTRS const Aff & operator+=(const Int &arg) ISLPP_INPLACE_FUNCTION{ give(isl_aff_add_constant(take(), arg.keep())); return *this; }
    ISLPP_INPLACE_ATTRS const Aff & operator-=(const Int &arg) ISLPP_INPLACE_FUNCTION{ 
      Int &cheat = const_cast<Int&>(arg);//FIXME: Is this performance optimization worth it? arg may point to volatile/read-only memory
      cheat.neg_inplace();
      give(isl_aff_add_constant(take(), cheat.keep()));
      cheat.neg_inplace(); // undo negate
      return *this;
    }

    ISLPP_PROJECTION_ATTRS uint32_t getComplexity() ISLPP_PROJECTION_FUNCTION;
    ISLPP_PROJECTION_ATTRS uint32_t getOpComplexity() ISLPP_PROJECTION_FUNCTION;

    /// Simplify this aff using the fact that this affine is never applied on inputs outside of this set
    /// The result value outside of the domain may change
    /// FIXME: Functionality of isl_aff_gist, is gist always better?
    ISLPP_INPLACE_ATTRS void normalize_inplace(const BasicSet &domain) ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff normalize(const BasicSet &domain)ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.normalize_inplace(domain); return result; }


    ISLPP_PROJECTION_ATTRS Int getDivCoefficient(const Aff &aff) ISLPP_PROJECTION_FUNCTION;

    ISLPP_INPLACE_ATTRS void normalizeDivs_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff normalizeDivs() ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.normalizeDivs_inplace(); return result; }

    ISLPP_EXSITU_ATTRS Aff operator-() ISLPP_EXSITU_FUNCTION{ return Aff::enwrap(isl_aff_neg(takeCopy())); }

      // TODO: May use isl_aff_add() with zero aff
    ISLPP_INPLACE_ATTRS void alignDivs_inplace(const Aff &that) ISLPP_INPLACE_FUNCTION{
      alignDivs_inplace(that.getLocalSpace());
      //give(isl_aff_align_divs(take(), that.take())); 
    }
    ISLPP_EXSITU_ATTRS Aff alignDivs(const Aff &that)ISLPP_EXSITU_FUNCTION{ 
      return alignDivs(that.getLocalSpace());
      //return Aff::enwrap(isl_aff_align_divs(takeCopy(), that.take()));
    }

      ISLPP_INPLACE_ATTRS void alignDivs_inplace(LocalSpace ls) ISLPP_INPLACE_FUNCTION{
      auto tmp = ls.createZeroAff();
      give(isl_aff_add(take(), tmp.take()));
    }
      ISLPP_EXSITU_ATTRS Aff alignDivs(LocalSpace ls)ISLPP_EXSITU_FUNCTION{
      auto tmp = ls.createZeroAff();
      return Aff::enwrap(isl_aff_add(takeCopy(), tmp.take()));
    }

    ISLPP_PROJECTION_ATTRS bool isLinearOnRange(Dim dim, const Int *lowerBound, const Int *upperBound, /*out*/Int &slope)ISLPP_PROJECTION_FUNCTION;
  }; // class Aff


  static inline Aff enwrap(__isl_take isl_aff *obj) { return Aff::enwrap(obj); }
  static inline Aff enwrapCopy(__isl_keep isl_aff *obj) { return Aff::enwrapCopy(obj); }



  static inline bool isPlainEqual(const Aff &aff1, const Aff &aff2)  { return isl_aff_plain_is_equal(aff1.keep(), aff2.keep()); }
  static inline Aff mul(Aff &&aff1, Aff &&aff2) { return enwrap(isl_aff_mul(aff1.take(), aff2.take())); }
  static inline Aff div(Aff aff1, Aff aff2) { return enwrap(isl_aff_div(aff1.take(), aff2.take())); }
  static inline Aff div(Aff aff1, int divisor) { return div(std::move(aff1), aff1.getDomainSpace().createConstantAff(Int(divisor))); }
  static inline Aff add(Aff aff1, Aff aff2) { return enwrap(isl_aff_add(aff1.take(), aff2.take())); }
  static inline Aff sub(Aff aff1, Aff aff2) { return enwrap(isl_aff_sub(aff1.take(), aff2.take())); }

  static inline Aff floor(Aff aff) { return Aff::enwrap(isl_aff_floor(aff.take())); }

  BasicSet zeroBasicSet(Aff &&aff);
  BasicSet negBasicSet(Aff &&aff);
  BasicSet leBasicSet(Aff &aff1, Aff &aff2);
  BasicSet geBasicSet(Aff &aff1, Aff &aff2);

  static inline Aff operator+(Aff lhs, Aff rhs) { return Aff::enwrap(isl_aff_add(lhs.take(), rhs.take())); }
  static inline Aff operator+(Aff aff, int v) { auto ls = aff.getDomainLocalSpace(); return add(aff.move(), ls.createConstantAff(v)); }
  static inline Aff operator+(const Int &lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return add(ls.createConstantAff(lhs), std::move(rhs)); }
  static inline Aff operator+(int lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return add(ls.createConstantAff(lhs), std::move(rhs)); }
  static inline Aff operator-(Aff lhs, Aff rhs) { return Aff::enwrap(isl_aff_sub(lhs.take(), rhs.take())); }
  static inline Aff operator-(Aff aff, int v) { auto ls = aff.getDomainLocalSpace(); return sub(aff.move(), ls.createConstantAff(v)); }
  static inline Aff operator-(Aff aff, const Int &v) { auto ls = aff.getDomainLocalSpace(); return sub(std::move(aff), ls.createConstantAff(v)); }
  static inline Aff operator-(const Int &lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return sub(ls.createConstantAff(lhs), std::move(rhs)); }
  static inline Aff operator-(int lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return sub(ls.createConstantAff(lhs), std::move(rhs)); }

  static inline Aff operator*(Aff lhs, Aff rhs) { return Aff::enwrap(isl_aff_mul(lhs.take(), rhs.take())); }
  static inline Aff operator*(Aff lhs, signed long v) { auto ls = lhs.getDomainLocalSpace(); return mul(lhs.move(), ls.createConstantAff(v)); }
  static inline Aff operator*(signed long lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return mul(std::move(rhs), ls.createConstantAff(lhs)); }
  static inline Aff operator*(const Int &lhs, Aff rhs) { auto ls = rhs.getDomainLocalSpace(); return mul(std::move(rhs), ls.createConstantAff(lhs)); }
  static inline Aff operator*(Aff lhs, const Int &rhs) { auto ls = lhs.getDomainLocalSpace(); return mul(std::move(lhs), ls.createConstantAff(rhs)); }


  static inline Aff operator/(Aff lhs, Aff rhs) { return div(std::move(lhs), std::move(rhs)); }
  static inline Aff operator/(Aff lhs, signed long v) { return div(lhs.move(), v); }

  static inline Aff floord(Aff divident, const Int &divisor) {
    auto ls = divident.getDomainLocalSpace();
    auto denom = ls.createConstantAff(divisor);
    return Aff::enwrap(isl_aff_floor(isl_aff_div(divident.take(), denom.take())));
  }

  bool isEqual(const Aff &lhs, const Aff &rhs);
  Tribool plainIsEqual(const Aff &lhs, const Aff &rhs);
  Tribool isEqual(const Aff &lhs, const Aff &rhs, Accuracy accuracy);

  static inline bool operator==(const Aff &lhs, const Aff &rhs) {
    return isEqual(lhs,rhs);
  }
  static inline bool operator!=(const Aff &lhs, const Aff &rhs) {
    return !operator==(lhs,rhs);
  }

  bool tryCombineAff(Set lhsContext, Aff lhsAff, const Set &rhsContext, const Aff &rhsAff, bool tryCoeffs, bool tryDivs, Set &resultContext, Aff &resultAff);

} // namespace isl
#endif /* ISLPP_AFF_H */
