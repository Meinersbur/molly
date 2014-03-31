#ifndef ISLPP_SPACE_H
#define ISLPP_SPACE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Ctx.h"
#include "Multi.h"
#include "Id.h"
#include "Expr.h"
#include "Obj.h" // isl::Obj<,> (baseclass of isl::Space)
#include "Spacelike.h" // isl::Spacelike<> (baseclass of isl::Space)

//#include <isl/ctx.h>
//#include <isl/space.h>

#include <assert.h>





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
  class Space;
} // namespace isl


namespace isl {

  /// Ensure both spaces have the same param dimensions in the same order
  void compatibilize(/*inout*/Space &space1, /*inout*/Space &space2);

  /// Test for equality of domain and range dimensions, but not param dims as these are aligned automatically
  bool matchesSpace(const Space &lhs, const Space &rhs);

  /// Like matchesSpace, bu never compares dim ids, just tuple ids recursively
  bool matchesTuples(const Space &lhs, const Space &rhs);

  /// Whenever a new set, relation or similar object is created from scratch, the space in which it lives needs to be specified using an isl_space. Each space involves zero or more parameters and zero, one or two tuples of set or input/output dimensions. The parameters and dimensions are identified by an isl_dim_type and a position. The type isl_dim_param refers to parameters, the type isl_dim_set refers to set dimensions (for spaces with a single tuple of dimensions) and the types isl_dim_in and isl_dim_out refer to input and output dimensions (for spaces with two tuples of dimensions). Local spaces (see Local Spaces) also contain dimensions of type isl_dim_div. Note that parameters are only identified by their position within a given object. Across different objects, parameters are (usually) identified by their names or identifiers. Only unnamed parameters are identified by their positions across objects. The use of unnamed parameters is discouraged.
  class Space : public Obj<Space, isl_space>, public Spacelike<Space> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_space_free(takeOrNull()); }
    StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    Space() { }

    /* implicit */ Space(const ObjTy &that) : Obj(that) { }
    /* implicit */ Space(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_space_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION{ return *this; }
    ISLPP_PROJECTION_ATTRS Space getSpacelike() ISLPP_PROJECTION_FUNCTION{ return *this; }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_is_params(keep())); }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_is_set(keep())); }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_is_map(keep())); }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_space_dim(keep(), type); }
    ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION{ return isl_space_find_dim_by_id(keep(), type, id.keep()); }

    ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_space_get_tuple_name(keep(), type); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_space_set_tuple_name(take(), type, s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_has_tuple_id(keep(), type)); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_space_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_space_set_tuple_id(take(), type, id.take())); }
    ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{ give(isl_space_reset_tuple_id(take(), type)); }

    ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_space_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_space_set_dim_name(take(), type, pos, s)); }
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_space_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_space_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_space_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_space_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_space_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_space_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_space_drop_dims(take(), type, first, count)); }
#pragma endregion


#pragma region Creational
    static Space createMapSpace(const Ctx *ctx, unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/);
    static Space createParamsSpace(const Ctx *ctx, unsigned nparam);
    static Space createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim);

    static Space createMapFromDomainAndRange(Space &&domain, Space &&range);
    static Space createMapFromDomainAndRange(Space &&domain, const Space &range) { return createMapFromDomainAndRange(domain.move(), range.copy()); }
    static Space createMapFromDomainAndRange(const Space &domain, Space &&range) { return createMapFromDomainAndRange(domain.copy(), range.move()); }
    static Space createMapFromDomainAndRange(const Space &domain, const Space &range) { return createMapFromDomainAndRange(domain.copy(), range.copy()); }
    static Space createMapFromDomainAndRange(count_t domainDims, Space rangeSpace) { auto islctx = rangeSpace.getCtx(); return Space::enwrap(isl_space_map_from_domain_and_range(isl_space_set_alloc(islctx->keep(), 0, domainDims), rangeSpace.take())); }
    static Space createMapFromDomainAndRange(Space domainSpace, count_t rangeDims) { auto islctx = domainSpace.getCtx(); return Space::enwrap(isl_space_map_from_domain_and_range(domainSpace.take(), isl_space_set_alloc(islctx->keep(), 0, rangeDims))); }
