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
//#include "Mat.h"
#include "Aff.h"
#include "MultiAff.h"
#include "PwAff.h"
#include "Tribool.h"
#include "Dim.h"
#include <llvm/Support/ErrorHandling.h>
#include "Ctx.h"
#include "Spacelike.h" // class Spacelike (base of Map)

struct isl_map;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class Space;
  class BasicMap;
  class id;
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

#define Map Map LLVM_FINAL
  class Map : public Spacelike {
#undef Map
#ifndef NDEBUG
    std::string _printed;
#endif

#pragma region Low-level
  private:
    isl_map *map;

  public: // Public because otherwise we had to add a lot of friends
    isl_map *take() { 
      assert(map); 
      isl_map *result = map; 
      map = nullptr; 
#ifndef NDEBUG
      _printed.clear();
#endif
      return result; 
    }

    isl_map *takeCopy() const;
    isl_map *keep() const { return map; }
  protected:
    void give(isl_map *map);

  public:
    static Map wrap(isl_map *map) { Map result; result.give(map);  return result; } //TODO: Rename to enwrap to avaoid name collision
#pragma endregion

  public:
    Map() : map(nullptr) {}
    Map(const Map &that) : map(nullptr) { give(that.takeCopy()); }
    Map(Map &&that) : map(nullptr) { give(that.take()); }
    virtual ~Map() { give(nullptr); }

    const Map &operator=(const Map &that) { give(that.takeCopy()); return *this; }
    const Map &operator=(Map &&that) { give(that.take()); return *this; }


#pragma region Conversion
    Map(const MultiAff &maff) : map(isl_map_from_multi_aff(maff.takeCopy())) {}
    Map(MultiAff &&maff) : map(isl_map_from_multi_aff(maff.take())) {}
#pragma endregion


#pragma region Creational
    static Map create(Ctx *ctx, unsigned nparam, unsigned in, unsigned out, int n, unsigned flags = 0) { return Map::wrap(isl_map_alloc(ctx->keep(), nparam, in, out, n, flags)); }
    static Map createUniverse(Space &&space) { return Map::wrap(isl_map_universe(space.take())); }
    static Map createUniverse(const Space &space) { return createUniverse(space.copy());  }
    static Map createNatUniverse(Space &&space) { return Map::wrap(isl_map_nat_universe(space.take())); }
    static Map createEmpty(Space &&space) { return Map::wrap(isl_map_empty(space.take())); }
    //static Map createEmptyLike(Map &&model) { return Map::wrap(isl_map_empty_like(model.take())); }
    //static Map createEmptyLike(BasicMap &&model) { return Map::wrap(isl_map_empty_like_basic_map(model.take())); }
    static Map createIdentity(Space &&space) { return Map::wrap(isl_map_identity(space.take())); }
    //static Map createIdentityLike(Map &&model) { return Map::wrap(isl_map_identity_like(model.take())); }
    //static Map createIdentityLike(Map &&model) { return Map::wrap(isl_map_identity_like_basic_map(model.take())); }
    static Map createLexLtFirst(Space &&dim, unsigned n) { return Map::wrap(isl_map_lex_lt_first(dim.take(), n)); }
    static Map createLexLeFirst(Space &&dim, unsigned n) { return Map::wrap(isl_map_lex_le_first(dim.take(), n)); }
    static Map createLexLt(Space &&dim) { return Map::wrap(isl_map_lex_lt(dim.take())); }
    static Map createLexLe(Space &&dim) { return Map::wrap(isl_map_lex_le(dim.take())); }
    static Map createLexGtFirst(Space &&dim, unsigned n) { return Map::wrap(isl_map_lex_gt_first(dim.take(), n)); }
    static Map createLexGeFirst(Space &&dim, unsigned n) { return Map::wrap(isl_map_lex_ge_first(dim.take(), n)); }
    static Map createLexGt(Space &&dim) { return Map::wrap(isl_map_lex_gt(dim.take())); }
    static Map createLexGe(Space &&dim) { return Map::wrap(isl_map_lex_ge(dim.take())); }
    static Map createIdentity(Set &&set) { return Map::wrap(isl_set_identity(set.take())); }

    static Map createFromBasicMap(BasicMap &&bmap) { return Map::wrap(isl_map_from_basic_map(bmap.take())); }
    static Map createFromDomain(Set &&set) { return Map::wrap(isl_map_from_domain(set.take())); }

    static Map createFromDomainAndRange(Set &&domain, Set &&range) { return Map::wrap(isl_map_from_domain_and_range(domain.take(), range.take())); }
    static Map createFromSet(Set &&set, Space &&dim) { return Map::wrap(isl_map_from_set(set.take(), dim.take())); }

    static Map fromAff(Aff &&aff) { return Map::wrap(isl_map_from_aff(aff.take())); }
    static Map fromMultiAff(MultiAff &&maff) { return Map::wrap(isl_map_from_multi_aff(maff.take())); }
    static Map fromPwMultiAff(PwMultiAff &&pwmaff) { return Map::wrap(isl_map_from_pw_multi_aff(pwmaff.take())); }
    static Map fromMultiPwAff(MultiPwAff &&mpaff);

    static Map readFrom(Ctx *ctx, const char *str);
    static Map readFrom(Ctx *ctx, FILE *input) { return Map::wrap(isl_map_read_from_file(ctx->keep(), input) ); }

    static Map createFromUnionMap(UnionMap &&umap);


    Map copy() const { return wrap(takeCopy()); }
    Map &&move() { return std::move(*this); }
#pragma endregion


#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
#pragma endregion

    Ctx *getCtx() const;
    Space getSpace() const { return Space::wrap(isl_map_get_space(keep())); }

    bool isEmpty() const;

    unsigned dim(isl_dim_type type) const { return isl_map_dim(keep(), type); }
    //unsigned dimParam() const { return isl_map_dim(keep(), isl_dim_param); }
    unsigned dimIn() const { return isl_map_dim(keep(), isl_dim_in); }
    unsigned dimOut() const { return isl_map_dim(keep(), isl_dim_out); }

    bool hasTupleName(isl_dim_type type) const { return isl_map_has_tuple_name(keep(), type); } 
    const char *getTupleName(isl_dim_type type) const { return isl_map_get_tuple_name(keep(), type); }
    void setTupleName(isl_dim_type type, const char *s) { give(isl_map_set_tuple_name(take(), type, s)); }

    bool hasTupleId(isl_dim_type type) const { return isl_map_has_tuple_id(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::wrap(isl_map_get_tuple_id(keep(), type)); }
    void setTupleId(isl_dim_type type, Id &&id) { give(isl_map_set_tuple_id(take(), type, id.take())); }

    bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_map_has_dim_name(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_map_get_dim_name(keep(), type, pos); }
    void setDimName(isl_dim_type type, unsigned pos, const char *s) { give(isl_map_set_dim_name(take(), type, pos, s)); }
    int findDimByName(isl_dim_type type, const char *name) const { return isl_map_find_dim_by_name(keep(), type, name); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_map_has_dim_id(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::wrap(isl_map_get_dim_id(keep(), type, pos)); }
    void setDimId(isl_dim_type type, unsigned pos, Id &&id) { give(isl_map_set_dim_id(take(), type, pos, id.take())); }
    int findDimById(isl_dim_type type, const Id &id) const { return isl_map_find_dim_by_id(keep(), type, id.keep()); }

    void removeRedundancies() { give (isl_map_remove_redundancies(take())); } 
    void neg() { give(isl_map_neg(take())); }
    void floordiv(const Int &d) { give(isl_map_floordiv(take(), d.keep())); }

    Set partialLexmax(Set &&dom) {
      isl_set *empty;
      give(isl_map_partial_lexmax(take(), dom.take(), &empty));
      return Set::wrap(empty);
    }
    Set partialLexmin(Set &&dom) {
      isl_set *empty;
      give(isl_map_partial_lexmin(take(), dom.take(), &empty));
      return Set::wrap(empty);
    }

    void lexmin() { give(isl_map_lexmin(take())); } 
    void lexmax() { give(isl_map_lexmax(take())); } 


    void addBasicMap(BasicMap &&bmap) { give(isl_map_add_basic_map(take(), bmap.take())); }

    /// reverse({ U -> V }) = { V -> U }
    Map reverse() const { wrap(isl_map_reverse(takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map reverse() && { wrap(isl_map_reverse(take())); }
#endif

    /// Function composition
    /// { U -> V }.applyRange({ X -> Y }) = { U -> {X->Y}(V) } => { U -> Y }
    Map applyRange(const Map &map2) const { return wrap(isl_map_apply_range(takeCopy(), map2.takeCopy())); }
    Map applyRange(Map &&map2) const { return wrap(isl_map_apply_range(takeCopy(), map2.take())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map applyRange(const Map &map2) && { return wrap(isl_map_apply_range(take(), map2.takeCopy())); }
    Map applyRange(Map &&map2) && { return wrap(isl_map_apply_range(take(), map2.take())); }
#endif

    void intersectDomain(Set &&set) { give(isl_map_intersect_domain(take(), set.take())); }
    void intersectRange(Set &&set) { give(isl_map_intersect_range(take(), set.take())); }

    void intersectParams(Set &&params) { give(isl_map_intersect_params(take(), params.take())); }

    Map substractDomain(Set &&dom) const { return wrap(isl_map_subtract_domain(takeCopy(), dom.take())); }
    Map substractDomain(const Set &dom) const { return wrap(isl_map_subtract_domain(takeCopy(), dom.takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Map substractDomain(Set &&dom) && { return wrap(isl_map_subtract_domain(take(), dom.take())); }
    Map substractDomain(const Set &dom) && { return wrap(isl_map_subtract_domain(take(), dom.takeCopy())); }
#endif

    void substractRange(Set &&dom) { give(isl_map_subtract_range(take(), dom.take())); }
    void complement() { give(isl_map_complement(take())); }

    Set getRange() const { return Set::wrap(isl_map_range(takeCopy())); }
    Set getDomain() const { return Set::wrap(isl_map_domain(takeCopy())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    Set getRange() && { return Set::wrap(isl_map_range(take())); }
    Set getDomain() && { return Set::wrap(isl_map_domain(take())); }
#endif


    void fix(isl_dim_type type, unsigned pos, const Int &value) { give(isl_map_fix(take(), type, pos, value.keep())); }
    void fix(isl_dim_type type, unsigned pos, int value) { give(isl_map_fix_si(take(), type, pos, value)); }
    void fix(const Dim &dim, const Int &value) {
      isl_dim_type type;
      unsigned pos;
      if(!findDim(dim, type, pos)) llvm_unreachable("Dim not found");
      give(isl_map_fix(take(), type, pos, value.keep()));
    }
    void fix(const Dim &dim, int value) {
      isl_dim_type type;
      unsigned pos;
      if(!findDim(dim, type, pos)) llvm_unreachable("Dim not found");
      give(isl_map_fix_si(take(), type, pos, value));
    }

    void lowerBound(isl_dim_type type, unsigned pos, int value) { give(isl_map_lower_bound_si(take(), type, pos, value));}
    void upperBound(isl_dim_type type, unsigned pos, int value) { give(isl_map_upper_bound_si(take(), type, pos, value));}

    void deltasMap() { give(isl_map_deltas_map(take())); }
    void detectEqualities() { give(isl_map_detect_equalities(take())); }

    void addDims(isl_dim_type type, unsigned n) { give(isl_map_add_dims(take(), type, n)); }

    void insertDims(isl_dim_type type, unsigned pos, unsigned n) { give(isl_map_insert_dims(take(), type, pos, n)); }
    void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { give(isl_map_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n)); }

    void projectOut( isl_dim_type type, unsigned first, unsigned n) { give(isl_map_project_out(take(), type, first, n)); }
    void removeUnknowsDivs() { give(isl_map_remove_unknown_divs(take())); } 
    void removeDivs() { give(isl_map_remove_divs(take())); } 
    void eliminate(isl_dim_type type, unsigned first, unsigned n) { give(isl_map_eliminate(take(), type, first, n)); }

    void removeDims(isl_dim_type type, unsigned first, unsigned n) { give (isl_map_remove_dims(take(), type, first, n)) ;} 
    void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) { give (isl_map_remove_divs_involving_dims(take(), type, first, n)) ;} 

    void equate(isl_dim_type type1, int pos1,  isl_dim_type type2, int pos2) { give(isl_map_equate(take(), type1, pos1, type2, pos2)); }
    void oppose(isl_dim_type type1, int pos1,  isl_dim_type type2, int pos2) { give(isl_map_oppose(take(), type1, pos1, type2, pos2)); }
    void orderLt(isl_dim_type type1, int pos1,  isl_dim_type type2, int pos2) { give(isl_map_order_lt(take(), type1, pos1, type2, pos2)); }
    void orderGt(isl_dim_type type1, int pos1,  isl_dim_type type2, int pos2) { give(isl_map_order_gt(take(), type1, pos1, type2, pos2)); }

    void flatten() { give(isl_map_flatten(take()));} 

    void domainMap() { give(isl_map_domain_map(take())); }
    void rangeMap() { give(isl_map_range_map(take())); }

    bool plainIsEmpty() const { return isl_map_plain_is_empty(keep()); }
    bool fastIsEmpty() const { return isl_map_fast_is_empty(keep()); }
    bool plainIsUniverse() const { return isl_map_plain_is_universe(keep()); }



    bool plainIsSingleValued() const { return isl_map_plain_is_single_valued(keep()); }
    bool isSingleValued() const { return isl_map_plain_is_single_valued(keep()); }

    bool plainIsInjective() const { return isl_map_plain_is_injective(keep()); }
    bool isInjective() const { return isl_map_is_injective(keep()); }
    bool isBijective() const { return isl_map_is_bijective(keep()); }
    bool isTranslation() const { return isl_map_is_translation(keep()); }

    bool canZip() const { return isl_map_can_zip(keep()); }
    void zip() {give(isl_map_zip(take()));}

    bool canCurry() const { return isl_map_can_curry(keep()); }
    void curry() { give(isl_map_curry(take())); }

    bool canUnurry() const { return isl_map_can_uncurry(keep()); }
    void uncurry() {give(isl_map_uncurry(take()));}

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

    void alignParams(Space &&model) { give(isl_map_align_params(take(), model.take())); }
  }; // class Map

  static inline Map enwrap(isl_map *obj) { return Map::wrap(obj); }

  static inline Map reverse(Map &&map) { return enwrap(isl_map_reverse(map.take())); }
  static inline Map reverse(Map &map) { return enwrap(isl_map_reverse(map.takeCopy())); }

  static inline Map intersectDomain(Map &&map, Set &&set) { return enwrap(isl_map_intersect_domain(map.take(), set.take())); }
  static inline Map intersectRange(Map &&map, Set &&set) { return enwrap(isl_map_intersect_range(map.take(), set.take())); }

  static  inline BasicMap simpleHull(Map &&map)  { return BasicMap::wrap(isl_map_simple_hull(map.take())); }
  static  inline BasicMap unshiftedSimpleHull(Map &&map)  { return BasicMap::wrap(isl_map_unshifted_simple_hull(map.take())); }
  static  inline Map sum(Map &&map1, Map &&map2) { return Map::wrap(isl_map_sum(map1.take(), map2.take())); }

  static  inline PwMultiAff lexminPwMultiAff(Map &&map) {return PwMultiAff::enwrap(isl_map_lexmin_pw_multi_aff(map.take())); } 
  static  inline PwMultiAff lexmaxPwMultiAff(Map &&map) { return PwMultiAff::enwrap(isl_map_lexmax_pw_multi_aff(map.take())); } 

  // "union" is a reserved word; "unite"?
  static inline Map union_(Map &&map1, Map &&map2) { return Map::wrap(isl_map_union(map1.take(), map2.take())); }
  static inline Map union_(Map &&map1, const Map &&map2) { return Map::wrap(isl_map_union(map1.take(), map2.takeCopy())); }
  static inline Map union_(const Map &map1, const Map &&map2) { return Map::wrap(isl_map_union(map1.takeCopy(), map2.takeCopy())); }
  static inline Map union_(const Map &map1, const Map &map2) { return Map::wrap(isl_map_union(map1.takeCopy(), map2.takeCopy())); }

  static  inline Map applyDomain(Map &&map1, Map &&map2) { return Map::wrap(isl_map_apply_domain(map1.take(), map2.take())); }
  static  inline Map applyRange(Map &&map1, Map &&map2) { return Map::wrap(isl_map_apply_range(map1.take(), map2.take())); }
  static   inline Map domainProduct(Map &&map1, Map &&map2) { return Map::wrap(isl_map_domain_product(map1.take(), map2.take())); }
  static   inline Map rangeProduct(Map &&map1, Map &&map2) { return Map::wrap(isl_map_range_product(map1.take(), map2.take())); }
  static  inline Map flatProduct(Map &&map1, Map &&map2) { return Map::wrap(isl_map_flat_product(map1.take(), map2.take())); }
  static  inline Map flatDomainProduct(Map &&map1, Map &&map2) { return Map::wrap(isl_map_flat_domain_product(map1.take(), map2.take())); }
  static  inline Map flatRangeProduct(Map &&map1, Map &&map2) { return Map::wrap(isl_map_flat_range_product(map1.take(), map2.take())); }
  static inline Map intersect(Map &&map1, Map &&map2) { return Map::wrap(isl_map_intersect(map1.take(), map2.take())); }
  static  inline Map substract(Map &&map1, Map &&map2) { return Map::wrap(isl_map_subtract(map1.take(), map2.take())); }

  static inline Set deltas(Map &&map) { return Set::wrap(isl_map_deltas(map.take())); }
  static inline BasicMap affineHull(Map &&map) { return BasicMap::wrap(isl_map_affine_hull(map.take()));}  
  static inline BasicMap convexHull(Map &&map) { return BasicMap::wrap(isl_map_convex_hull(map.take()));} 
  static inline BasicMap polyhedralHull(Map &&map) { return BasicMap::wrap(isl_map_polyhedral_hull(map.take()));} 

  static  inline Set wrap(Map &&map) { return Set::wrap(isl_map_wrap(map.take()));} 

  static inline Set params(Map &&map) { return Set::wrap(isl_map_params(map.take()));}
  static inline Set domain(Map &&map) { return Set::wrap(isl_map_domain(map.take()));}
  static  inline Set range(Map &&map) { return Set::wrap(isl_map_range(map.take()));}


  static  inline BasicMap sample(Map &&map) { return BasicMap::wrap(isl_map_sample(map.take())); }

  static  inline bool isSubset(const Map &map1, const Map &map2) { return isl_map_is_subset(map1.keep(), map2.keep()); }
  static  inline bool isStrictSubset(const Map &map1, const Map &map2) { return isl_map_is_strict_subset(map1.keep(), map2.keep()); }

  static   inline bool isDisjoint(const Map &map1, const Map &map2) { return isl_map_is_disjoint(map1.keep(), map2.keep()); }

  static  inline bool hasEqualSpace(const Map &map1, const Map &map2) { return isl_map_has_equal_space(map1.keep(), map2.keep()); }
  static  inline bool plainIsEqual(const Map &map1, const Map &map2) { return isl_map_plain_is_equal(map1.keep(), map2.keep()); }
  static  inline bool fastIsEqual(const Map &map1, const Map &map2) { return isl_map_fast_is_equal(map1.keep(), map2.keep()); }
  static  inline bool isEqual(const Map &map1, const Map &map2) { return isl_map_is_equal(map1.keep(), map2.keep()); }

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

  static inline Map lexLeMap(Map &&map1, Map &&map2) { return Map::wrap(isl_map_lex_le_map(map1.take(), map2.take())); } 
  static inline Map lexGeMap(Map &&map1, Map &&map2) { return Map::wrap(isl_map_lex_lt_map(map1.take(), map2.take())); } 
  static inline Map lexGtMap(Map &&map1, Map &&map2) { return Map::wrap(isl_map_lex_gt_map(map1.take(), map2.take())); } 
  static inline Map lexLtMap(Map &&map1, Map &&map2) { return Map::wrap(isl_map_lex_lt_map(map1.take(), map2.take())); } 

  static inline PwAff dimMax(Map &&map, int pos) { return PwAff::wrap(isl_map_dim_max(map.take(), pos)); }




} // namespace isl
#endif /* ISLPP_MAP_H */
