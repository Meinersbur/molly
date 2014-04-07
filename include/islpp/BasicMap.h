#ifndef ISLPP_BASICMAP_H
#define ISLPP_BASICMAP_H

#include "islpp_common.h"

#include "Obj.h" // class Obj<> (base of BasicMap)
#include "Spacelike.h" // class Spacelike<> (base of BasicMap)

#include "Ctx.h"
#include "Space.h"
#include "Aff.h"
#include "LocalSpace.h"
#include "Id.h"
#include "BasicSet.h"
#include "AffList.h"
#include "Mat.h"
#include "MultiAff.h"
#include "Constraint.h"
#include "MapSpace.h"
#include "SetSpace.h"

#include <isl/map.h>
#include <isl/constraint.h>
#include <isl/deprecated/map_int.h>

#include <llvm/Support/ErrorHandling.h>
#include <cassert>


struct isl_basic_map;

namespace llvm {
} // namespace llvm

namespace isl {
  class Map;
} // namespace isl


namespace isl {
  class BasicMap : public Obj<BasicMap, struct isl_basic_map>, public SetSpacelike<BasicMap>, public Spacelike<BasicMap> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_basic_map_free(takeOrNull()); }
    StructTy *addref() const { return isl_basic_map_copy(keepOrNull()); }

  public:
    BasicMap() { }
    //static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    /* implicit */ BasicMap(const ObjTy &that) : Obj(that) { }
    /* implicit */ BasicMap(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_basic_map_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS MapSpace getSpace() ISLPP_PROJECTION_FUNCTION{ return MapSpace::enwrap(isl_basic_map_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION{ return getLocalSpace(); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return true; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_basic_map_dim(keep(), type); }
      //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_basic_map_find_dim_by_id(keep(), type, id.keep()); }

      //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_map_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_basic_map_get_tuple_name(keep(), type); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_set_tuple_name(take(), type, s)); }
      //ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_map_has_tuple_id(keep(), type)); }
      //ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_basic_map_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_set_tuple_id(take(), type, id.take())); }
      //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_basic_map_reset_tuple_id(take(), type)); }

      //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_basic_map_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_basic_map_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_set_dim_name(take(), type, pos, s)); }
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_basic_map_has_dim_id(keep(), type, pos)); }
      //ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_basic_map_get_dim_id(keep(), type, pos)); }
      //ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION { give(isl_basic_map_set_dim_id(take(), type, pos, id.take())); }
      ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{
      auto space = getSpace();
      space.setDimId_inplace(type, pos, std::move(id));
      cast_inplace(space);
    }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_basic_map_reset_dim_id(take(), type, pos)); }

  protected:
    //ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_basic_map_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_remove_dims(take(), type, first, count)); }
#pragma endregion


    ISLPP_PROJECTION_ATTRS LocalSpace getLocalSpace() ISLPP_PROJECTION_FUNCTION{ return LocalSpace::enwrap(isl_basic_map_get_local_space(keep())); }


#pragma region Conversion
  public:
    Map toMap() const;
    //operator Map() const { return toMap(); }
#pragma endregion


    unsigned nIn() const { return isl_basic_map_n_in(keep()); }
    unsigned nOut() const { return isl_basic_map_n_out(keep()); }
    unsigned nParam() const { return isl_basic_map_n_param(keep()); }
    unsigned nDiv() const { return isl_basic_map_n_div(keep()); }
    unsigned totalDim() const { return isl_basic_map_total_dim(keep()); }

    Aff getDiv(int pos) const { return Aff::enwrap(isl_basic_map_get_div(keep(), pos)); }



    bool isRational() const { return isl_basic_map_is_rational(keep()); }


