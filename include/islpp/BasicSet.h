#ifndef ISLPP_BASICSET_H
#define ISLPP_BASICSET_H

#include "islpp_common.h"
#include "Spacelike.h" // class Obj<,>
#include "Ctx.h"
#include "Id.h"
#include "Space.h"
#include "LocalSpace.h"
#include "Constraint.h"
#include "Islfwd.h"
#include "SetSpacelike.h"
#include "SetSpace.h"

#include <isl/space.h> // enum isl_dim_type;
#include <isl/set.h>
#include <isl/deprecated/map_int.h>
#include <isl/deprecated/set_int.h>

#include <llvm/Support/ErrorHandling.h>
#include <llvm/ADT/SmallBitVector.h>
#include <llvm/ADT/SmallVector.h>

#include <string>
#include <functional>

struct isl_basic_set;
struct isl_constraint;

namespace llvm {
  class raw_ostream;
  class SmallBitVector;
} // namespace llvm


namespace isl {
  typedef int(*ConstraintCallback)(isl_constraint *c, void *user);


  // Thanks to Lord Odin (http://stackoverflow.com/questions/12059774/c11-standard-conformant-bitmasks-using-enum-class#answer-17771358)
#define ENUM_FLAGS_EX_NO_FLAGS_FUNC(T,INT_T) \
  enum class T;	\
  static inline T	operator	&	(T x, T y)		{	return static_cast<T>	(static_cast<INT_T>(x) & static_cast<INT_T>(y));	}; \
  static inline T	operator	|	(T x, T y)		{	return static_cast<T>	(static_cast<INT_T>(x) | static_cast<INT_T>(y));	}; \
  static inline T	operator	^	(T x, T y)		{	return static_cast<T>	(static_cast<INT_T>(x) ^ static_cast<INT_T>(y));	}; \
  static inline T	operator	~	(T x)			{	return static_cast<T>	(~static_cast<INT_T>(x));							}; \
  static inline T&	operator	&=	(T& x, T y)		{	x = x & y;	return x;	}; \
  static inline T&	operator	|=	(T& x, T y)		{	x = x | y;	return x;	}; \
  static inline T&	operator	^=	(T& x, T y)		{	x = x ^ y;	return x;	};
#define ENUM_FLAGS_EX(T,INT_T) \
  ENUM_FLAGS_EX_NO_FLAGS_FUNC(T,INT_T) \
  static inline bool			flags(T x)			{	return static_cast<INT_T>(x) != 0;} \
  static inline bool hasFlag(T x, T flag) { return (static_cast<INT_T>(x) & static_cast<INT_T>(flag)) == static_cast<INT_T>(flag); }
#define ENUM_FLAGS(T) ENUM_FLAGS_EX(T,intptr_t)

  ENUM_FLAGS(HasBounds)
  enum class HasBounds{
    Unbounded = 0,
    LowerBound = 1,
    UpperBound = 2,
    Bounded = LowerBound | UpperBound,
    Empty = 8,
    Fixed = LowerBound | UpperBound | 4,
    Impossible = LowerBound | UpperBound | Empty,
  };


  class Interval {
  public:
    bool hasLower;
    Int lower;
    bool hasUpper;
    Int upper;

  protected:
    Interval(const Int &lower, const Int &upper) : hasLower(true), lower(lower), hasUpper(true), upper(upper) { assert (lower<=upper); }
    Interval(bool hasLower, const Int &upper) : hasLower(hasLower), hasUpper(true), upper(upper) {}
    Interval(const Int &lower, bool hasUpper) : hasLower(true), lower(lower), hasUpper(hasUpper) {}
    Interval(bool hasLower, bool hasUpper) : hasLower(hasLower), hasUpper(hasUpper) {}

  public:
    Interval() : hasLower(false), hasUpper(false) {}

   static Interval createBounded(const Int &lower, const Int &upper) { return Interval(lower, upper); }
   static Interval createUpper(const Int &upper) { return Interval(false, upper); }
   static Interval createLower(const Int &lower){ return Interval(lower, false); }