#pragma endregion


#pragma region Create other spaces
    Space mapsTo(Space range) ISLPP_EXSITU_FUNCTION{
      auto self = copy();
      compatibilize(self, range);
      return Space::enwrap(isl_space_map_from_domain_and_range(self.take(), range.take()));
    }
    ISLPP_EXSITU_ATTRS Space mapsTo(count_t nOut) ISLPP_EXSITU_FUNCTION { return Space::enwrap(isl_space_map_from_domain_and_range(takeCopy(), isl_space_align_params(isl_space_set_alloc(isl_space_get_ctx(keep()), 0, nOut), getSpace().take()))); }
    ISLPP_EXSITU_ATTRS Space mapsToItself() ISLPP_EXSITU_FUNCTION { assert(isSet()); return Space::createMapFromDomainAndRange(*this, *this); }

      // If this is a param space
    Space createSetSpace(count_t nDims) const { assert(isParamsSpace()); return Space::enwrap(isl_space_align_params(isl_space_set_alloc(getCtx()->keep(), 0, nDims), takeCopy())); }
    Space createMapSpace(count_t nDomainDims, count_t nRangeDims) const { assert(isParamsSpace()); return Space::enwrap(isl_space_align_params(isl_space_alloc(getCtx()->keep(), 0, nDomainDims, nRangeDims), takeCopy())); }
    ISLPP_EXSITU_ATTRS Space createMapSpace(count_t nDomainDims, Space rangeSpace) ISLPP_EXSITU_FUNCTION;
#pragma endregion


#pragma region Create Sets/Maps etc. using this map
    Set emptySet() const;
    Set universeSet() const;

    BasicSet emptyBasicSet() const;
    BasicSet universeBasicSet() const;

    BasicMap emptyBasicMap() const;
    BasicMap universeBasicMap() const;
    BasicMap identityBasicMap() const;

    ISLPP_EXSITU_ATTRS BasicMap identityToSelfBasicMap() ISLPP_EXSITU_FUNCTION;

    /// create a map where the first n_equal dimensions map to equal value
    BasicMap equalBasicMap(unsigned n_equal) const;
    ISLPP_EXSITU_ATTRS BasicMap equalBasicMap() ISLPP_EXSITU_FUNCTION;

    /// Create a map that equates the selected dimensions
    BasicMap equalBasicMap(isl_dim_type type1, unsigned pos1, unsigned count, isl_dim_type type2, unsigned pos2) const;
    BasicMap equalSubspaceBasicMap(const Space &domainSubpace, const Space &rangeSubspace) const;
    BasicMap equalSubspaceBasicMap(const Space &subspace) const;

    ISLPP_EXSITU_ATTRS BasicSet equalBasicSet(isl_dim_type type1, unsigned pos1, unsigned count, isl_dim_type type2, unsigned pos2) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS BasicSet equalBasicSet(Space subspace1, Space subspace2) ISLPP_EXSITU_FUNCTION;

    //ISLPP_EXSITU_ATTRS MultiAff equalMultiAff(Space inputSubspace, Space outputSubspace) ISLPP_EXSITU_FUNCTION;


    /// Create a relation the maps a value to everything that is lexically smaller at dimension pos
    BasicMap lessAtBasicMap(unsigned pos) const;

    /// Create a relation the maps a value to everything that is lexically larger at dimension pos
    BasicMap moreAtBasicMap(unsigned pos) const;

    Map emptyMap() const;
    Map universeMap() const;
    Map identityMap() const;

    /// Maps vectors to anything that is lexically smaller
    Map lexLtMap() const;
    Map lexLeMap() const;

    /// Maps vectors to anything that is lexically greater
    Map lexGtMap() const;
    Map lexGeMap() const;

    /// Maps vectors to any vector that is lexically less in the first n coordinates (other coordinates are ignored, meaning vectors are lexically equal if their first pos coordinates are equal)
    Map lexLtFirstMap(unsigned pos) const;
    Map lexLeFirstMap(unsigned pos) const;

    /// Maps vectors to any vector that is lexically greater in the first n coordinates (other coordinates are ignored, meaning vectors are lexically equal if their first pos coordinates are equal)
    Map lexGtFirstMap(unsigned pos) const;
    Map lexGeFirstMap(unsigned pos) const;

    UnionMap emptyUnionMap() const;
    UnionSet emptyUnionSet() const;

    Aff createZeroAff() const;
    Aff createConstantAff(const Int &) const;

    Aff createVarAff(isl_dim_type type, unsigned pos) const;
    Aff createAffOnVar(unsigned pos) const;
    Aff createAffOnParam(const Id &dimId) const;

    AffExpr createVarAffExpr(isl_dim_type type, unsigned pos) const;

    // Piecewise without pieces (i.e. defined on nothing)
    PwAff createEmptyPwAff() const;
    PwAff createZeroPwAff() const;
    MultiAff createZeroMultiAff() const;
    MultiAff createIdentityMultiAff() const;
    MultiPwAff createZeroMultiPwAff() const;
    ISLPP_EXSITU_ATTRS MultiPwAff createEmptyMultiPwAff() ISLPP_EXSITU_FUNCTION;
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
    bool isNontrivialSetSpace() const { return isSetSpace() && !isParamsSpace(); }
    bool isMapSpace() const;


    bool match(isl_dim_type thisType, const Space &that, isl_dim_type thatType) const {
      return isl::matchesSpace(extractTuple(thisType), that.extractTuple(thatType));
    }