#pragma region Creational
    static BasicMap create(Ctx *ctx, unsigned nparam, unsigned in, unsigned out, unsigned extra, unsigned n_eq, unsigned n_ineq) { return BasicMap::enwrap(isl_basic_map_alloc(ctx->keep(), nparam, in, out, extra, n_eq, n_ineq)); }
    static BasicMap createIdentity(Space &&space) { return BasicMap::enwrap(isl_basic_map_identity(space.take())); }
    static BasicMap createIdentityLike(BasicMap &&model) { return BasicMap::enwrap(isl_basic_map_identity_like(model.take())); }

    static BasicMap createEqual(Space space, unsigned n_equal) { return BasicMap::enwrap(isl_basic_map_equal(space.take(), n_equal)); }
    static BasicMap createLessAt(Space space, unsigned pos) { return BasicMap::enwrap(isl_basic_map_less_at(space.take(), pos)); }
    static BasicMap createMoreAt(Space space, unsigned pos) { return BasicMap::enwrap(isl_basic_map_more_at(space.take(), pos)); }
    static BasicMap createEmpty(Space dim) { return BasicMap::enwrap(isl_basic_map_empty(dim.take())); }
    static BasicMap createEmptyLikeMap(Map &&model);
    static BasicMap createEmptyLike(BasicMap &&model) { return BasicMap::enwrap(isl_basic_map_empty_like(model.take())); }

    static BasicMap createUniverse(Space &&dim) { return BasicMap::enwrap(isl_basic_map_identity(dim.take())); }
    static BasicMap createNatUniverse(Space &&dim) { return BasicMap::enwrap(isl_basic_map_nat_universe(dim.take())); }
    static BasicMap createUniverseLike(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_universe_like(bmap.take())); }

    static BasicMap readFromFile(Ctx *ctx, FILE *input) { return BasicMap::enwrap(isl_basic_map_read_from_file(ctx->keep(), input)); }
    static BasicMap readFromFStr(Ctx *ctx, const char *str) { return BasicMap::enwrap(isl_basic_map_read_from_str(ctx->keep(), str)); }

    static BasicMap createFromBasicSet(BasicSet &&bset, Space &&space) { return BasicMap::enwrap(isl_basic_map_from_basic_set(bset.take(), space.take())); }
    static BasicMap createFromDomain(BasicSet &&bset) { return BasicMap::enwrap(isl_basic_map_from_domain(bset.take())); }
    static BasicMap createFromRange(BasicSet &&bset) { return BasicMap::enwrap(isl_basic_map_from_range(bset.take())); }
    static BasicMap createFromDomainAndRange(BasicSet &&domain, BasicSet &&range) { return BasicMap::enwrap(isl_basic_map_from_domain_and_range(domain.take(), range.take())); }
    static BasicMap createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type c5);
    static BasicMap createFromAff(Aff &&aff) { return BasicMap::enwrap(isl_basic_map_from_aff(aff.take())); }
    static BasicMap createFromMultiAff(MultiAff &&maff) { return BasicMap::enwrap(isl_basic_map_from_multi_aff(maff.take())); }
    static BasicMap createFromAffList(Space &&space, AffList &&list) { return BasicMap::enwrap(isl_basic_map_from_aff_list(space.take(), list.take())); }

    /// Create a map that contains all the domain elements that are, in all coordinates, less than the range elements (domain[0..n-1] < range[0..n-1])
    /// space is a set space, returns an mapping from that space to the same space
    static BasicMap createAllLt(Space space) {
      assert(space.isSetSpace());
      auto resultSpace = space.mapsToItself();
      auto result = resultSpace.createUniverseBasicMap();
      auto localSpace = resultSpace.asLocalSpace();
      auto nDims = space.getSetDimCount();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto c = localSpace.createInequalityConstraint();
        c.setCoefficient_inplace(isl_dim_in, i, -1);
        c.setCoefficient_inplace(isl_dim_out, i, +1);
        c.setConstant_inplace(1);
        result.addConstraint_inplace(std::move(c));
      }
      return result; // NRVO
    }
    static BasicMap createAllLe(Space space) {
      assert(space.isSetSpace());
      auto resultSpace = space.mapsToItself();
      auto result = resultSpace.createUniverseBasicMap();
      auto localSpace = resultSpace.asLocalSpace();
      auto nDims = space.getSetDimCount();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto c = localSpace.createInequalityConstraint();
        c.setCoefficient_inplace(isl_dim_in, i, -1);
        c.setCoefficient_inplace(isl_dim_out, i, +1);
        c.setConstant_inplace(0);
        result.addConstraint_inplace(std::move(c));
      }
      return result; // NRVO
    }
    static BasicMap createAllGe(Space space) { return createAllLe(std::move(space)).reverse(); }
    static BasicMap createAllEq(Space space) { return createEqual(space.copy(), space.getSetDimCount()); }
    static BasicMap createAllGt(Space space) { return createAllLt(std::move(space)).reverse(); }
