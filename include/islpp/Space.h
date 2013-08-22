#ifndef ISLPP_SPACE_H
#define ISLPP_SPACE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include <assert.h>
#include <isl/space.h> // enum isl_dim_type;

#include "Ctx.h"
#include "Multi.h"
#include "Id.h"
#include "Expr.h"
#include "Obj.h"
#include "Spacelike.h"

struct isl_space;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class Id;
  class Set;
  class BasicSet;
  class Map;
  class BasicMap;
  class Int;
  class Point;
  class Constraint;
  class AstBuild;
} // namespace isl

extern "C" {
  // Forgotten declaration is <isl/space.h>
  __isl_give isl_space *isl_space_reset_dim_id(__isl_take isl_space *space, enum isl_dim_type type, unsigned pos);
} // extern "C"


namespace isl {
  /// Whenever a new set, relation or similar object is created from scratch, the space in which it lives needs to be specified using an isl_space. Each space involves zero or more parameters and zero, one or two tuples of set or input/output dimensions. The parameters and dimensions are identified by an isl_dim_type and a position. The type isl_dim_param refers to parameters, the type isl_dim_set refers to set dimensions (for spaces with a single tuple of dimensions) and the types isl_dim_in and isl_dim_out refer to input and output dimensions (for spaces with two tuples of dimensions). Local spaces (see Local Spaces) also contain dimensions of type isl_dim_div. Note that parameters are only identified by their position within a given object. Across different objects, parameters are (usually) identified by their names or identifiers. Only unnamed parameters are identified by their positions across objects. The use of unnamed parameters is discouraged.
  class Space : public Obj<Space,isl_space>, public Spacelike<Space> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_space_free(takeOrNull()); }
    StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    Space() { }
    //static ObjTy wrap(StructTy *obj) { return Space::enwrap(obj); }// obsolete

    /* implicit */ Space(const ObjTy &that) : Obj(that) { }
    /* implicit */ Space(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_space_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_space_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return *this; }
    Space getSpacelike() const { return *this; }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_dim_id(take(), type, pos, id.take())); }

    // optional
    bool isSet() const { return isSetSpace(); }
    bool isMap() const { return isMapSpace(); }

  public:
    void resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_QUALIFIER { give(isl_space_reset_tuple_id(take(), type)); }
    void resetDimId_inplace(isl_dim_type type, unsigned pos) ISLPP_INPLACE_QUALIFIER { give(isl_space_reset_dim_id(take(), type, pos)); }

    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_space_insert_dims(take(), type, pos, count)); }
    void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_space_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_space_dim(keep(), type); }
    int findDimById(isl_dim_type type, const Id &id) const { return isl_space_find_dim_by_id(keep(), type, id.keep()); }

    bool hasTupleId(isl_dim_type type) const { return isl_space_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_space_get_tuple_name(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_space_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_tuple_name(take(), type, s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_space_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_space_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_space_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_space_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Creational
    static Space createMapSpace(const Ctx *ctx, unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/);
    static Space createParamsSpace(const Ctx *ctx, unsigned nparam);
    static Space createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim);

    static Space createMapFromDomainAndRange(Space &&domain, Space &&range);
    static Space createMapFromDomainAndRange(Space &&domain, const Space &range) { assert(domain.getCtx() == range.getCtx()); return Space::enwrap(isl_space_map_from_domain_and_range(domain.take(), range.takeCopy())); }
    static Space createMapFromDomainAndRange(const Space &domain,  Space &&range) { assert(domain.getCtx() == range.getCtx()); return Space::enwrap(isl_space_map_from_domain_and_range(domain.takeCopy(), range.take())); }
    static Space createMapFromDomainAndRange(const Space &domain, const Space &range) { assert(domain.getCtx() == range.getCtx()); return Space::enwrap(isl_space_map_from_domain_and_range(domain.takeCopy(), range.takeCopy())); }
#pragma endregion


#pragma region Create other spaces
    Space mapsTo(const Space &range) const { return Space::enwrap(isl_space_map_from_domain_and_range(takeCopy(), range.takeCopy())); }
    Space mapsTo(unsigned nOut) const { return Space::enwrap(isl_space_map_from_domain_and_range(takeCopy(), isl_space_set_alloc(isl_space_get_ctx(keep()), 0, nOut) )); }
#pragma endregion


#pragma region Create Sets/Maps etc. using this map
    Set emptySet() const;
    Set universeSet() const;

    BasicSet emptyBasicSet() const;
    BasicSet universeBasicSet() const;

    BasicMap emptyBasicMap() const;
    BasicMap universeBasicMap() const;

    /// create a map where the first n_equal dimensions map to equal value
    BasicMap equalBasicMap(unsigned n_equal) const;

     /// Create a map that equates the selected dimensions
    BasicMap equalBasicMap(isl_dim_type type1, unsigned pos1, unsigned count, isl_dim_type type2, unsigned pos2) const;

    /// Create a relation the maps a value to everything that is lexically smaller at dimension pos
    BasicMap lessAtBasicMap(unsigned pos) const;

    /// Create a relation the maps a value to everything that is lexically larger at dimension pos
    BasicMap moreAtBasicMap(unsigned pos) const;

    Map emptyMap() const;
    Map universeMap() const;

    /// Maps vectors to anything that is lexically smaller
    Map lexLtMap() const;

    /// Maps vectors to anything that is lexically greater
    Map lexGtMap() const;

    /// Maps vectors to any vector that is lexically less in the first n coordinates (other coordinates are ignored, meaning vectors are lexically equal if their first pos coordinates are equal)
    Map lexLtFirstMap(unsigned pos) const;

    /// Maps vectors to any vector that is lexically greater in the first n coordinates (other coordinates are ignored, meaning vectors are lexically equal if their first pos coordinates are equal)
    Map lexGtFirstMap(unsigned pos) const;

    Aff createZeroAff() const;
    Aff createConstantAff(const Int &) const;
    Aff createVarAff(isl_dim_type type, unsigned pos) const;

    // Piecewise without pieces (i.e. defined on nothing)
    PwAff createEmptyPwAff() const;
    PwAff createZeroPwAff() const;
    MultiAff createZeroMultiAff() const;
    MultiPwAff createZeroMultiPwAff() const;
    PwMultiAff createEmptyPwMultiAff() const;

    Point zeroPoint() const;


    Constraint createZeroConstraint() const;
    Constraint createConstantConstraint(int) const;
    Constraint createVarConstraint(isl_dim_type type, int pos) const;

    Constraint createEqualityConstraint() const;
    Constraint createInequalityConstraint() const;

    Constraint createLtConstraint(Aff &&lhs, Aff &&rhs) const;
    Constraint createLeConstraint(Aff &&lhs, Aff &&rhs) const;
    Constraint createEqConstraint(Aff &&lhs, Aff &&rhs) const;
    Constraint createEqConstraint(Aff &&lhs, int rhs) const;
    Constraint createEqConstraint(Aff &&lhs, isl_dim_type type, unsigned pos) const;
    Constraint createGeConstraint(Aff &&lhs, Aff &&rhs) const;
    Constraint createGtConstraint(Aff &&lhs, Aff &&rhs) const;

    Expr createVarExpr(isl_dim_type type, int pos) const;
#pragma endregion


    bool isParamsSpace() const;
    bool isSetSpace() const;
    bool isMapSpace() const;

    bool matches(isl_dim_type thisType, const Space &that, isl_dim_type thatType) const { return isl_space_match(keep(), thisType, that.keep(), thatType); }


#pragma region Matching spaces
    bool matchesSpace(const Space &that) const {
  if (that.isSetSpace())
    return matchesSetSpace(that);

  if (that.isMapSpace())
    return matchesMapSpace(that);

  assert(that.isParamsSpace());
  return isParamsSpace();
    }


    bool matchesSetSpace(const Id &id) const { 
      if (!this->isSetSpace())
        return false;
      if (this->getSetTupleId() != id)
        return false;
      return true;
    }


    bool matchesSetSpace(const Space &that) const {
      assert(that.isSetSpace());

      if (!this->isSetSpace())
        return false;
      return matches(isl_dim_set, that, isl_dim_set);

      if (this->getSetDimCount() != that.getSetDimCount())
        return false;

      auto thisHasTupleId = this->hasTupleId(isl_dim_set);
      auto thatHasTupleId = that.hasTupleId(isl_dim_set);
      if (thisHasTupleId != thatHasTupleId)
        return false;

      if (thisHasTupleId && this->getSetTupleId()!= that.getSetTupleId()) 
        return false;

      return true;
    }


    bool matchesMapSpace(const Id &domainId, const Id &rangeId) const {
      if (!this->isMapSpace())
        return false;
      if (this->getInTupleId() != domainId)
        return false;
      if (this->getOutTupleId() != rangeId)
        return false;
      return true;
    }


    bool matchesMapSpace(const Space &domainSpace, const Space &rangeSpace) const {
  assert(domainSpace.isSetSpace());
  assert(rangeSpace.isSetSpace());
  if (!isMapSpace())
    return false;
  return matches(isl_dim_in, domainSpace, isl_dim_set) && matches(isl_dim_out, rangeSpace, isl_dim_set);
    }


    bool matchesMapSpace (const Space &that)  const {
      assert(that.isMapSpace());
      if (!this->isMapSpace())
        return false;

      return matches(isl_dim_in, that, isl_dim_in) &&  matches(isl_dim_out, that, isl_dim_out);
    }


    bool matchesMapSpace(const Space &domainSpace, const Id &rangeId)const {
      if (!this->isMapSpace())
        return false;
      return matches(isl_dim_in, domainSpace, isl_dim_set) && (getOutTupleId() == rangeId);
    }


    bool matchesMapSpace(const Id &domainId, const Space &rangeSpace) const{
      if (!this->isMapSpace())
        return false;
      return (getInTupleId() == domainId) && matches(isl_dim_out, rangeSpace, isl_dim_set);
    }
#pragma endregion


    bool isWrapping() const;
    void wrap_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_space_wrap(take())); }
    Space wrap() const { return Space::enwrap(isl_space_wrap(takeCopy())); }
    void unwrap_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_space_unwrap(take())); }
    Space unwrap() const { return Space::enwrap(isl_space_unwrap(takeCopy())); }

    Space domain() const { return Space::enwrap(isl_space_domain(takeCopy())); }
    Space range() const { return Space::enwrap(isl_space_range(takeCopy())); }
    Space params() const { return Space::enwrap(isl_space_params(takeCopy())); }
    Space extractTuple(isl_dim_type type) const {
      switch(type) {
      case isl_dim_param:
        return params();
      case isl_dim_in:
        return domain();
      case isl_dim_out:
        return range();
      case isl_dim_all:
        return copy();
      default:
        llvm_unreachable("Invalid dim type");
        return Space();
      }
    }

    void fromDomain();

    void fromRange();
    void setFromParams();
    void reverse();
    //void insertDims(isl_dim_type type, unsigned pos, unsigned n);
    //void addDims(isl_dim_type type, unsigned n);
    //void dropDims(isl_dim_type type, unsigned first, unsigned n);
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n);
    void mapFromSet();
    void zip();
    void curry();
    void uncurry();