   bool isBounded() const { return hasLower && hasUpper; }
  }; // class Interval


  static inline bool unitable(const Interval &lhs, const Interval &rhs) {
    if (lhs.hasUpper && rhs.hasLower && (lhs.upper - rhs.lower > 1))
      return false;
    if (lhs.hasLower && rhs.hasUpper && (rhs.upper - lhs.upper > 1))
      return false;
    return true;
  }

  static inline Interval unite(const Interval &lhs, const Interval &rhs) {
    Interval result;

    if (lhs.hasLower && rhs.hasLower) {
      result.hasLower = true;
      result.lower = min(lhs.lower, rhs.lower);
    } else if (lhs.hasLower) {
      result.hasLower = true;
      result.lower = lhs.lower;
    } else if (rhs.hasLower) {
      result.hasLower = true;
      result.lower= rhs.lower;
    } else 
      result.hasLower = false;

    if (lhs.hasUpper && rhs.hasUpper) {
      result.hasUpper = true;
      result.upper = min(lhs.upper, rhs.upper);
    } else if (lhs.hasUpper) {
      result.hasUpper = true;
      result.upper = lhs.upper;
    } else if (rhs.hasUpper) {
      result.hasUpper = true;
      result.upper = rhs.upper;
    } else
      result.hasUpper = false;

    return result;
  }


  class AllBounds {
    friend class BasicSet;
    friend class Set;

    LocalSpace space;
    llvm::SmallBitVector hasBound;
    std::vector<Int> bounds;

  protected:
    pos_t offset(isl_dim_type type, pos_t pos) const {
      assert(type == isl_dim_param || type == isl_dim_set || type == isl_dim_div);
      assert(space.isValidDim(type, pos));
      pos_t result = pos;
      switch (type)      {
      case isl_dim_div:
        result += space.getSetDimCount();
        // fallthrough
      case isl_dim_set:
        result += space.getParamDimCount();
        // fallthrough
      case isl_dim_param:
        return result;
      default:
        llvm_unreachable("Illegal dimension type");
      }
    }
    void offsetToTypePos(size_t offset, isl_dim_type &type, pos_t &pos) {
      assert(offset < getAllDimCount());

      auto nParamDims = getParamDimsCount();
      if (offset < nParamDims) {
        type = isl_dim_param;
        pos = offset;
        return;
      }
      offset -= nParamDims;

      auto nSetDims = getSetDimCount();
      if (offset < nSetDims) {
        type = isl_dim_set;
        pos = offset;
        return;
      }
      offset -= nSetDims;

      type = isl_dim_div;
      pos = offset;
      assert(pos < getDivDimCount());
    }
    pos_t lowerOffset(size_t offset) const {
      return 2 * offset;
    }
    pos_t lowerOffset(isl_dim_type type, pos_t pos) const {
      return 2 * offset(type, pos);
    }
    pos_t upperOffset(size_t offset) const {
      return 2 * offset + 1;
    }
    pos_t upperOffset(isl_dim_type type, pos_t pos) const {
      return 2 * offset(type, pos) + 1;
    }

    bool updateEqBounds(const Mat &eqs, bool areIneqs, int i);
    bool updateDivBounds(const Aff &div, pos_t d);
    void searchAllBounds(const BasicSet &bset);

    bool isImpossible(size_t offset) const {
      return isBounded(offset) && (getLowerBound(offset) > getUpperBound(offset));
    }

    bool hasLowerBound(size_t offset) const {
      return hasBound[lowerOffset(offset)];
    }
    bool hasUpperBound(size_t offset) const {
      return hasBound[upperOffset(offset)];
    }
    bool isBounded(size_t offset) const {
      return hasLowerBound(offset) && hasUpperBound(offset);
    }