#pragma endregion


    void finalize_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_finalize(take())); }
    void extend(unsigned nparam, unsigned n_in, unsigned n_out, unsigned extra, unsigned n_eq, unsigned n_ineq) { give(isl_basic_map_extend(take(), nparam, n_in, n_out, extra, n_eq, n_ineq)); }
    void extendConstraints(unsigned n_eq, unsigned n_ineq) { give(isl_basic_map_extend_constraints(take(), n_eq, n_ineq)); }

    void removeRedundancies() { give(isl_basic_map_remove_redundancies(take())); }

    void intersect_inplace(const BasicMap &bmap) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_intersect(take(), bmap.takeCopy())); }
    BasicMap intersect(const BasicMap &bmap) const { return BasicMap::enwrap(isl_basic_map_intersect(takeCopy(), bmap.takeCopy())); }

    ISLPP_EXSITU_ATTRS Map intersect(Map &&that) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Map intersect(const Map &that) ISLPP_EXSITU_FUNCTION;

    BasicMap intersectDomain(const BasicSet &bset) ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_intersect_domain(takeCopy(), bset.takeCopy())); }
    Map intersectDomain(const Set &set) ISLPP_EXSITU_FUNCTION;
    BasicMap intersectRange(const BasicSet &bset) ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_intersect_range(takeCopy(), bset.takeCopy())); }
    Map intersectRange(const Set &set) ISLPP_EXSITU_FUNCTION;

    void affineHull() { give(isl_basic_map_affine_hull(take())); }
    void reverse_inplace() { give(isl_basic_map_reverse(take())); }
    ISLPP_EXSITU_ATTRS BasicMap reverse() ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_reverse(takeCopy())); }

    ISLPP_EXSITU_ATTRS BasicSet domain() ISLPP_EXSITU_FUNCTION{ return BasicSet::wrap(isl_basic_map_domain(takeCopy())); }
    ISLPP_EXSITU_ATTRS BasicSet getDomain() ISLPP_EXSITU_FUNCTION{ return BasicSet::wrap(isl_basic_map_domain(takeCopy())); }
    ISLPP_EXSITU_ATTRS BasicSet range() ISLPP_EXSITU_FUNCTION{ return BasicSet::wrap(isl_basic_map_range(takeCopy())); }
    ISLPP_EXSITU_ATTRS BasicSet getRange() ISLPP_EXSITU_FUNCTION{ return BasicSet::wrap(isl_basic_map_range(takeCopy())); }

    void removeDims(isl_dim_type type, unsigned first, unsigned n) { give(isl_basic_map_remove_dims(take(), type, first, n)); }
    void eliminate(isl_dim_type type, unsigned first, unsigned n) { give(isl_basic_map_eliminate(take(), type, first, n)); }

    BasicMap simplify() ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_simplify(takeCopy())); }
    BasicMap detectEqualities() ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_detect_equalities(takeCopy())); }

      //    void dump() const;
    void print(FILE *out, int indent, const char *prefix, const char *suffix, unsigned output_format) const { isl_basic_map_print(keep(), out, indent, prefix, suffix, output_format); }

    void fix_inplace(isl_dim_type type, unsigned pos, int value) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_fix_si(take(), type, pos, value)); }
    BasicMap fix(isl_dim_type type, unsigned pos, int value) const { return BasicMap::enwrap(isl_basic_map_fix_si(takeCopy(), type, pos, value)); }

    BasicMap lowerBound(isl_dim_type type, unsigned pos, int value) const { return BasicMap::enwrap(isl_basic_map_lower_bound_si(takeCopy(), type, pos, value)); }
    BasicMap upperBound(isl_dim_type type, unsigned pos, int value) const { return BasicMap::enwrap(isl_basic_map_upper_bound_si(takeCopy(), type, pos, value)); }

    bool isFixed(isl_dim_type type, unsigned pos, Int &val) const {
      isl_int rawval;
      isl_int_init(rawval);
      auto result = isl_basic_map_plain_is_fixed(keep(), type, pos, &rawval);
      val = Int::wrap(rawval);
      return result;
    }

    //bool imageIsBounded() const { return isl_basic_map_image_is_bounded(keep()); } // isBoundedRange
    bool isUniverse() const { return isl_basic_map_is_universe(keep()); }
    bool plainIsEmpty() const { return isl_basic_map_plain_is_empty(keep()); }
    bool fastIsEmpty() const { return isl_basic_map_fast_is_empty(keep()); }
    bool isEmpty() const { return isl_basic_map_is_empty(keep()); }

    //void add(isl_dim_type type, unsigned n) { give(isl_basic_map_add(take(), type, n)); } 
    //void addDims(isl_dim_type type, unsigned n) { add(type,n); } 
    //void insertDims(isl_dim_type type, unsigned pos, unsigned n) { give(isl_basic_map_insert_dims(take(), type, pos, n)); }
    void removeDivs() { give(isl_basic_map_remove_divs(take())); }

    bool isSingleValued() const { return isl_basic_map_is_single_valued(keep()); }
    bool canZip() const { return isl_basic_map_can_zip(keep()); }
    bool canCurry() const { return isl_basic_map_can_curry(keep()); }
    bool canUncurry() const { return isl_basic_map_can_uncurry(keep()); }
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const { return isl_basic_map_involves_dims(keep(), type, first, n); }

    Mat equalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type c5) { return Mat::enwrap(isl_basic_map_equalities_matrix(keep(), c1, c2, c3, c4, c5)); }
    Mat inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type c5) { return Mat::enwrap(isl_basic_map_inequalities_matrix(keep(), c1, c2, c3, c4, c5)); }

    void addConstraint_inplace(Constraint &&constraint) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_add_constraint(take(), constraint.take())); }
    void addConstraint_inplace(const Constraint &constraint) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_add_constraint(take(), constraint.takeCopy())); }
    BasicMap addContraint(Constraint &&constraint) const { return BasicMap::enwrap(isl_basic_map_add_constraint(takeCopy(), constraint.take())); }
    BasicMap addContraint(const Constraint &constraint) const { return BasicMap::enwrap(isl_basic_map_add_constraint(takeCopy(), constraint.takeCopy())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    BasicMap addContraint(Constraint &&constraint) && { return BasicMap::enwrap(isl_basic_map_add_constraint(take(), constraint.take())); } 
    BasicMap addContraint(const Constraint &constraint) && { return BasicMap::enwrap(isl_basic_map_add_constraint(take(), constraint.takeCopy())); } 
#endif


    bool foreachConstraint(const std::function<bool(Constraint)> &func) const;
    std::vector<Constraint> getConstraints() const;

    void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION;
    BasicMap cast(Space space) const { auto result = copy(); result.cast_inplace(space.move()); return result; }

    void equate_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_equate(take(), type1, pos1, type2, pos2)); }
    BasicMap equate(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return BasicMap::enwrap(isl_basic_map_equate(takeCopy(), type1, pos1, type2, pos2)); }
    void equate_inplace(Dim dim1, Dim dim2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_equate(takeCopy(), dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos())); }
    BasicMap equate(Dim dim1, Dim dim2) const { return BasicMap::enwrap(isl_basic_map_equate(takeCopy(), dim1.getType(), dim1.getPos(), dim2.getType(), dim2.getPos())); }

    void orderGt_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_order_gt(take(), type1, pos1, type2, pos2)); }
    BasicMap orderGt(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return BasicMap::enwrap(isl_basic_map_order_gt(takeCopy(), type1, pos1, type2, pos2)); }
    void orderGe_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_order_ge(take(), type1, pos1, type2, pos2)); }
    BasicMap orderGe(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return BasicMap::enwrap(isl_basic_map_order_ge(takeCopy(), type1, pos1, type2, pos2)); }
    void orderLt_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_order_gt(take(), type2, pos2, type1, pos1)); }
    BasicMap orderLt(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return BasicMap::enwrap(isl_basic_map_order_gt(takeCopy(), type2, pos2, type1, pos1)); }
    void orderLe_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_order_ge(take(), type2, pos2, type1, pos1)); }
    BasicMap orderLe(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return BasicMap::enwrap(isl_basic_map_order_ge(takeCopy(), type2, pos2, type1, pos1)); }

    SetSpace getDomainSpace() const { return SetSpace::enwrap(isl_space_domain(isl_basic_map_get_space(takeCopy()))); }
    SetSpace getRangeSpace() const { return SetSpace::enwrap(isl_space_range(isl_basic_map_get_space(takeCopy()))); }

    Map domainProduct(const Map &that) const;
    Map rangeProduct(const Map &that) const;

    ISLPP_INPLACE_ATTRS void applyDomain_inplace(BasicMap that) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_apply_domain(take(), that.take())); }
    ISLPP_EXSITU_ATTRS BasicMap applyDomain(BasicMap that) ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_apply_domain(takeCopy(), that.take())); }
    Map applyDomain(const Map &that) const;

    ISLPP_INPLACE_ATTRS void applyRange_inplace(BasicMap that) ISLPP_INPLACE_FUNCTION{ give(isl_basic_map_apply_range(take(), that.take())); }
    ISLPP_EXSITU_ATTRS BasicMap applyRange(BasicMap that) ISLPP_EXSITU_FUNCTION{ return BasicMap::enwrap(isl_basic_map_apply_range(takeCopy(), that.take())); }
    Map applyRange(const Map &that) const;

    ISLPP_EXSITU_ATTRS Aff dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION;

    ISLPP_PROJECTION_ATTRS bool imageIsBounded() ISLPP_PROJECTION_FUNCTION{ // isBoundedRange
      return checkBool(isl_basic_map_image_is_bounded(keep()));
    }

    ISLPP_EXSITU_ATTRS  BasicSet      wrap()ISLPP_EXSITU_ATTRS{ return BasicSet::enwrap(isl_basic_map_wrap(takeCopy())); }
  }; // class BasicMap


  static inline BasicMap copy(const BasicMap &bmap) { return BasicMap::enwrap(isl_basic_map_copy(bmap.keep())); }
  static inline BasicMap enwrap(__isl_take isl_basic_map *bmap) { return BasicMap::enwrap(bmap); }

  static inline BasicMap intersect(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_intersect(bmap1.take(), bmap2.take())); }
  Map unite(BasicMap &&bmap1, BasicMap &&bmap2);
  static inline BasicMap applyDomain(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_apply_domain(bmap1.take(), bmap2.take())); }
  static inline BasicMap applyRange(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_apply_range(bmap1.take(), bmap2.take())); }

  static inline BasicSet domain(BasicMap &&bmap) { return BasicSet::wrap(isl_basic_map_domain(bmap.take())); }
  static inline BasicSet range(BasicMap &&bmap) { return BasicSet::wrap(isl_basic_map_range(bmap.take())); }

  static inline BasicMap domainMap(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_domain_map(bmap.take())); }
  static inline BasicMap rangeMap(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_range_map(bmap.take())); }

  static inline BasicMap sample(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_sample(bmap.take())); }
  static inline BasicMap sum(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_sum(bmap1.take(), bmap2.take())); }
  static inline BasicMap neg(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_neg(bmap.take())); }
  //static inline BasicMap floordiv(BasicMap &&bmap, const Int &d) { return BasicMap::enwrap(isl_basic_map_floordiv(bmap.take(), d.keep())); }

  static inline bool isEqual(BasicMap &&bmap1, BasicMap &&bmap2) { return isl_basic_map_is_equal(bmap1.take(), bmap2.take()); }
  Map partialLexmax(BasicMap &&bmap, BasicSet &&dom, Set &empty);
  Map partialLexmin(BasicMap &&bmap, BasicSet &&dom, Set &empty);
  Map lexmin(BasicMap &&bmap);
  Map lexmax(BasicMap &&bmap);

  PwMultiAff partialLexminPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty);
  PwMultiAff partialLexmaxPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty);

  PwMultiAff lexminPwMultiAff(BasicMap &&bmap);
  PwMultiAff lexmaxPwMultiAff(BasicMap &&bmap);

  static inline BasicMap product(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_product(bmap1.take(), bmap2.take())); }
  static inline BasicMap domainProduct(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_domain_product(bmap1.take(), bmap2.take())); }
  static inline BasicMap rangeProduct(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_range_product(bmap1.take(), bmap2.take())); }
  static inline BasicMap flatProduct(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_flat_product(bmap1.take(), bmap2.take())); }
  static inline BasicMap flatRangeProduct(BasicMap &&bmap1, BasicMap &&bmap2) { return BasicMap::enwrap(isl_basic_map_flat_range_product(bmap1.take(), bmap2.take())); }
  static inline BasicSet deltas(BasicMap &&bmap) { return BasicSet::wrap(isl_basic_map_deltas(bmap.take())); }
  static inline BasicMap deltasMap(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_deltas_map(bmap.take())); }

  //static inline BasicMap add(BasicMap &&bmap) { return BasicMap::wrap(isl_basic_map_add(bmap.take())); }

  static inline bool isSubset(const BasicMap &bmap1, const BasicMap &bmap2) { return isl_basic_map_is_subset(bmap1.keep(), bmap2.keep()); }
  static inline bool isStrictSubset(const BasicMap &bmap1, const BasicMap &bmap2) { return isl_basic_map_is_strict_subset(bmap1.keep(), bmap2.keep()); }

  static inline BasicMap projectOut(BasicMap &&bmap, isl_dim_type type, unsigned first, unsigned n) { return BasicMap::enwrap(isl_basic_map_project_out(bmap.take(), type, first, n)); }
  static inline BasicMap removeDivs(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_remove_divs(bmap.take())); }
  static inline BasicMap removeDivsInvolvingDims(BasicMap &&bmap, isl_dim_type type, unsigned first, unsigned n) { return BasicMap::enwrap(isl_basic_map_remove_divs_involving_dims(bmap.take(), type, first, n)); }

  static inline BasicMap equate(BasicMap &&bmap, isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return BasicMap::enwrap(isl_basic_map_equate(bmap.take(), type1, pos1, type2, pos2)); }
  static inline BasicMap orderGe(BasicMap &&bmap, isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return BasicMap::enwrap(isl_basic_map_order_ge(bmap.take(), type1, pos1, type2, pos2)); }
  static inline BasicMap orderGt(BasicMap &&bmap, isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return BasicMap::enwrap(isl_basic_map_order_gt(bmap.take(), type1, pos1, type2, pos2)); }

  static inline BasicSet wrap(BasicMap &&bmap) { return BasicSet::wrap(isl_basic_map_wrap(bmap.take())); }
  static inline BasicMap flatten(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_flatten(bmap.take())); }
  static inline BasicMap flattenDomain(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_flatten_domain(bmap.take())); }
  static inline BasicMap flattenRange(BasicMap &&bmap) { return BasicMap::enwrap(isl_basic_map_flatten_range(bmap.take())); }

  static inline BasicMap zip(BasicMap &&bmap) { return enwrap(isl_basic_map_zip(bmap.take())); }
  static inline BasicMap curry(BasicMap &&bmap) { return enwrap(isl_basic_map_curry(bmap.take())); }
  static inline BasicMap uncurry(BasicMap &&bmap) { return enwrap(isl_basic_map_uncurry(bmap.take())); }

  Map computeDivs(BasicMap &&bmap);
  static inline BasicMap dropConstraintsInvolvingDims(BasicMap &&bmap, isl_dim_type type, unsigned first, unsigned n) { return enwrap(isl_basic_map_drop_constraints_involving_dims(bmap.take(), type, first, n)); }

  static inline BasicMap gist(BasicMap &&bmap, BasicMap &&context) { return enwrap(isl_basic_map_gist(bmap.take(), context.take())); }
  static inline BasicMap alignParams(BasicMap &&bmap, Space &&space) { return enwrap(isl_basic_map_align_params(bmap.take(), space.take())); }

  static inline BasicMap addConstraint(BasicMap &&bmap, Constraint &&constraint) { return BasicMap::enwrap(isl_basic_map_add_constraint(bmap.take(), constraint.take())); }
  static inline BasicMap addConstraint(BasicMap &&bmap, const Constraint &constraint) { return BasicMap::enwrap(isl_basic_map_add_constraint(bmap.take(), constraint.takeCopy())); }
  static inline BasicMap addConstraint(const BasicMap &bmap, Constraint &&constraint) { return BasicMap::enwrap(isl_basic_map_add_constraint(bmap.takeCopy(), constraint.take())); }
  static inline BasicMap addConstraint(const BasicMap &bmap, const Constraint &constraint) { return BasicMap::enwrap(isl_basic_map_add_constraint(bmap.takeCopy(), constraint.takeCopy())); }

} // namespace isl
#endif /* ISLPP_BASICMAP_H */