#pragma region Matching spaces
    bool matchesSpace(const Space &that) const {
      return isl::matchesSpace(*this, that);
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
      return isSet() && isl::matchesSpace(*this, that);
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
      return isMap() && isl::matchesSpace(domain(), domainSpace) && isl::matchesSpace(range(), rangeSpace);
    }


    bool matchesMapSpace(const Space &that)  const {
      assert(that.isMapSpace());
      return isMap() && isl::matchesSpace(*this, that);
    }


    bool matchesMapSpace(const Space &domainSpace, const Id &rangeId)const {
      if (!this->isMapSpace())
        return false;
      return match(isl_dim_in, domainSpace, isl_dim_set) && (getOutTupleId() == rangeId);
    }


    bool matchesMapSpace(const Id &domainId, const Space &rangeSpace) const{
      if (!this->isMapSpace())
        return false;
      return (getInTupleId() == domainId) && match(isl_dim_out, rangeSpace, isl_dim_set);
    }
#pragma endregion


    /// Returns whether this is a set space which wraps a map space
    bool isWrapping() const;

    /// Wraps a map space into a set space
    /// map{ A -> B }.wrap() = set{ (A -> B) }
    ///  or with alternative notation: { A -> B }.wrap() = { (A, B) }
    Space wrap() const { return Space::enwrap(isl_space_wrap(takeCopy())); }
    void wrap_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_space_wrap(take())); }
    Space wrap_consume() { return Space::enwrap(isl_space_wrap(take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Space wrap() && { return Space::enwrap(isl_space_wrap(take())); }
#endif

    /// Undoes wrap()
    Space unwrap() const { return Space::enwrap(isl_space_unwrap(takeCopy())); }
    void unwrap_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_space_unwrap(take())); }
    Space unwrap_consume() { return Space::enwrap(isl_space_unwrap(take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Space unwrap() && { return Space::enwrap(isl_space_unwrap(take())); }
#endif

    Space domain() const { return Space::enwrap(isl_space_domain(takeCopy())); }
    Space range() const { return Space::enwrap(isl_space_range(takeCopy())); }
    Space params() const { return Space::enwrap(isl_space_params(takeCopy())); }
    Space extractTuple(isl_dim_type type) const {
      switch (type) {
      case isl_dim_param:
        return params(); //TODO: Does is return a set space?
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

    Space getParamsSpace() const { return Space::enwrap(isl_space_params(takeCopy())); }
    Space getDomainSpace() const { return Space::enwrap(isl_space_domain(takeCopy())); }
    Space getRangeSpace() const { return Space::enwrap(isl_space_range(takeCopy())); }

    Space fromDomain() ISLPP_EXSITU_FUNCTION{ return Space::enwrap(isl_space_from_domain(takeCopy())); }
    Space fromRange() ISLPP_EXSITU_FUNCTION{ return Space::enwrap(isl_space_from_range(takeCopy())); }

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
    UnionSet createEmptyUnionSet() const;

    AstBuild createAstBuild() const;
#pragma endregion

    LocalSpace asLocalSpace() const;

    ISLPP_DEPRECATED bool isNested(isl_dim_type type) const { return extractTuple(type).isWrapping(); }
    ISLPP_DEPRECATED bool isNestedDomain() const { assert(isMapSpace()); return domain().isWrapping(); }
    ISLPP_DEPRECATED bool isNestedRange() const { assert(isMapSpace()); return range().isWrapping(); }
    ISLPP_DEPRECATED bool isNestedSet() const { assert(isSetSpace()); return isWrapping(); }
    //ISLPP_DEPRECATED Space getNested(isl_dim_type type) const { return isNested(type) ? extractTuple(type).unwrap() : Space(); }
    //ISLPP_DEPRECATED Space getNestedDomain() const { assert(isMapSpace()); return isNestedDomain() ? domain() : Space(); }
    //ISLPP_DEPRECATED Space getNestedRange() const { assert(isMapSpace()); return isNestedRange() ? range() : Space(); }
    ISLPP_DEPRECATED Space getNested() const { assert(isSetSpace()); return isWrapping() ? this->copy() : Space(); }

    //ISLPP_DEPRECATED Space getNestedOrDefault(isl_dim_type type) const;


    /// Set the nested spaces
    //ISLPP_DEPRECATED void setNested_inplace(isl_dim_type type, const Space &nest) ISLPP_INPLACE_FUNCTION { obj_give(setNested(type,nest)); } 
    ISLPP_DEPRECATED Space setNested(isl_dim_type type, const Space &nest) const {
      if (isSetSpace() && type == isl_dim_set)
        return nest;
      else if (isMapSpace() && type == isl_dim_in)
        return Space::createMapFromDomainAndRange(nest, range());
      else if (isMapSpace() && type == isl_dim_out)
        return Space::createMapFromDomainAndRange(domain(), nest);
      else
        llvm_unreachable("unsupported type");
    }
    //ISLPP_DEPRECATED void setInNested_inplace(const Space &nest) ISLPP_INPLACE_FUNCTION { assert(isMapSpace()); setNested_inplace(isl_dim_in, nest); }
    //ISLPP_DEPRECATED Space setInNested(const Space &nest) const { assert(isMapSpace()); return setNested(isl_dim_in,nest); }
    //ISLPP_DEPRECATED void setOutNested_inplace(const Space &nest) ISLPP_INPLACE_FUNCTION {assert(isMapSpace()); setNested_inplace(isl_dim_out, nest); }
    //ISLPP_DEPRECATED Space setOutNested(const Space &nest) const { assert(isMapSpace()); return setNested(isl_dim_in,nest); }

    //DimRange findNestedTuple(unsigned tuplePos) const;
    //DimRange findNestedTuple(const Id &tupleId) const;
    DimRange findSubspace(isl_dim_type type, const Space &subspace) const;

    ISLPP_DEPRECATED Space moveTuple(isl_dim_type dst_type, unsigned dst_tuplePos, isl_dim_type src_type, unsigned src_tuplePos) const;

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

    Space removeSubspace(const Space &subspace) const;
    Space replaceSubspace(const Space &subspaceToReplace, const Space &replaceBy) const;

    ISLPP_EXSITU_ATTRS Space alignParams(Space that) ISLPP_EXSITU_FUNCTION{ return Space::enwrap(isl_space_align_params(this->takeCopy(), that.take())); }
    ISLPP_INPLACE_ATTRS  void alignParams_inplace(Space that) ISLPP_INPLACE_FUNCTION{ give(isl_space_align_params(this->take(), that.take())); }
    ISLPP_CONSUME_ATTRS Space alignParams_consume(Space that) ISLPP_CONSUME_FUNCTION{ return Space::enwrap(isl_space_align_params(this->take(), that.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    Space alignParams(Space that) && { return Space::enwrap(isl_space_align_params(this->take(), that.take())); } 
#endif

    /// If a nested set space, unwrap it
    /// May return set and map spaces
    Space normalizeUnwrapped() const {
      if (isWrapping())
        return unwrap();
      return copy();
    }
    void normalizeUnwrapped_inplace() ISLPP_INPLACE_FUNCTION{
      if (isWrapping())
      unwrap_inplace();
    }

      /// Guaranteed to return a set space
      Space normalizeWrapped() const {
        if (isMapSpace())
          return wrap();
        return copy();
    }
    void normalizeWrapped_inplace() ISLPP_INPLACE_FUNCTION{
      if (isMapSpace())
      wrap_inplace();
      assert(!isMapSpace());
    }

      /// Return the pos' (0=first element) encountered leaf space in a depth-first traversal
    Space findNthSubspace(isl_dim_type type, unsigned pos, DimRange &dimrange) const;
    Space findNthSubspace(isl_dim_type type, unsigned pos) const { DimRange dummy; return findNthSubspace(type, pos, dummy); }

    /// Remove nesting and dim ids from non-param dimensions
    ISLPP_EXSITU_ATTRS Space untyped() ISLPP_EXSITU_FUNCTION{
      if (isParamsSpace())
      return *this;

      if (isSetSpace()) {
        return params().createSetSpace(getSetDimCount());
      }

      assert(isMapSpace());
      return params().createMapSpace(getInDimCount(), getOutDimCount());
    }

      ISLPP_EXSITU_ATTRS Space selfMap() ISLPP_EXSITU_FUNCTION{
      auto norm = normalizeWrapped();
      return createMapFromDomainAndRange(norm, norm);
    }
  }; // class Space


  static inline Space enwrap(__isl_take isl_space *obj) { return Space::enwrap(obj); }
  static inline Space enwrapCopy(__isl_take isl_space *obj) { return Space::enwrapCopy(obj); }

  BasicMap alltoall(Space domain, Space range);

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

  static inline bool operator==(const Space &lhs, const Space &rhs) { return (lhs.isNull() && rhs.isNull()) || (lhs.isValid() && rhs.isValid() && isEqual(lhs, rhs)); }
  static inline bool operator!=(const Space &lhs, const Space &rhs) { return !operator==(lhs, rhs); }

  /// Create spaceNesting from valid subspaces
  Space combineSpaces(const Space &lhs, const Space &rhs);

  static inline Space operator>>(const Space &domainSpace, const Space &rangeSpace) {
    return isl::Space::createMapFromDomainAndRange(domainSpace.normalizeWrapped(), rangeSpace.normalizeWrapped());
  }

  template<typename T, typename S>
  static inline bool matchesSpace(const Spacelike<T> &lhs, const Spacelike<S> &rhs) {
    return matchesSpace(lhs.getDerived()->getSpace(), rhs.getDerived()->getSpace());
  }

} // namespace isl
#endif /* ISLPP_SPACE_H */