    const Int &getLowerBound(size_t offset) const {
      assert(hasLowerBound(offset));
      return bounds[lowerOffset(offset)];
    }
    Int &getLowerBound(size_t offset) {
      assert(hasLowerBound(offset));
      return bounds[lowerOffset(offset)];
    }
    const Int &getLowerBound(size_t offset, const Int &def) const {
      if (!hasLowerBound(offset))
        return def;
      return bounds[lowerOffset(offset)];
    }
    const Int &getUpperBound(size_t offset) const {
      assert(hasUpperBound(offset));
      return bounds[upperOffset(offset)];
    }
    Int &getUpperBound(size_t offset)  {
      assert(hasUpperBound(offset));
      return bounds[upperOffset(offset)];
    }
    const Int &getUpperBound(size_t offset, const Int &def) const {
      if (!hasUpperBound(offset))
        return def;
      return bounds[upperOffset(offset)];
    }

    void setLowerBound(size_t offset, const Int &val) {
      auto o = lowerOffset(offset);
      hasBound.set(o);
      bounds[o] = val;
    }
    void resetLowerBound(size_t offset) {
      auto o = lowerOffset(offset);
      hasBound.reset(o);
    }
    void setUpperBound(size_t offset, const Int &val) {
      auto o = upperOffset(offset);
      hasBound.set(o);
      bounds[o] = val;
    }
    void resetUpperBound(size_t offset) {
      auto o = upperOffset(offset);
      hasBound.reset(o);
    }

    bool updateConjunctiveLowerBound(size_t offset, const Int &val) {
      if (isImpossible(offset))
        return false;
      if (!hasLowerBound(offset) || (getLowerBound(offset) < val)) {
        setLowerBound(offset, val);
        return true;
      }
      return false;
    }
    bool updateDisjunktiveLowerBound(size_t offset, const Int &val) {
      if (!hasLowerBound(offset) || (getLowerBound(offset) <= val))
        return false;
      setLowerBound(offset, val);
      return false;
    }
    bool updateConjunctiveUpperBound(size_t offset, const Int &val) {
      if (isImpossible(offset))
        return false;
      if (!hasUpperBound(offset) || (getUpperBound(offset) > val)) {
        setUpperBound(offset, val);
        return true;
      }
      return false;
    }
    bool updateDisjunctiveUpperBound(size_t offset, const Int &val) {
      if (!hasUpperBound(offset) || (getUpperBound(offset) >= val))
        return false;
      setUpperBound(offset, val);
      return true;
    }

    Interval getInterval(size_t offset) const {
    Interval result;
    result.hasLower = hasLowerBound(offset);
    if (result.hasLower)
      result.lower = getLowerBound(offset);
    result.hasUpper = hasUpperBound(offset);
      if (result.hasUpper)
        result.upper = getUpperBound(offset);
    return result;
    }

    void disjunctiveMerge(const AllBounds &that);

    AllBounds() {}

    void init(LocalSpace space)  {
      assert(space.isSet());
      this->space = std::move(space);
      auto nParamDims = this->space.getParamDimCount();
      auto nInDims = this->space.getSetDimCount();
      auto nDivDims = this->space.getDivDimCount();
      hasBound.resize(2 * (nParamDims + nInDims + nDivDims));
      bounds.resize(2 * (nParamDims + nInDims + nDivDims));
    }

    void init(SetSpace space) {
      this->space = std::move(space);
      auto nParamDims = this->space.getParamDimCount();
      auto nInDims = this->space.getSetDimCount();
      hasBound.resize(2 * (nParamDims + nInDims));
      bounds.resize(2 * (nParamDims + nInDims));
    }

  public:
    void dump() const;

    /// impossible necessarily means empty set, but not the other way around
    bool isImpossible() const;
    bool isImpossible(isl_dim_type type, pos_t pos) const {
      return isImpossible(offset(type, pos));
    }

    void scrapDivDims() {
      if (!getDivDimCount())
        return;

      space = space.getSpace();
      auto nParamDims = space.getParamDimCount();
      auto nInDims = space.getSetDimCount();
      hasBound.resize(2 * (nParamDims + nInDims));
      bounds.resize(2 * (nParamDims + nInDims));
    }

