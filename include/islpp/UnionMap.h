#ifndef ISLPP_UNIONMAP_H
#define ISLPP_UNIONMAP_H

#include <cassert>
#include <llvm/Support/raw_ostream.h>
#include <isl/union_map.h>
#include "BasicMap.h"
#include "Map.h"
#include "Space.h"
#include "Ctx.h"
#include "Spacelike.h"
#include "Set.h"
#include "UnionSet.h"
#include "Int.h"
#include <functional>
#include "Obj.h"
#include "Union.h"
#include <isl/flow.h>
#include <vector>
#include <isl/deprecated/union_map_int.h>

struct isl_union_map;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  /// Contains a Map per Space
  template<>
  class Union<Map> : public/*because otherwise we had to make a lot of friends*/ isl::Obj<Union<Map>, struct isl_union_map> {
    friend class isl::Obj<UnionMap, struct isl_union_map>;
  public:
    //typedef struct isl_union_map StructTy;
    //typedef Union<Map> ObjTy;
    typedef Map EltTy;
    typedef Union<Map> UnionTy;

#pragma region Low-level
  private:
    void release() { isl_union_map_free(keepOrNull()); }
    StructTy *addref() const { return isl_union_map_copy(keep()); }

  public:
    Union() {}

    /* implicit */ Union(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Union(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    static UnionTy enwrap(StructTy *map) { ObjTy result; result.give(map); return result; }
#pragma endregion


#pragma region Conversion
    /* implicit */ Union(Map &&that) : Obj(isl_union_map_from_map(that.take())) { }
    /* implicit */ Union(const Map &that) : Obj(isl_union_map_from_map(that.takeCopy())) { }
    const UnionMap &operator=(Map &&that) { reset(isl_union_map_from_map(that.take())); return *this; }
    const UnionMap &operator=(const Map &that) { reset(isl_union_map_from_map(that.takeCopy())); return *this; }
#pragma endregion


#pragma region Creational
    static UnionMap createFrom(BasicMap &&bmap) { return UnionMap::enwrap(isl_union_map_from_basic_map(bmap.take())); }
    static UnionMap createFrom(Map &&map) { return UnionMap::enwrap(isl_union_map_from_map(map.take())); }
    static UnionMap createEmpty(Space &&space) { return UnionMap::enwrap(isl_union_map_empty(space.take())); } 

    static UnionMap createFromDomain(UnionSet &&uset) {return UnionMap::enwrap(isl_union_map_from_domain(uset.take())); } 
    static UnionMap createFromRange(UnionSet &&uset) {return UnionMap::enwrap(isl_union_map_from_range(uset.take())); } 

    static UnionMap createFromDomainAndRange(UnionSet &&domain, UnionSet &&range) { return UnionMap::enwrap(isl_union_map_from_domain_and_range(domain.take(), range.take())); }
    static UnionMap createFromDomainAndRange(UnionSet &&domain, const UnionSet &range) { return UnionMap::enwrap(isl_union_map_from_domain_and_range(domain.take(), range.takeCopy())); }
    static UnionMap createFromDomainAndRange(const UnionSet &domain, UnionSet &&range) { return UnionMap::enwrap(isl_union_map_from_domain_and_range(domain.takeCopy(), range.take())); }
    static UnionMap createFromDomainAndRange(const UnionSet &domain, const UnionSet &range) { return UnionMap::enwrap(isl_union_map_from_domain_and_range(domain.takeCopy(), range.takeCopy())); }

    static UnionMap createIdentity(UnionSet &&uset) { return UnionMap::enwrap(isl_union_set_identity(uset.take())); }

    static UnionMap readFromFile(Ctx *ctx, 	FILE *input) { return UnionMap::enwrap(isl_union_map_read_from_file(ctx->keep(), input)); }
    static UnionMap readFromStr(Ctx *ctx, const char *str) { return UnionMap::enwrap(isl_union_map_read_from_str(ctx->keep(), str)); }

    UnionMap copy() { return UnionMap::enwrap(takeCopy()); }
#pragma endregion


#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    void dump() const;
#pragma endregion


    Ctx *getCtx() const { return Ctx::enwrap(isl_union_map_get_ctx(keep())); }
    Space getSpace() const { return Space::enwrap(isl_union_map_get_space(keep())); }

    void universe() { give(isl_union_map_universe(take())); } // ???
    void domainMap() { give(isl_union_map_domain_map(take())); } // ???
    void rangeMap() { give(isl_union_map_range_map(take())); } // Create a union_map that maps each of its maps to their rangs (a collection of mappings from maps to sets; before: [ domain -> range ] after: [ [ domain -> range ] -> range ] )

    void affineHull() { give(isl_union_map_affine_hull(take())); }
    void polyhedralHull()  { give(isl_union_map_polyhedral_hull(take())); }
    void simpleHull()  { give(isl_union_map_simple_hull(take())); }
    void coalesce()  { give(isl_union_map_coalesce(take())); }

    void computeDivs() { give(isl_union_map_compute_divs(take())); }
    void lexmin() { give(isl_union_map_lexmin(take())); }
    void lexmax() { give(isl_union_map_lexmax(take())); }

    void addMap_inplace(Map &&map) ISLPP_INPLACE_FUNCTION { give(isl_union_map_add_map(take(), map.take())); }
    void addMap_inplace(const Map &map) ISLPP_INPLACE_FUNCTION { give(isl_union_map_add_map(take(), map.takeCopy())); }
    UnionMap addMap(Map &&map) const { return UnionMap::enwrap(isl_union_map_add_map(takeCopy(), map.take())); }
    UnionMap addMap(const Map &map) const { return UnionMap::enwrap(isl_union_map_add_map(takeCopy(), map.takeCopy())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    UnionMap addMap(Map &&map) && { return UnionMap::enwrap(isl_union_map_add_map(take(), map.take())); }
    UnionMap addMap(const Map &map) && { return UnionMap::enwrap(isl_union_map_add_map(take(), map.takeCopy())); }
#endif

    void intersectParams(Set &&set) { give(isl_union_map_intersect_params(take(), set.take())); } 

    void gist(UnionMap &&context) { give(isl_union_map_gist(take(), context.take())); } 
    void gistParams(Set &&set) { give(isl_union_map_gist_params(take(), set.take())); } 
    void gistDomain(UnionSet &&uset) { give(isl_union_map_gist_domain(take(), uset.take())); } 
    void gistRange(UnionSet &&uset) { give(isl_union_map_gist_range(take(), uset.take())); } 

    void intersectDomain(UnionSet &&uset) { give(isl_union_map_intersect_domain(take(), uset.take())); } 
    void intersectRange(UnionSet &&uset) { give(isl_union_map_intersect_range(take(), uset.take())); } 

    void substractDomain(UnionSet &&uset) { give(isl_union_map_subtract_domain(take(), uset.take())); } 
    void substractRange(UnionSet &&uset) { give(isl_union_map_subtract_range(take(), uset.take())); } 

   ISLPP_EXSITU_ATTRS UnionMap reverse() ISLPP_EXSITU_FUNCTION { return UnionMap::enwrap(isl_union_map_reverse(takeCopy())); }
  ISLPP_INPLACE_ATTRS void reverse_inplace() ISLPP_INPLACE_FUNCTION { give(isl_union_map_reverse(take())); }
   ISLPP_CONSUME_ATTRS UnionMap reverse_consume() ISLPP_CONSUME_FUNCTION { return UnionMap::enwrap(isl_union_map_reverse(take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    UnionMap reverse() && { return UnionMap::enwrap(isl_union_map_reverse(take())); }
#endif

    void detectEqualities() { give(isl_union_map_detect_equalities(take())); }
    void deltasMap() { give(isl_union_map_deltas_map(take())); }

    bool isEmpty() const { return isl_union_map_is_empty(keep()); }
    bool isSingleValued() const { return isl_union_map_is_single_valued(keep()); }
    bool plainIsInjective() const { return isl_union_map_plain_is_injective(keep()); }
    bool isInjective() const { return isl_union_map_is_injective(keep()); }
    bool isBijective() const { return isl_union_map_is_bijective(keep()); }

    unsigned getNumMaps() const { return isl_union_map_n_map(keep()); }
    //bool foreachMap(int (*fn)(__isl_take isl_map *map, void *user), void *user) const { return isl_union_map_foreach_map(keep(), fn, user); }
    bool foreachMap(const std::function<bool(isl::Map)> &/*return true to break enumeration*/) const;
    std::vector<Map> getMaps() const;

    bool contains(const Space &space) const { return checkBool(isl_union_map_contains(keep(), space.keep())); }

    void fixedPower(const Int &exp) { give(isl_union_map_fixed_power(take(), exp.keep())); } 
    void mapPower(bool &exact) { 
      int tmp=-1;
      give(isl_union_map_power(take(), &tmp)); 
      assert(tmp!=-1);
      exact = tmp;
    } 

    ISLPP_EXSITU_ATTRS UnionMap transitiveClosure() ISLPP_EXSITU_FUNCTION {return UnionMap::enwrap(   isl_union_map_transitive_closure(takeCopy(), nullptr));    }
    ISLPP_EXSITU_ATTRS UnionMap transitiveClosure(/*out*/Approximation &approximation) ISLPP_EXSITU_FUNCTION {
      int exact=-1;
      auto result = UnionMap::enwrap(   isl_union_map_transitive_closure(takeCopy(), &exact));   
      assert(exact!=-1);
      approximation = exact ? Approximation::Exact : Approximation::Over;
      return result;
    }

    void transitiveClosure_inplace(bool &exact) {
      int tmp=-1;
      give(isl_union_map_transitive_closure(take(), &tmp)); 
      assert(tmp!=-1);
      exact = tmp;
    }

    void zip() { give(isl_union_map_zip(take())); }
    void curry() { give(isl_union_map_curry(take())); }
    void uncurry() { give(isl_union_map_uncurry(take())); }

    void alignParams(Space &&model) { give(isl_union_map_align_params(take(), model.take())); }

    void unite_inplace(UnionMap &&that) ISLPP_INPLACE_FUNCTION { give(isl_union_map_union(take(), that.take())); }
    void unite_inplace(const UnionMap &that) ISLPP_INPLACE_FUNCTION { give(isl_union_map_union(take(), that.takeCopy())); }
    UnionMap unite(UnionMap &&that) const { return UnionMap::enwrap(isl_union_map_union(takeCopy(), that.take())); }
    UnionMap unite(const UnionMap &that) const { return UnionMap::enwrap(isl_union_map_union(takeCopy(), that.takeCopy())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    UnionMap unite(UnionMap &&that) && { return UnionMap::enwrap(isl_union_map_union(take(), that.take())); }
    UnionMap unite(const UnionMap &that) && { return UnionMap::enwrap(isl_union_map_union(take(), that.takeCopy())); }
#endif

    Map extractMap(const Space &mapSpace) const { assert(mapSpace.isMapSpace()); return Map::enwrap(isl_union_map_extract_map(keep(), mapSpace.takeCopy())); }
    Map operator[](const Space &mapSpace) const { assert(mapSpace.isMapSpace()); return Map::enwrap(isl_union_map_extract_map(keep(), mapSpace.takeCopy())); } 

   ISLPP_EXSITU_ATTRS UnionSet domain() ISLPP_EXSITU_FUNCTION { return UnionSet::enwrap(isl_union_map_domain(takeCopy())); }
    ISLPP_EXSITU_ATTRS UnionSet range() ISLPP_EXSITU_FUNCTION { return UnionSet::enwrap(isl_union_map_range(takeCopy())); }

    ISLPP_EXSITU_ATTRS UnionMap applyDomain(UnionMap umap2) ISLPP_EXSITU_FUNCTION { return UnionMap::enwrap(isl_union_map_apply_domain(takeCopy(), umap2.take())); }
    ISLPP_INPLACE_ATTRS void applyDomain_inplace(UnionMap umap2) ISLPP_INPLACE_FUNCTION { give(isl_union_map_apply_domain(take(), umap2.take())); }
     ISLPP_CONSUME_ATTRS UnionMap applyDomain_consume(UnionMap umap2) ISLPP_CONSUME_FUNCTION { return UnionMap::enwrap(isl_union_map_apply_domain(take(), umap2.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
          UnionMap applyDomain_consume(UnionMap umap2) && { return UnionMap::enwrap(isl_union_map_apply_domain(take(), umap2.take())); }
#endif

          ISLPP_EXSITU_ATTRS UnionMap applyRange(UnionMap umap2) ISLPP_EXSITU_FUNCTION { return UnionMap::enwrap(isl_union_map_apply_range(takeCopy(), umap2.take())); }
          ISLPP_INPLACE_ATTRS void applyRange_inplace(UnionMap umap2) ISLPP_INPLACE_FUNCTION { give(isl_union_map_apply_range(take(), umap2.take())); }
          ISLPP_CONSUME_ATTRS UnionMap applyRange_consume(UnionMap umap2) ISLPP_CONSUME_FUNCTION { return UnionMap::enwrap(isl_union_map_apply_range(take(), umap2.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
          UnionMap applyRange_consume(UnionMap umap2) && { return UnionMap::enwrap(isl_union_map_apply_range(take(), umap2.take())); }
#endif

  }; // class UnionMap



  static inline UnionMap enwrap(isl_union_map *map) { return UnionMap::enwrap(map); }
  static inline UnionMap enwrapCopy(isl_union_map *map) { return UnionMap::enwrapCopy(map); }


  static inline Set params(UnionMap &&umap) { return Set::enwrap(isl_union_map_params(umap.take())); }
  static inline Set params(const UnionMap &umap) { return Set::enwrap(isl_union_map_params(umap.takeCopy())); }
  static inline UnionSet domain(UnionMap &&umap) { return UnionSet::enwrap(isl_union_map_domain(umap.take())); }
  static inline UnionSet domain(const UnionMap &umap) { return UnionSet::enwrap(isl_union_map_domain(umap.takeCopy())); }
  static inline UnionSet range(UnionMap &&umap) { return UnionSet::enwrap(isl_union_map_range(umap.take())); }
  static inline UnionSet range(const UnionMap &umap) { return UnionSet::enwrap(isl_union_map_range(umap.takeCopy())); }

  static inline UnionMap unite(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_union(umap1.take(), umap2.take())); }
  static inline UnionMap unite(UnionMap &&umap1, const UnionMap &umap2) { return UnionMap::enwrap(isl_union_map_union(umap1.take(), umap2.takeCopy())); } 
  static inline UnionMap unite(const UnionMap &umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_union(umap1.takeCopy(), umap2.take())); } 
  static inline UnionMap unite(const UnionMap &umap1, const UnionMap &umap2) { return UnionMap::enwrap(isl_union_map_union(umap1.takeCopy(), umap2.takeCopy())); }


  static inline UnionMap substract(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_subtract(umap1.take(), umap2.take())); }

  static inline UnionMap intersect(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_intersect(umap1.take(), umap2.take())); }
  static inline UnionMap product(UnionMap umap1, UnionMap umap2) { return UnionMap::enwrap(isl_union_map_product(umap1.take(), umap2.take())); }
  static inline UnionMap domainProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_domain_product(umap1.take(), umap2.take())); }
  static inline UnionMap rangeProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_range_product(umap1.take(), umap2.take())); }
  static inline UnionMap flatRangeProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_flat_range_product(umap1.take(), umap2.take())); }

  static inline UnionMap applyDomain(UnionMap umap1, UnionMap umap2) { return UnionMap::enwrap(isl_union_map_apply_domain(umap1.take(), umap2.take())); }
  static inline UnionMap applyRange(UnionMap umap1, UnionMap umap2) { return UnionMap::enwrap(isl_union_map_apply_range(umap1.take(), umap2.take())); }

  static inline UnionSet deltas(UnionMap &&umap) { return UnionSet::enwrap(isl_union_map_deltas(umap.take())); }

  static inline bool isSubset(const UnionMap &umap1, const UnionMap &umap2) { return checkBool(isl_union_map_is_subset(umap1.keep(), umap2.keep())); }
  static inline bool isEqual(const UnionMap &umap1, const UnionMap &umap2) { return checkBool(isl_union_map_is_equal(umap1.keep(), umap2.keep())); }
  static inline bool isStrictSubset(const UnionMap &umap1, const UnionMap &umap2) { return checkBool(isl_union_map_is_strict_subset(umap1.keep(), umap2.keep())); }

  static inline Map extractMap(const UnionMap &umap, Space &&dim) { return Map::enwrap(isl_union_map_extract_map(umap.keep(), dim.take())); }

  static inline BasicMap sample(UnionMap &&umap) { return BasicMap::enwrap(isl_union_map_sample(umap.take())); }

  static inline UnionMap lexLt(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_lt_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexLe(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_le_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexGt(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_gt_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexGe(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_ge_union_map(umap1.take(), umap2.take())); }

  static inline UnionSet wrap(UnionMap &&umap) { return UnionSet::enwrap(isl_union_map_wrap(umap.take())); }

  static inline bool operator==(const UnionMap &lhs, const UnionMap &rhs) { return (lhs.isNull() && rhs.isNull()) || (lhs.isValid() && rhs.isValid() && isEqual(lhs, rhs));  } 
  static inline bool operator!=(const UnionMap &lhs, const UnionMap &rhs) { return !operator==(lhs, rhs); } 
  static inline bool operator<(const UnionMap &lhs, const UnionMap &rhs) { return isStrictSubset(lhs, rhs); } 
  static inline bool operator<=(const UnionMap &lhs, const UnionMap &rhs) { return isSubset(lhs, rhs); } 
  static inline bool operator>(const UnionMap &lhs, const UnionMap &rhs) { return isStrictSubset(rhs, lhs); } 
  static inline bool operator>=(const UnionMap &lhs, const UnionMap &rhs) { return isSubset(rhs, lhs); } 


  static inline void computeFlow(UnionMap &&sink, UnionMap &&mustSource, UnionMap &&maySource, UnionMap &&schedule, UnionMap *mustDep, UnionMap *mayDep, UnionMap *mustNoSource, UnionMap *mayNoSource) {
    isl_union_map *mustDepObj = nullptr;
    isl_union_map *mayDepObj = nullptr;
    isl_union_map *mustNoSourceObj = nullptr;
    isl_union_map *mayNoSourceObj = nullptr;
    auto retval = isl_union_map_compute_flow(sink.take(), mustSource.take(), maySource.take(), schedule.take(), mustDep ? &mustDepObj : nullptr, mayDep ? &mayDepObj : nullptr, mustDep ? &mustNoSourceObj : nullptr, mayNoSource ? &mayNoSourceObj : nullptr);
    assert(retval == 0);
    if (mustDep) *mustDep = UnionMap::enwrap(mustDepObj);
    if (mayDep) *mayDep = UnionMap::enwrap(mayDepObj);
    if (mustNoSource) *mustNoSource = UnionMap::enwrap(mustNoSourceObj);
    if (mayNoSource) *mayNoSource = UnionMap::enwrap(mayNoSourceObj);
  }


  /// Inputs:
  ///   sink   = {  readStmt[domain] -> array[index] }
  ///   source = { writeStmt[domain] -> array[index] }
  ///   schedule = { stmt[domain] -> scattering[scatter] }
  /// Output:
  ///   dep =   { writeStmt[domain] -> stmtRead[domain] }
  ///   nosrc = {  readStmt[domain] -> array[index] }
  void simpleFlow(const UnionMap &sink, const UnionMap &source, const UnionMap &schedule, UnionMap *dep, UnionMap *nosrc); 

  static inline UnionMap alltoall(UnionSet domain, UnionSet range) {
    return UnionMap::enwrap(isl_union_set_unwrap( isl_union_set_product(domain.take(), range.take())));
  }

} // namespace isl
#endif /* ISLPP_UNIONMAP_H */
