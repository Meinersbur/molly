#ifndef ISLPP_MAP_H
#define ISLPP_MAP_H

#include "islpp_common.h"
#include "Multi.h"
#include <cassert>
#include <string>

#include <isl/map.h>
#include "Space.h"
#include "Id.h"
#include "BasicMap.h"
#include "Set.h"
#include "PwMultiAff.h"
#include <functional>
#include "Aff.h"
#include "MultiAff.h"
#include "PwAff.h"
#include "Tribool.h"
#include "Dim.h"
#include <llvm/Support/ErrorHandling.h>
#include "Ctx.h"
#include "Spacelike.h" // class Spacelike (base of Map)
#include "Obj.h"
#include "MultiPwAff.h"
#include <llvm/ADT/ArrayRef.h>

struct isl_map;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class Space;
  class BasicMap;
  class Id;
  class Set;
  class Mat;
  class Aff;
} // namespace isl


namespace isl {

  enum class Accuracy {
    Exact,
    Fast,
    Plain,
    None
  };
  enum class Approximation {
    Exact,
    Over,
    Under,
    Rough
  };


  class Map : public Obj<Map,isl_map>, public Spacelike<Map> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_map_free(takeOrNull()); }
    StructTy *addref() const { return isl_map_copy(keepOrNull()); }

  public:
    Map() { }

    /* implicit */ Map(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Map(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_map_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_map_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_map_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_map_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_map_set_dim_id(take(), type, pos, id.take())); }

    // optional
    bool isSet() const { return false; }
    bool isMap() const { return true; }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_map_insert_dims(take(), type, pos, count)); }
    void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_map_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_map_remove_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_map_dim(keep(), type); }
    int findDimById(isl_dim_type type, const Id &id) const { return isl_map_find_dim_by_id(keep(), type, id.keep()); }

    bool hasTupleId(isl_dim_type type) const { return isl_map_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_map_get_tuple_name(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_map_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_map_set_tuple_name(take(), type, s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_map_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_map_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_map_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_map_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_map_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Conversion
    // from BasicMap
    Map(const BasicMap &bmap) : Obj(isl_map_from_basic_map(bmap.takeCopy())) {}
    Map(BasicMap &&bmap) : Obj(isl_map_from_basic_map(bmap.take())) {}
    const Map &operator=(const BasicMap &bmap) LLVM_LVALUE_FUNCTION { give(isl_map_from_basic_map(bmap.takeCopy())); return *this; }
    const Map &operator=(BasicMap &&bmap) LLVM_LVALUE_FUNCTION { give(isl_map_from_basic_map(bmap.take())); return *this; }

    // from MultiAff
    Map(const MultiAff &maff) : Obj(isl_map_from_multi_aff(maff.takeCopy())) {}
    Map(MultiAff &&maff) : Obj(isl_map_from_multi_aff(maff.take())) {}
    const Map &operator=(const MultiAff &maff) LLVM_LVALUE_FUNCTION { give(isl_map_from_multi_aff(maff.takeCopy())); return *this; }
    const Map &operator=(MultiAff &&maff) LLVM_LVALUE_FUNCTION { give(isl_map_from_multi_aff(maff.take())); return *this; }

    // from PwMultiAff
    Map(PwMultiAff &&pmaff) : Obj(isl_map_from_pw_multi_aff(pmaff.take())) {}
    Map(const PwMultiAff &pmaff) : Obj(isl_map_from_pw_multi_aff(pmaff.takeCopy())) {}
    const Map &operator=(PwMultiAff &&pmaff) LLVM_LVALUE_FUNCTION { give(isl_map_from_pw_multi_aff(pmaff.take())); return *this; }
    const Map &operator=(const PwMultiAff &pmaff) LLVM_LVALUE_FUNCTION { give(isl_map_from_pw_multi_aff(pmaff.takeCopy())); return *this; }

    // to PwMultiAff
    PwMultiAff toPwMultiAff() const { return PwMultiAff::enwrap(isl_pw_multi_aff_from_map(takeCopy())); } 

    // to MultiPwAff
    MultiPwAff toMultiPwAff() const;

    // to UnionMap
    UnionMap toUnionMap() const;
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    UnionMap toUnionMap() &&;
#endif
    //operator UnionMap() const;
    //operator UnionMap() &&;
#pragma endregion


#pragma region Creational
    static Map create(Ctx *ctx, unsigned nparam, unsigned in, unsigned out, int n, unsigned flags = 0) { return Map::enwrap(isl_map_alloc(ctx->keep(), nparam, in, out, n, flags)); }
    static Map createUniverse(Space &&space) { return Map::enwrap(isl_map_universe(space.take())); }
    static Map createUniverse(const Space &space) { return createUniverse(space.copy());  }
    static Map createNatUniverse(Space &&space) { return Map::enwrap(isl_map_nat_universe(space.take())); }
    static Map createEmpty(Space &&space) { return Map::enwrap(isl_map_empty(space.take())); }
    //static Map createEmptyLike(Map &&model) { return Map::wrap(isl_map_empty_like(model.take())); }
    //static Map createEmptyLike(BasicMap &&model) { return Map::wrap(isl_map_empty_like_basic_map(model.take())); }
    static Map createIdentity(Space &&space) { return Map::enwrap(isl_map_identity(space.take())); }
    //static Map createIdentityLike(Map &&model) { return Map::wrap(isl_map_identity_like(model.take())); }
    //static Map createIdentityLike(Map &&model) { return Map::wrap(isl_map_identity_like_basic_map(model.take())); }
    static Map createLexLtFirst(Space &&dim, unsigned n) { return Map::enwrap(isl_map_lex_lt_first(dim.take(), n)); }
    static Map createLexLeFirst(Space &&dim, unsigned n) { return Map::enwrap(isl_map_lex_le_first(dim.take(), n)); }
    static Map createLexLt(Space &&dim) { return Map::enwrap(isl_map_lex_lt(dim.take())); }
    static Map createLexLe(Space &&dim) { return Map::enwrap(isl_map_lex_le(dim.take())); }
    static Map createLexGtFirst(Space &&dim, unsigned n) { return Map::enwrap(isl_map_lex_gt_first(dim.take(), n)); }
    static Map createLexGeFirst(Space &&dim, unsigned n) { return Map::enwrap(isl_map_lex_ge_first(dim.take(), n)); }
    static Map createLexGt(Space &&dim) { return Map::enwrap(isl_map_lex_gt(dim.take())); }
    static Map createLexGe(Space &&dim) { return Map::enwrap(isl_map_lex_ge(dim.take())); }
    static Map createIdentity(Set &&set) { return Map::enwrap(isl_set_identity(set.take())); }

    static Map createFromBasicMap(BasicMap &&bmap) { return Map::enwrap(isl_map_from_basic_map(bmap.take())); }
    static Map createFromDomain(Set &&set) { return Map::enwrap(isl_map_from_domain(set.take())); }
    static Map createFromRange(Set &&set) { return Map::enwrap(isl_map_from_range(set.take())); }

    static Map createFromDomainAndRange(Set &&domain, Set &&range) { return Map::enwrap(isl_map_from_domain_and_range(domain.take(), range.take())); }
    static Map createFromSet(Set &&set, Space &&dim) { return Map::enwrap(isl_map_from_set(set.take(), dim.take())); }

    static Map fromAff(Aff &&aff) { return Map::enwrap(isl_map_from_aff(aff.take())); }
    static Map fromMultiAff(MultiAff &&maff) { return Map::enwrap(isl_map_from_multi_aff(maff.take())); }
    static Map fromPwMultiAff(PwMultiAff &&pwmaff) { return Map::enwrap(isl_map_from_pw_multi_aff(pwmaff.take())); }
    static Map fromMultiPwAff(MultiPwAff &&mpaff);

    static Map readFrom(Ctx *ctx, const char *str);
    static Map readFrom(Ctx *ctx, FILE *input) { return Map::enwrap(isl_map_read_from_file(ctx->keep(), input) ); }

    static Map createFromUnionMap(UnionMap &&umap);
#pragma endregion


    //Space getSpace() const { return Space::wrap(isl_map_get_space(keep())); }
    //Space getSpacelike() const { return getSpace(); }
    Space getDomainSpace() const { return Space::enwrap(isl_space_domain(isl_map_get_space(keep()))); }
    Space getRangeSpace() const { return Space::enwrap(isl_space_range(isl_map_get_space(keep()))); }


    bool isEmpty() const { return checkBool(isl_map_is_empty(keep())); }


    void removeRedundancies() { give (isl_map_remove_redundancies(take())); } 
    void neg() { give(isl_map_neg(take())); }
    void floordiv(const Int &d) { give(isl_map_floordiv(take(), d.keep())); }

    Set partialLexmax(Set &&dom) {
      isl_set *empty;
      give(isl_map_partial_lexmax(take(), dom.take(), &empty));
      return Set::enwrap(empty);
    }
    Set partialLexmin(Set &&dom) {
      isl_set *empty;
      give(isl_map_partial_lexmin(take(), dom.take(), &empty));
      return Set::enwrap(empty);
    }

    void lexmin_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_map_lexmin(take())); } 
    Map lexmin() const { return Map::enwrap(isl_map_lexmin(takeCopy())); }
      PwMultiAff lexminPwMultiAff() const { return PwMultiAff::enwrap(isl_map_lexmin_pw_multi_aff(takeCopy())); }

    void lexmax_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_map_lexmax(take())); } 
    Map lexmax() const { return Map::enwrap(isl_map_lexmax(takeCopy())); }
    PwMultiAff lexmaxPwMultiAff() const { return PwMultiAff::enwrap(isl_map_lexmin_pw_multi_aff(takeCopy())); }