    bool hasLowerBound(isl_dim_type type, pos_t pos) const {
      return hasBound[lowerOffset(type, pos)];
    }
    bool hasUpperBound(isl_dim_type type, pos_t pos) const {
      return hasBound[upperOffset(type, pos)];
    }
    bool isBounded(isl_dim_type type, pos_t pos) const {
      return hasLowerBound(type, pos) && hasUpperBound(type, pos);
    }
    bool isFixed(isl_dim_type type, pos_t pos) const {
      return isBounded(type, pos) && getLowerBound(type, pos) == getUpperBound(type, pos);
    }
    bool isFixed(isl_dim_type type, pos_t pos, Int &val) const {
      if (!isBounded(type, pos))
        return false;

      val = getLowerBound(type, pos);
      return val == getUpperBound(type, pos);
    }
    const Int &getLowerBound(isl_dim_type type, pos_t pos) const {
      assert(hasLowerBound(type, pos));
      return bounds[lowerOffset(type, pos)];
    }
    const Int &getLowerBound(isl_dim_type type, pos_t pos, const Int &def) const {
      if (!hasLowerBound(type, pos))
        return def;
      return bounds[lowerOffset(type, pos)];
    }
    Int &getLowerBound(isl_dim_type type, pos_t pos)  {
      assert(hasLowerBound(type, pos));
      return bounds[lowerOffset(type, pos)];
    }
    const Int &getUpperBound(isl_dim_type type, pos_t pos) const {
      assert(hasUpperBound(type, pos));
      return bounds[upperOffset(type, pos)];
    }
    const Int &getUpperBound(isl_dim_type type, pos_t pos, const Int &def) const {
      if (!hasUpperBound(type, pos))
        return def;
      return bounds[upperOffset(type, pos)];
    }
    Int &getUpperBound(isl_dim_type type, pos_t pos)  {
      assert(hasUpperBound(type, pos));
      return bounds[upperOffset(type, pos)];
    }
    Int &getFixed(isl_dim_type type, pos_t pos) {
      assert(isFixed(type, pos));
      return getLowerBound(type, pos);
    }
    const Int &getFixed(isl_dim_type type, pos_t pos) const {
      assert(isFixed(type, pos));
      return getLowerBound(type, pos);
    }

    void setLowerBound(isl_dim_type type, pos_t pos, const Int &val) {
      auto o = lowerOffset(type, pos);
      hasBound.set(o);
      bounds[o] = val;
    }
    void setUpperBound(isl_dim_type type, pos_t pos, const Int &val) {
      auto o = upperOffset(type, pos);
      hasBound.set(o);
      bounds[o] = val;
    }

    Interval getInterval(isl_dim_type type, pos_t pos) const {
      auto o = upperOffset(type, pos);
      return getInterval(o);
    }

    count_t getAllDimCount() const { return bounds.size() / 2; }
    count_t getParamDimsCount() const { return space.getParamDimCount(); }
    count_t getSetDimCount() const { return space.getSetDimCount(); }
    count_t getDivDimCount() const { return space.getDivDimCount(); }
    Space getSpace() const { return space.getSpace(); }
    LocalSpace getLocalSpace() const { return space; }

  }; // class AllBounds