#pragma region Create
    Set createUniverseSet() const;
    BasicSet createUniverseBasicSet() const;
    Map createUniverseMap() const;
    BasicMap createUniverseBasicMap() const;

    UnionMap createEmptyUnionMap() const;

    AstBuild createAstBuild() const;
#pragma endregion

    LocalSpace asLocalSpace() const;

    bool isNested(isl_dim_type type) const { return checkBool(isl_space_is_nested(keep(), type)); }
    bool isNestedDomain() const {assert(isMapSpace()); return checkBool(isl_space_is_nested(keep(), isl_dim_in)); }
    bool isNestedRange() const {assert(isMapSpace()); return checkBool(isl_space_is_nested(keep(), isl_dim_out)); }
    bool isNestedSet() const {assert(isSetSpace()); return checkBool(isl_space_is_nested(keep(), isl_dim_set)); }
    Space getNested(isl_dim_type type) const { return Space::enwrap(isl_space_get_nested(takeCopy(), type)); }
    Space getNestedDomain() const { assert(isMapSpace()); return Space::enwrap(isl_space_get_nested(takeCopy(), isl_dim_in)); }
    Space getNestedRange() const { assert(isMapSpace()); return Space::enwrap(isl_space_get_nested(takeCopy(), isl_dim_out)); }
    Space getNested() const { assert(isSetSpace()); return Space::enwrap(isl_space_get_nested(takeCopy(), isl_dim_set)); }


    /// Set the nested spaces
    void setNested_inplace(isl_dim_type type, const Space &nest) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_nested(take(), type, nest.takeCopy())); } 
    Space setNested(isl_dim_type type, const Space &nest) const { return Space::enwrap(isl_space_set_nested(takeCopy(), type, nest.takeCopy())); }
    void setInNested_inplace(const Space &nest) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_nested(takeCopy(), isl_dim_in, nest.takeCopy())); }
    Space setInNested(const Space &nest) const  { return Space::enwrap(isl_space_set_nested(takeCopy(), isl_dim_in, nest.takeCopy())); }
    void setOutNested_inplace(const Space &nest) ISLPP_INPLACE_QUALIFIER { give(isl_space_set_nested(take(), isl_dim_out, nest.takeCopy())); }
    Space setOutNested(const Space &nest) const  { return Space::enwrap(isl_space_set_nested(takeCopy(), isl_dim_out, nest.takeCopy())); }

    DimRange findNestedTuple(unsigned tuplePos) const;
    DimRange findNestedTuple(const Id &tupleId) const;

    void unwrapTuple_inplace(unsigned tuplePos) ISLPP_INPLACE_QUALIFIER;
    void unwrapTuple_inplace(const Id &tupleId) ISLPP_INPLACE_QUALIFIER;

    Space moveTuple(isl_dim_type dst_type, unsigned dst_tuplePos, isl_dim_type src_type, unsigned src_tuplePos) const;

    /// 0=isNull();
    /// 1=isSetSpace()
    /// 2=isMapSpace() or a set with a simple wrapped set
    /// ...
    unsigned nestedTupleCount() const;

    /// 0=isNull()
    /// 1=no nesting (!isWrapping()): set{ A } or map{ A -> B }
    /// 2=up to one level of nesting: set{ (A -> B) } or map{ (A -> B) -> (C -> D) }
    /// ...
    unsigned nestedMaxDepth() const;
    bool findTuple(isl_dim_type type, unsigned tuplePos, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount, /*out*/Id &tupleId) const;
    bool findTupleAt(isl_dim_type type, unsigned dimPos, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount, /*out*/Id &tupleId, /*out*/unsigned &tuplePos) const;
    bool findTuple(isl_dim_type type, const Id &tupleToFind, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount) const;
    unsigned findTuplePos(isl_dim_type type, const Id &tupleToFind) const;

    Space extractNestedTupleSpace(isl_dim_type type, unsigned tuplePos) const;
    Space extractNestedTupleSpace(isl_dim_type type, const Id &tupleToFind) const;

    Space extractDimRange(isl_dim_type type, unsigned first, unsigned count) const;
    std::vector<Space> flattenNestedSpaces() const;
  }; // class Space


  static inline Space enwrap(isl_space *obj) { return Space::enwrap(obj); }

  bool isEqual(const Space &space1, const Space &space2);
  /// checks whether the first argument is equal to the domain of the second argument. This requires in particular that the first argument is a set space and that the second argument is a map space.
  bool isDomain(const Space &space1, const Space &space2);
  bool isRange(const Space &space1, const Space &space2);

  Space join(Space &&left, Space &&right);
  Space alignParams(Space &&space1, Space &&space2);

  Space setTupleId(Space &&space, isl_dim_type type, Id &&id);
  Space setTupleId(Space &&space, isl_dim_type type, const Id &id);
  Space setTupleId(const Space &space, isl_dim_type type, Id &&id);
  Space setTupleId(const Space &space, isl_dim_type type, const Id &id);

  static inline bool operator==(const Space &lhs, const Space &rhs) { return isEqual(lhs, rhs); }
  static inline bool operator!=(const Space &lhs, const Space &rhs) { return !isEqual(lhs, rhs); }

} // namespace isl
#endif /* ISLPP_SPACE_H */