#if 0
    // This is internal, use unite() instead
    void addBasicMap_inplace(BasicMap &&bmap) ISLPP_INPLACE_QUALIFIER { give(isl_map_add_basic_map(take(), bmap.take())); }
    void addBasicMap_inplace(const BasicMap &bmap) ISLPP_INPLACE_QUALIFIER { give(isl_map_add_basic_map(take(), bmap.takeCopy())); }
    Map addBasicMap(BasicMap &&bmap) const { return Map::enwrap(isl_map_add_basic_map(takeCopy(), bmap.take())); }
    Map addBasicMap(const BasicMap &bmap) const { return Map::enwrap(isl_map_add_basic_map(takeCopy(), bmap.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map addBasicMap(BasicMap &&bmap) && { return Map::enwrap(isl_map_add_basic_map(take(), bmap.take())); }
    Map addBasicMap(const BasicMap &bmap) && { return Map::enwrap(isl_map_add_basic_map(take(), bmap.takeCopy())); }  
#endif
#endif

    /// reverse({ U -> V }) = { V -> U }
    void reverse_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_map_reverse(take())); }
    Map reverse() const { return Map::enwrap(isl_map_reverse(takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map reverse() && { wrap(isl_map_reverse(take())); }
#endif


    void applyDomain_inplace(const Map &map2) ISLPP_INPLACE_QUALIFIER { give(isl_map_apply_domain(take(), map2.takeCopy())); }
    void applyDomain_inplace(Map &&map2) ISLPP_INPLACE_QUALIFIER { give(isl_map_apply_domain(take(), map2.take())); }
    Map applyDomain(const Map &map2) const { return Map::enwrap(isl_map_apply_domain(takeCopy(), map2.takeCopy())); }
    Map applyDomain(Map &&map2) const { return Map::enwrap(isl_map_apply_domain(takeCopy(), map2.take())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map applyDomain(const Map &map2) && { return Map::wrap(isl_map_apply_domain(take(), map2.takeCopy())); }
    Map applyDomain(Map &&map2) && { return Map::wrap(isl_map_apply_domain(take(), map2.take())); }
#endif


    /// Function composition
    /// { U -> V }.applyRange({ X -> Y }) = { U -> {X->Y}(V) } => { U -> Y }
    void applyRange_inplace(const Map &map2) ISLPP_INPLACE_QUALIFIER { give(isl_map_apply_range(take(), map2.takeCopy())); }
    void applyRange_inplace(Map &&map2) ISLPP_INPLACE_QUALIFIER { give(isl_map_apply_range(take(), map2.take())); }
    Map applyRange(const Map &map2) const { return Map::enwrap(isl_map_apply_range(takeCopy(), map2.takeCopy())); }
    Map applyRange(Map &&map2) const { return Map::enwrap(isl_map_apply_range(takeCopy(), map2.take())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map applyRange(const Map &map2) && { return Map::wrap(isl_map_apply_range(take(), map2.takeCopy())); }
    Map applyRange(Map &&map2) && { return Map::wrap(isl_map_apply_range(take(), map2.take())); }
#endif


    void intersect_inplace(Map &&that) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect(take(), that.take())); }
    void intersect_inplace(const Map &that) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect(take(), that.takeCopy())); }
    Map intersect(Map &&that) const { return Map::enwrap(isl_map_intersect(takeCopy(), that.take())); } 
    Map intersect(const Map &that) const { return Map::enwrap(isl_map_intersect(takeCopy(), that.takeCopy())); } 
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map intersect(Map &&that) && { return Map::enwrap(isl_map_intersect(take(), that.take())); } 
    Map intersect(const Map &that) && { return Map::enwrap(isl_map_intersect(take(), that.takeCopy())); }
#endif

    void intersectDomain_inplace(Set &&set) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect_domain(take(), set.take())); }
    void intersectDomain_inplace(const Set &set) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect_domain(take(), set.takeCopy())); }
    Map intersectDomain(Set &&set) const { return Map::enwrap(isl_map_intersect_domain(takeCopy(), set.take())); }
    Map intersectDomain(const Set &set) const { return Map::enwrap(isl_map_intersect_domain(takeCopy(), set.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map intersectDomain(Set &&set)&& { return Map::enwrap(isl_map_intersect_domain(take(), set.take())); }
    Map intersectDomain(const Set &set) &&{ return Map::enwrap(isl_map_intersect_domain(take(), set.takeCopy())); }
#endif

    void intersectRange_inplace(Set &&set) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect_range(take(), set.take())); }
    void intersectRange_inplace(const Set &set) ISLPP_INPLACE_QUALIFIER { give(isl_map_intersect_range(take(), set.takeCopy())); }
    Map intersectRange(Set &&set) { return Map::enwrap(isl_map_intersect_range(take(), set.take())); }
    Map intersectRange(const Set &set) { return Map::enwrap(isl_map_intersect_range(take(), set.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map intersectRange(Set &&set)&& { return Map::enwrap(isl_map_intersect_range(take(), set.take())); }
    Map intersectRange(const Set &set) &&{ return Map::enwrap(isl_map_intersect_range(take(), set.takeCopy())); }
#endif

    void intersectParams(Set &&params) { give(isl_map_intersect_params(take(), params.take())); }

    Map subtractDomain(Set &&dom) const { return Map::enwrap(isl_map_subtract_domain(takeCopy(), dom.take())); }
    Map subtractDomain(const Set &dom) const { return Map::enwrap(isl_map_subtract_domain(takeCopy(), dom.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map subtractDomain(Set &&dom) && { return Map::wrap(isl_map_subtract_domain(take(), dom.take())); }
    Map subtractDomain(const Set &dom) && { return Map::enwrap(isl_map_subtract_domain(take(), dom.takeCopy())); }
#endif

    void subtract_inplace(const Map &map) ISLPP_INPLACE_QUALIFIER { return give(isl_map_subtract(take(), map.takeCopy())); }
    Map subtract(const Map &map) const { return Map::enwrap(isl_map_subtract(takeCopy(), map.takeCopy())); }

    void substractRange(Set &&dom) { give(isl_map_subtract_range(take(), dom.take())); }
    void complement() { give(isl_map_complement(take())); }

    Set getRange() const { return Set::enwrap(isl_map_range(takeCopy())); }
    Set getDomain() const { return Set::enwrap(isl_map_domain(takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Set getRange() && { return Set::enwrap(isl_map_range(take())); }
    Set getDomain() && { return Set::enwrap(isl_map_domain(take())); }
#endif


    void fix_inplace(isl_dim_type type, unsigned pos, const Int &value) ISLPP_INPLACE_QUALIFIER { give(isl_map_fix(take(), type, pos, value.keep())); }
    void fix_inplace(isl_dim_type type, unsigned pos, int value) ISLPP_INPLACE_QUALIFIER { give(isl_map_fix_si(take(), type, pos, value)); }
    void fix_inplace(const Dim &dim, const Int &value) ISLPP_INPLACE_QUALIFIER {
      isl_dim_type type;
      unsigned pos;
      if(!findDim(dim, type, pos)) llvm_unreachable("Dim not found");
      give(isl_map_fix(take(), type, pos, value.keep()));
    }
    void fix_inplace(const Dim &dim, int value) ISLPP_INPLACE_QUALIFIER {
      isl_dim_type type;
      unsigned pos;
      if(!findDim(dim, type, pos)) llvm_unreachable("Dim not found");
      give(isl_map_fix_si(take(), type, pos, value));
    }

    void lowerBound(isl_dim_type type, unsigned pos, int value) { give(isl_map_lower_bound_si(take(), type, pos, value)); }
    void upperBound(isl_dim_type type, unsigned pos, int value) { give(isl_map_upper_bound_si(take(), type, pos, value)); }

    void deltasMap() { give(isl_map_deltas_map(take())); }
    void detectEqualities() { give(isl_map_detect_equalities(take())); }


    void projectOut( isl_dim_type type, unsigned first, unsigned n) { give(isl_map_project_out(take(), type, first, n)); }
    void removeUnknowsDivs() { give(isl_map_remove_unknown_divs(take())); } 
    void removeDivs() { give(isl_map_remove_divs(take())); } 
    void eliminate(isl_dim_type type, unsigned first, unsigned n) { give(isl_map_eliminate(take(), type, first, n)); }

    void removeDims(isl_dim_type type, unsigned first, unsigned n) { give (isl_map_remove_dims(take(), type, first, n)) ;} 
    void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) { give (isl_map_remove_divs_involving_dims(take(), type, first, n)) ;} 

    void equate_inplace(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) ISLPP_INPLACE_QUALIFIER { give(isl_map_equate(take(), type1, pos1, type2, pos2)); }
    Map equate(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) const { return Map::enwrap(isl_map_equate(takeCopy(), type1, pos1, type2, pos2)); }
    Map oppose(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return Map::enwrap(isl_map_oppose(take(), type1, pos1, type2, pos2)); }
    Map orderLt(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return Map::enwrap(isl_map_order_lt(take(), type1, pos1, type2, pos2)); }
    Map orderGt(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) { return Map::enwrap(isl_map_order_gt(take(), type1, pos1, type2, pos2)); }

    void flatten_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_map_flatten(take()));} 
    Map flatten() const { return Map::enwrap(isl_map_flatten(takeCopy()));} 

    Map domainMap() const { return Map::enwrap(isl_map_domain_map(takeCopy())); }
    Map rangeMap() const { return Map::enwrap(isl_map_range_map(takeCopy())); }

    bool plainIsEmpty() const { return isl_map_plain_is_empty(keep()); }
    bool fastIsEmpty() const { return isl_map_fast_is_empty(keep()); }
    bool plainIsUniverse() const { return isl_map_plain_is_universe(keep()); }


    bool plainIsSingleValued() const { return isl_map_plain_is_single_valued(keep()); }
    bool isSingleValued() const { return isl_map_plain_is_single_valued(keep()); }

    bool plainIsInjective() const { return checkBool(isl_map_plain_is_injective(keep())); }
    bool isInjective() const { return checkBool( isl_map_is_injective(keep())); }
    bool isBijective() const { return checkBool(isl_map_is_bijective(keep())); }
    bool isTranslation() const { return checkBool(isl_map_is_translation(keep())); }

    bool canZip() const { return checkBool(isl_map_can_zip(keep())); }
    void zip_inplace() ISLPP_INPLACE_QUALIFIER {give(isl_map_zip(take()));}
    Map zip() const { return Map::enwrap(isl_map_zip(takeCopy()));}

    bool canCurry() const { return checkBool(isl_map_can_curry(keep())); }
    void curry_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_map_curry(take())); }
    Map curry() const { return Map::enwrap(isl_map_curry(takeCopy())); }

    bool canUnurry() const { return isl_map_can_uncurry(keep()); }
    void uncurry_inplace() ISLPP_INPLACE_QUALIFIER{ give(isl_map_uncurry(take()));}
    Map uncurry() const { return Map::enwrap(isl_map_uncurry(takeCopy()));  }

    void makeDisjoint() { give(isl_map_make_disjoint(take()));}

    void computeDivs() { give(isl_map_compute_divs(take()));}
    void alignDivs() { give(isl_map_align_divs(take()));}

    void dropConstraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) { give(isl_map_drop_constraints_involving_dims(take(), type, first, n));  }
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const { return isl_map_involves_dims(keep(), type, first, n);  }

    bool plainInputIsFixed(unsigned in, Int &val) const { return isl_map_plain_input_is_fixed(keep(), in, val.change()); } 
    bool plainIsFixed(isl_dim_type type, unsigned pos, Int &val) const { return isl_map_plain_is_fixed(keep(), type, pos, val.change()); } 
    bool fastIsFixed(isl_dim_type type, unsigned pos, Int &val) const { return isl_map_fast_is_fixed(keep(), type, pos, val.change()); } 

    /// Simplify this map by removing redundant constraint that are implied by context (assuming that the points of this map that are outside of context are irrelevant)
    void gist(Map &&context) { give(isl_map_gist(take(), context.take())); }
    void gist(BasicMap &&context) { give(isl_map_gist_basic_map(take(), context.take())); }
    void gistDomain(Set &&context) { give(isl_map_gist_domain(take(), context.take())); }
    void gistRange(Set &&context) { give(isl_map_gist_range(take(), context.take())); }
    void gistParams(Set &&context) { give(isl_map_gist_params(take(), context.take())); }

    void coalesce() { give(isl_map_coalesce(take())); } 
    uint32_t getHash() const { return isl_map_get_hash(keep()); } 

    bool foreachBasicMap(std::function<bool(BasicMap&&)> func) const;
    std::vector<BasicMap> getBasicMaps() const;

    void fixedPower(const Int &exp) { give(isl_map_fixed_power(take(), exp.keep())); } 
    void power(const Int &exp, bool &exact) {
      int exact_;
      give(isl_map_power(take(), &exact_)); 
      exact = exact_;
    } 
    Approximation power(const Int &exp) {
      int exact;
      give(isl_map_power(take(), &exact));
      return exact ? Approximation::Exact : Approximation::Over;
    } 

    void reachingPathLengths(bool &exact) { 
      int tmp;
      give(isl_map_reaching_path_lengths(take(), &tmp));
      exact = tmp;
    }
    Approximation reachingPathLengths() { 
      int exact;
      give(isl_map_reaching_path_lengths(take(), &exact));
      return exact ? Approximation::Exact : Approximation::Over;
    }

    void transitiveClosure(bool &exact) {
      int tmp;
      give(isl_map_transitive_closure(take(), &tmp));
      exact = tmp;
    }
    Approximation transitiveClosure() { 
      int exact;
      give(isl_map_transitive_closure(take(), &exact));
      return exact ? Approximation::Exact : Approximation::Over;
    }

    void alignParams_inplace(Space &&model) ISLPP_INPLACE_QUALIFIER { give(isl_map_align_params(take(), model.take())); }
    void alignParams_inplace(const Space &model) ISLPP_INPLACE_QUALIFIER { give(isl_map_align_params(take(), model.takeCopy())); }
    Map alignParams(Space &&model) const { return Map::enwrap(isl_map_align_params(takeCopy(), model.take())); }
       Map alignParams(const Space &model) const { return Map::enwrap(isl_map_align_params(takeCopy(), model.takeCopy())); }

    PwAff dimMax_consume(unsigned pos) { return PwAff::enwrap(isl_map_dim_max(take(), pos)); }
    PwAff dimMax(unsigned pos) const { return PwAff::enwrap(isl_map_dim_max(takeCopy(), pos)); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    PwAff dimMax(unsigned pos) && { return PwAff::enwrap(isl_map_dim_max(take(), pos)); }
#endif

    PwAff dimMin_consume(unsigned pos) { return PwAff::enwrap(isl_map_dim_min(take(), pos)); }
    PwAff dimMin(unsigned pos) const { return PwAff::enwrap(isl_map_dim_min(takeCopy(), pos)); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    PwAff dimMin(unsigned pos) && { return PwAff::enwrap(isl_map_dim_min(take(), pos)); }
#endif

    bool isSubsetOf(const Map &map) const { return isl_map_is_subset(keep(), map.keep()); }
    bool isSupersetOf(const Map &map) const { return isl_map_is_subset(map.keep(), keep()); }

    /// Add the value of aff to all the range values of the corresponding map elements
    void sum_inplace(const Map &map) ISLPP_INPLACE_QUALIFIER { give(isl_map_sum(take(), map.takeCopy() )); }
    Map sum(const Map &map) const { return Map::enwrap(isl_map_sum(takeCopy(), map.takeCopy() )); }

    /// { A -> B } and { B' -> C } to { (A -> B*B') -> C }
    /// Function composition
    /// similar to apply() function, but returns nested domain
    /// Use curry() to get { A -> (B -> C) }
    Map chain(const Map &that) const { // rename: (inner) join?
      auto result = this->rangeMap(); // { (A -> B) -> B }
      result.applyRange_inplace(that);
      return result;
    }

    Map chainNested(const Map &map) const;
    Map chainNested(const Map &map, unsigned tuplePos) const;

    /// { A -> B } to { (A -> B) } (with nested range)
    Set wrap() const { return Set::enwrap(isl_map_wrap(takeCopy()));} 

    /// Overwrite the space information (tuple ids, dim ids, space nesting); Number of dimensions must be the same, separately for isl_dim_in and isl_dim_out
    /// param dimensions are not changes, these are aligned automatically.
    void cast_inplace(const Space &space) ISLPP_INPLACE_QUALIFIER { give(isl_map_cast(take(), space.takeCopy())); }
    Map cast(const Space &space) const { return Map::enwrap(isl_map_cast(takeCopy(), space.takeCopy())); }
  }; // class Map


  static inline Map enwrap(__isl_take isl_map *obj) { return Map::enwrap(obj); }
  static inline Map enwrapCopy(__isl_keep isl_map *obj) { return Map::enwrapCopy(obj); }

  static inline Map reverse(Map &&map) { return enwrap(isl_map_reverse(map.take())); }
  static inline Map reverse(const Map &map) { return enwrap(isl_map_reverse(map.takeCopy())); }

  static inline Map intersectDomain(Map &&map, Set &&set) { return enwrap(isl_map_intersect_domain(map.take(), set.take())); }
  static inline Map intersectRange(Map &&map, Set &&set) { return enwrap(isl_map_intersect_range(map.take(), set.take())); }

  static inline BasicMap simpleHull(Map &&map) { return BasicMap::enwrap(isl_map_simple_hull(map.take())); }
  static inline BasicMap unshiftedSimpleHull(Map &&map) { return BasicMap::enwrap(isl_map_unshifted_simple_hull(map.take())); }
  static inline Map sum(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_sum(map1.take(), map2.take())); }

  static inline PwMultiAff lexminPwMultiAff(Map &&map) {return PwMultiAff::enwrap(isl_map_lexmin_pw_multi_aff(map.take())); } 
  static inline PwMultiAff lexmaxPwMultiAff(Map &&map) { return PwMultiAff::enwrap(isl_map_lexmax_pw_multi_aff(map.take())); } 

  // "union" is a reserved word
  static inline Map unite(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_union(map1.take(), map2.take())); }
  static inline Map unite(Map &&map1, const Map &map2) { return Map::enwrap(isl_map_union(map1.take(), map2.takeCopy())); }
  static inline Map unite(const Map &map1, Map &&map2) { return Map::enwrap(isl_map_union(map1.takeCopy(), map2.take())); }
  static inline Map unite(const Map &map1, const Map &map2) { return Map::enwrap(isl_map_union(map1.takeCopy(), map2.takeCopy())); }

  static inline Map applyDomain(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_apply_domain(map1.take(), map2.take())); }
  static inline Map applyRange(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_apply_range(map1.take(), map2.take())); }
  static inline Map domainProduct(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_domain_product(map1.take(), map2.take())); }
  static inline Map rangeProduct(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_range_product(map1.take(), map2.take())); }
  static inline Map flatProduct(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_flat_product(map1.take(), map2.take())); }
  static inline Map flatDomainProduct(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_flat_domain_product(map1.take(), map2.take())); }
  static inline Map flatRangeProduct(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_flat_range_product(map1.take(), map2.take())); }

  static inline Map intersect(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_intersect(map1.take(), map2.take())); }
  static inline Map intersect(Map &&map1, const Map &map2) { return Map::enwrap(isl_map_intersect(map1.take(), map2.takeCopy())); }
  static inline Map intersect(const Map &map1, Map &&map2) { return Map::enwrap(isl_map_intersect(map1.takeCopy(), map2.take())); }
  static inline Map intersect(const Map &map1, const Map &map2) { return Map::enwrap(isl_map_intersect(map1.takeCopy(), map2.takeCopy())); }

  static inline Map substract(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_subtract(map1.take(), map2.take())); }

  static inline Set deltas(Map &&map) { return Set::enwrap(isl_map_deltas(map.take())); }
  static inline BasicMap affineHull(Map &&map) { return BasicMap::enwrap(isl_map_affine_hull(map.take()));}  
  static inline BasicMap convexHull(Map &&map) { return BasicMap::enwrap(isl_map_convex_hull(map.take()));} 
  static inline BasicMap polyhedralHull(Map &&map) { return BasicMap::enwrap(isl_map_polyhedral_hull(map.take()));} 

  static inline Set wrap(Map &&map) { return Set::enwrap(isl_map_wrap(map.take()));} 

  static inline Set params(Map &&map) { return Set::enwrap(isl_map_params(map.take()));}
  static inline Set domain(Map &&map) { return Set::enwrap(isl_map_domain(map.take()));}
  static inline Set range(Map &&map) { return Set::enwrap(isl_map_range(map.take()));}


  static inline BasicMap sample(Map &&map) { return BasicMap::enwrap(isl_map_sample(map.take())); }

  static inline bool isSubset(const Map &map1, const Map &map2) { return isl_map_is_subset(map1.keep(), map2.keep()); }
  static inline bool isStrictSubset(const Map &map1, const Map &map2) { return isl_map_is_strict_subset(map1.keep(), map2.keep()); }

  static inline bool isDisjoint(const Map &map1, const Map &map2) { return isl_map_is_disjoint(map1.keep(), map2.keep()); }

  static inline bool hasEqualSpace(const Map &map1, const Map &map2) { return isl_map_has_equal_space(map1.keep(), map2.keep()); }
  static inline bool plainIsEqual(const Map &map1, const Map &map2) { return isl_map_plain_is_equal(map1.keep(), map2.keep()); }
  static inline bool fastIsEqual(const Map &map1, const Map &map2) { return isl_map_fast_is_equal(map1.keep(), map2.keep()); }
  static inline bool isEqual(const Map &map1, const Map &map2) { return isl_map_is_equal(map1.keep(), map2.keep()); }

  static inline Tribool isEqual(const Map &map1, const Map &map2, Accuracy accuracy) {
    switch (accuracy) {
    case Accuracy::Exact:
      return isl_map_is_equal(map1.keep(), map2.keep());
    case Accuracy::Fast:
      return isl_map_fast_is_equal(map1.keep(), map2.keep()) ? Tribool::True : Tribool::Indeterminate;
    case Accuracy::Plain:
      return isl_map_plain_is_equal(map1.keep(), map2.keep()) ? Tribool::True : Tribool::Indeterminate;
    case Accuracy::None:
      return Tribool::Indeterminate;
    }
  }

  static inline Map lexLeMap(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_lex_le_map(map1.take(), map2.take())); } 
  static inline Map lexGeMap(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_lex_lt_map(map1.take(), map2.take())); } 
  static inline Map lexGtMap(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_lex_gt_map(map1.take(), map2.take())); } 
  static inline Map lexLtMap(Map &&map1, Map &&map2) { return Map::enwrap(isl_map_lex_lt_map(map1.take(), map2.take())); } 

  static inline PwAff dimMax(Map &&map, int pos) { return PwAff::enwrap(isl_map_dim_max(map.take(), pos)); }

  /// Create a cartesian product with a subset of dimensions are equated
  /// e.g.
  /// join({ (A -> B1) }, { (B2 -> C) }) = { (A -> B1) -> (B2 -> C) | B1==B2 }
  Map join(const Set &domain, const Set &range, unsigned firstDomainDim, unsigned firstRangeDim, unsigned countEquate);

  /// Select the dimensions to equate itself, that is all nested tuples and dims with same id 
  Map naturalJoin(const Set &domain, const Set &range);

  // Note these are NOT total orders
  static inline bool operator<=(const Map &map1, const Map &map2) { return checkBool(isl_map_is_subset(map1.keep(), map2.keep())); }
   static inline bool operator<(const Map &map1, const Map &map2) { return checkBool(isl_map_is_strict_subset(map1.keep(), map2.keep())); }
   static inline bool operator>=(const Map &map1, const Map &map2) { return checkBool(isl_map_is_subset(map2.keep(), map1.keep())); }
   static inline bool operator>(const Map &map1, const Map &map2) { return checkBool(isl_map_is_strict_subset(map2.keep(), map1.keep())); }

} // namespace isl
#endif /* ISLPP_MAP_H */