  class BasicSet : public Obj<BasicSet, isl_basic_set>, public SetSpacelike<BasicSet>, public Spacelike<BasicSet> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_basic_set_free(takeOrNull()); }
    StructTy *addref() const { return isl_basic_set_copy(keepOrNull()); }

  public:
    BasicSet() { }
    static ObjTy wrap(StructTy *obj) { return BasicSet::enwrap(obj); }// obsolete

    /* implicit */ BasicSet(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ BasicSet(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }


    Ctx *getCtx() const { return Ctx::enwrap(isl_basic_set_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS SetSpace getSpace() ISLPP_PROJECTION_FUNCTION{ return SetSpace::enwrap(isl_basic_set_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION{ return getLocalSpace(); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return true; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return false; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_basic_set_dim(keep(), type); }
      //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_basic_set_find_dim_by_id(keep(), type, id.keep()); }

      //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ assert(type == isl_dim_set); return isl_basic_set_get_tuple_name(keep()); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION{ assert(type == isl_dim_set); give(isl_basic_set_set_tuple_name(take(), s)); }
      //ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_tuple_id(keep(), type)); }
      //ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_basic_set_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ assert(type == isl_dim_set); give(isl_basic_set_set_tuple_id(take(), id.take())); }
      //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_reset_tuple_id(take(), type)); }

      //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_basic_set_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_set_dim_name(take(), type, pos, s)); }
      //ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_set_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_basic_set_get_dim_id(keep(), type, pos)); }
     // ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_basic_set_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_remove_dims(take(), type, first, count)); }
#pragma endregion


    ISLPP_PROJECTION_ATTRS LocalSpace getLocalSpace() ISLPP_PROJECTION_FUNCTION{ return LocalSpace::enwrap(isl_basic_set_get_local_space(keep())); }

    unsigned getDimCount() const { return isl_basic_set_dim(keep(), isl_dim_set); }
    bool hasDimId(unsigned pos) const { return Spacelike<BasicSet>::hasDimId(isl_dim_set, pos); }
    Id getDimId(unsigned pos) const { return Id::enwrap(isl_basic_set_get_dim_id(keep(), isl_dim_set, pos)); }

    isl::Id getTupleId() const { return Spacelike<BasicSet>::getTupleId(isl_dim_set); }
    bool hasTupleId() const { return Spacelike<BasicSet>::hasTupleId(isl_dim_set); }
    BasicSet setTupleId(const Id &id) const  { return Spacelike<BasicSet>::setTupleId(isl_dim_set, id); }
    ISLPP_INPLACE_ATTRS void setTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ Spacelike<BasicSet>::setSetTupleId_inplace(std::move(id)); }

#pragma region Creational
    static BasicSet create(Ctx *ctx, count_t nparam, count_t dim, count_t extra, count_t n_eq, count_t n_ineq) { return BasicSet::wrap(isl_basic_set_alloc(ctx->keep(), nparam, dim, extra, n_eq, n_ineq)); }
    static BasicSet createEmpty(Space space) { return BasicSet::wrap(isl_basic_set_empty(space.take())); }
    static BasicSet createUniverse(Space space) { return BasicSet::wrap(isl_basic_set_universe(space.take())); }
    static BasicSet createNatUniverse(Space &&space);
    static BasicSet createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4);
    static BasicSet createFromPoint(Point &&pnt);
    static BasicSet createBoxFromPoints(Point &&pnt1, Point &&pnt2);
    static BasicSet createFromBasicMap(BasicMap &&bmap);

    static BasicSet readFromFile(Ctx *ctx, FILE *input);
    static BasicSet readFromStr(Ctx *ctx, const char *str);
#pragma endregion


#pragma region Conversion
    Set toSet() const;
#pragma endregion


#pragma region Constraints
    ISLPP_EXSITU_ATTRS  BasicSet addConstraint(Constraint constraint) ISLPP_EXSITU_FUNCTION{ return BasicSet::enwrap(isl_basic_set_add_constraint(takeCopy(), constraint.take())); }
    ISLPP_INPLACE_ATTRS  void addConstraint_inplace(Constraint constraint) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_add_constraint(take(), constraint.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    BasicSet addConstraint(Constraint constraint) && { return BasicSet::enwrap(isl_basic_set_add_constraint(take(), constraint.take())); }
#endif

    void dropContraint(Constraint &&constraint);

    int getCountConstraints() const;
    bool foreachConstraint(ConstraintCallback fn, void *user) const;
    bool foreachConstraint(std::function<bool(Constraint)>) const;

    Mat equalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const;
    ISLPP_EXSITU_ATTRS  Mat equalitiesMatrix() ISLPP_EXSITU_FUNCTION;
    Mat inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const;
    ISLPP_EXSITU_ATTRS  Mat inequalitiesMatrix() ISLPP_EXSITU_FUNCTION;

    pos_t matrixOffset(isl_dim_type type, pos_t pos, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const {
      assert(0 <= pos && pos < dim(type));
      pos_t result = pos;
      if (type == c1)
        return result;
      result += dim(c1);
      if (type == c2)
        return result;
      result += dim(c2);
      if (type == c3)
        return result;
      result += dim(c3);
      return result;
    }
    pos_t matrixOffset(isl_dim_type type, pos_t pos) const {
      return matrixOffset(type, pos, isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div);
    }
#pragma endregion

    ISLPP_EXSITU_ATTRS BasicSet projectOut(isl_dim_type type, pos_t first, count_t n) ISLPP_EXSITU_FUNCTION{ return BasicSet::enwrap(isl_basic_set_project_out(takeCopy(), type, first, n)); }
    ISLPP_INPLACE_ATTRS void projectOut_inplace(isl_dim_type type, pos_t first, count_t n) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_project_out(take(), type, first, n)); }
    ISLPP_CONSUME_ATTRS BasicSet projectOut_consume(isl_dim_type type, pos_t first, count_t n) ISLPP_CONSUME_FUNCTION{ return BasicSet::enwrap(isl_basic_set_project_out(take(), type, first, n)); }

    void params();
    /// Eliminate the coefficients for the given dimensions from the constraints, without removing the dimensions.
    void eliminate(isl_dim_type type, unsigned first, unsigned n);

    void fix_inplace(isl_dim_type type, unsigned pos, const Int &value) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_fix(take(), type, pos, value.keep())); }
    BasicSet fix(isl_dim_type type, unsigned pos, const Int &value) const { return  BasicSet::enwrap(isl_basic_set_fix(takeCopy(), type, pos, value.keep())); }
    void fix_inplace(isl_dim_type type, unsigned pos, int value) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_fix_si(take(), type, pos, value)); }
    BasicSet fix(isl_dim_type type, unsigned pos, int value) const { return BasicSet::enwrap(isl_basic_set_fix_si(takeCopy(), type, pos, value)); }

    ISLPP_INPLACE_ATTRS void detectEqualities_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS  void removeRedundancies_inplace()ISLPP_INPLACE_FUNCTION;
    ISLPP_INPLACE_ATTRS void affineHull_inplace()ISLPP_INPLACE_FUNCTION;

    //Space getSpace() const;
    //LocalSpace getLocalSpace() const;
    void removeDivs();
    void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n);
    void removeUnknownDivs();

    /// The number of parameters, input, output or set dimensions can be obtained using the following functions.
    //    unsigned dim(isl_dim_type type) const;
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const;

    const char *getTupleName() const;
    void setTupleName(const char *s);

    //    Id getDimId(isl_dim_type type, unsigned pos) const;
    //   const char *getDimName(isl_dim_type type, unsigned pos) const;

    bool plainIsEmpty() const;
    bool isEmpty() const;
    bool isUniverse() const;
    bool isWrapping() const;

    void dropConstraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n);
    void dropConstraintsNotInvolvingDims(isl_dim_type type, unsigned first, unsigned n);

    /// If the input (basic) set or relation is non-empty, then return a singleton subset of the input. Otherwise, return an empty set.
    void sample();
    void coefficients();
    void solutions();
    void flatten();
    void lift();
    void alignParams(Space &&model);

    //void addDims(isl_dim_type type, unsigned n);
    //void insertDims(isl_dim_type type, unsigned pos, unsigned n);
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos,  unsigned n);

    void apply_inplace(BasicMap &&bmap) ISLPP_INPLACE_FUNCTION;

    BasicSet apply(const BasicMap &bmap) const;
    Set apply(const Map &map) const;

    //void preimage(MultiAff &&ma);

    ISLPP_EXSITU_ATTRS BasicSet preimage(MultiAff maff) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void preimage_inplace(MultiAff maff) ISLPP_INPLACE_FUNCTION;
    ISLPP_CONSUME_ATTRS BasicSet preimage_consume(MultiAff maff) ISLPP_CONSUME_FUNCTION;

    void gist(BasicSet &&context);
    Vertices computeVertices() const;

    void equate_inplace(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) ISLPP_INPLACE_FUNCTION{
      auto result = take();
      auto c = isl_equality_alloc(isl_basic_set_get_local_space(result));
      c = isl_constraint_set_coefficient_si(c, type1, pos1, 1);
      c = isl_constraint_set_coefficient_si(c, type2, pos2, -1);
      result = isl_basic_set_add_constraint(result, c);
      give(result);
    }
    BasicSet equate(isl_dim_type type1, unsigned pos1, isl_dim_type type2, unsigned pos2) const { auto result = copy(); result.equate_inplace(type1, pos1, type2, pos2); return result; }
    void equate_inplace(Dim dim1, Dim dim2) ISLPP_INPLACE_FUNCTION{ equate_inplace(dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos()); }
    BasicSet equate(Dim dim1, Dim dim2) const { return equate(dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos()); }

    void alignParams_inplace(Space &&model) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_align_params(take(), model.take())); }
    void alignParams_inplace(const Space &model) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_align_params(take(), model.takeCopy())); }
    BasicSet alignParams(Space &&model) const { return BasicSet::enwrap(isl_basic_set_align_params(takeCopy(), model.take())); }
    BasicSet alignParams(const Space &model) const { return BasicSet::enwrap(isl_basic_set_align_params(takeCopy(), model.takeCopy())); }

    /// Returns a somewhat strange aff without a domain (i.e. getSpace() returns a set space)
    ISLPP_EXSITU_ATTRS Aff dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS BasicSet cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ obj_give(cast(std::move(space))); }

      ISLPP_PROJECTION_ATTRS bool isFixed(isl_dim_type type, unsigned pos, Int &val) ISLPP_PROJECTION_FUNCTION{
      // There is no set variant; use the fact that in the implementation, a isl_basic_set is a isl_basic_map in disguise
      auto result = isl_basic_map_plain_is_fixed((isl_basic_map*)keep(), type, pos, val.change());
      val.updated();
      return checkBool(result);
    }

    ISLPP_PROJECTION_ATTRS uint32_t getComplexity() ISLPP_PROJECTION_FUNCTION;
    ISLPP_PROJECTION_ATTRS uint32_t getOpComplexity() ISLPP_PROJECTION_FUNCTION;

    bool hasLowerBound(isl_dim_type type, pos_t pos, Int &lowerBound) const;
    bool hasUpperBound(isl_dim_type type, pos_t pos, Int &upperBound) const;

    /// PLAIN algorithm (i.e. some bounds may not be found)
    HasBounds getDimBounds(isl_dim_type type, pos_t pos, Int &lowerBound, Int &upperBound);

    ISLPP_PROJECTION_ATTRS Aff getDiv(pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Aff::enwrap(isl_basic_set_get_div(keep(), pos)); }

    AllBounds getAllBounds() const;

    ISLPP_INPLACE_ATTRS void intersect_inplace(BasicSet that) ISLPP_INPLACE_FUNCTION{ give(isl_basic_set_intersect(take(), that.take())); }

  }; // class BasicSet



  static inline BasicSet enwrap(isl_basic_set *bset) { return BasicSet::wrap(bset); }

  BasicSet params(BasicSet &&params);

  bool isSubset(const BasicSet &bset1, const BasicSet &bset2);

  BasicMap unwrap(BasicSet &&bset);

  BasicSet intersectParams(BasicSet &&bset1, BasicSet &&bset2);
  BasicSet intersect(BasicSet &&bset1, BasicSet &&bset2);
  Set unite(BasicSet &&bset1, BasicSet &&bset2);
  BasicSet flatProduct(BasicSet &&bset1, BasicSet &&bset2);

  Set partialLexmin(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  Set partialLexmax(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  PwMultiAff partialLexminPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);
  PwMultiAff partialLexmaxPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty);

  Set lexmin(BasicSet &&bset);
  Set lexmax(BasicSet &&bset);

  Point samplePoint(BasicSet &&bset);
} // namespace isl
#endif /* ISLPP_BASICSET_H */
