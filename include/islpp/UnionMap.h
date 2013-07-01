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

struct isl_union_map;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  /// Contains a Map per Space
  template<>
#define UnionMap Union<Map> LLVM_FINAL
  class UnionMap : public/*because otherwise we had to make a lot of friends*/ isl::Obj3<Union<Map>, struct isl_union_map> {
#undef UnionMap
 friend class isl::Obj3<UnionMap, struct isl_union_map>;
  public:
    //typedef struct isl_union_map StructTy;
    //typedef Union<Map> ObjTy;
    typedef Map EltTy;
    typedef Union<Map> UnionTy;

#pragma region Low-level
  private:
    void release() { isl_union_map_free(keepOrNull()); }

  public:
     Union() {}

    /* implicit */ Union(const ObjTy &that) : Obj3(that) { }
    /* implicit */ Union(ObjTy &&that) : Obj3(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    StructTy *takeCopyOrNull() const { return isl_union_map_copy(keep()); }
    static UnionTy enwrap(StructTy *map) { ObjTy result; result.give(map); return result; }
#pragma endregion


#pragma region Creational
    static UnionMap createFrom(BasicMap &&bmap) { return UnionMap::enwrap(isl_union_map_from_basic_map(bmap.take())); }
    static UnionMap createFrom(Map &&map) { return UnionMap::enwrap(isl_union_map_from_map(map.take())); }
    static UnionMap createEmpty(Space &&space) { return UnionMap::enwrap(isl_union_map_empty(space.take())); } 

    static UnionMap createFromDomain(UnionSet &&uset) {return UnionMap::enwrap(isl_union_map_from_domain(uset.take())); } 
    static UnionMap createFromRange(UnionSet &&uset) {return UnionMap::enwrap(isl_union_map_from_range(uset.take())); } 
    static UnionMap createFromDomainAndRange(UnionSet &&domain, UnionSet &&range) { return UnionMap::enwrap(isl_union_map_from_domain_and_range(domain.take(), range.take())); }

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


    Ctx *getCtx() const { return Ctx::wrap(isl_union_map_get_ctx(keep())); }
    Space getSpace() const { return Space::wrap(isl_union_map_get_space(keep())); }

    void universe() { give(isl_union_map_universe(take())); } // ???
    void domainMap() { give(isl_union_map_domain_map(take())); } // ???
    void rangeMap() { give(isl_union_map_range_map(take())); } // ???

    void affineHull() { give(isl_union_map_affine_hull(take())); }
    void polyhedralHull()  { give(isl_union_map_polyhedral_hull(take())); }
    void simpleHull()  { give(isl_union_map_simple_hull(take())); }
    void coalesce()  { give(isl_union_map_coalesce(take())); }

    void computeDivs() { give(isl_union_map_compute_divs(take())); }
    void lexmin() { give(isl_union_map_lexmin(take())); }
    void lexmax() { give(isl_union_map_lexmax(take())); }

    void addMap(Map &&map) { give(isl_union_map_add_map(take(), map.take())); }
    void intersectParams(Set &&set) { give(isl_union_map_intersect_params(take(), set.take())); } 

    void gist(UnionMap &&context) { give(isl_union_map_gist(take(), context.take())); } 
    void gistParams(Set &&set) { give(isl_union_map_gist_params(take(), set.take())); } 
    void gistDomain(UnionSet &&uset) { give(isl_union_map_gist_domain(take(), uset.take())); } 
    void gistRange(UnionSet &&uset) { give(isl_union_map_gist_range(take(), uset.take())); } 

    void intersectDomain(UnionSet &&uset) { give(isl_union_map_intersect_domain(take(), uset.take())); } 
    void intersectRange(UnionSet &&uset) { give(isl_union_map_intersect_range(take(), uset.take())); } 

    void substractDomain(UnionSet &&uset) { give(isl_union_map_subtract_domain(take(), uset.take())); } 
    void substractRange(UnionSet &&uset) { give(isl_union_map_subtract_range(take(), uset.take())); } 

    void reverse() { give(isl_union_map_reverse(take())); }
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

    bool contains(const Space &space) const { return isl_union_map_contains(keep(), space.keep()); }

    void fixedPower(const Int &exp) { give(isl_union_map_fixed_power(take(), exp.keep())); } 
    void mapPower(bool &exact) { 
      int tmp=-1;
      give(isl_union_map_power(take(), &tmp)); 
      assert(tmp!=-1);
      exact = tmp;
    } 

    void transitiveClosure(bool &exact) {
      int tmp=-1;
      give(isl_union_map_transitive_closure(take(), &tmp)); 
      assert(tmp!=-1);
      exact = tmp;
    } 

    void zip() { give(isl_union_map_zip(take())); }
    void curry() { give(isl_union_map_curry(take())); }
    void uncurry() { give(isl_union_map_uncurry(take())); }

    void alignParams(Space &&model) { give(isl_union_map_align_params(take(), model.take())); }
  }; // class UnionMap

  //static inline UnionMap wrap(isl_union_map *map) { return UnionMap::wrap(map); }
  static inline UnionMap enwrap(isl_union_map *map) { return UnionMap::enwrap(map); }

  static inline Set params(UnionMap &&umap) { return Set::wrap(isl_union_map_params(umap.take())); }
  static inline UnionSet domain(UnionMap &&umap) { return UnionSet::wrap(isl_union_map_domain(umap.take())); }
  static inline UnionSet range(UnionMap &&umap) { return UnionSet::wrap(isl_union_map_range(umap.take())); }

  static inline UnionMap union_(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_union(umap1.take(), umap2.take())); } //TODO: rename to unit?
  static inline UnionMap substract(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_subtract(umap1.take(), umap2.take())); }
  static inline UnionMap intersect(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_intersect(umap1.take(), umap2.take())); }
  static inline UnionMap product(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_product(umap1.take(), umap2.take())); }
  static inline UnionMap domainProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_domain_product(umap1.take(), umap2.take())); }
  static inline UnionMap rangeProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_range_product(umap1.take(), umap2.take())); }
  static inline UnionMap flatRangeProduct(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_flat_range_product(umap1.take(), umap2.take())); }

  static inline UnionMap applyDomain(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_apply_domain(umap1.take(), umap2.take())); }
  static inline UnionMap applyRange(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_apply_range(umap1.take(), umap2.take())); }

  static  inline UnionSet deltas(UnionMap &&umap) { return UnionSet::wrap(isl_union_map_deltas(umap.take())); }

  static  inline bool isSubset(const UnionMap &umap1, const UnionMap &umap2) { return isl_union_map_is_subset(umap1.keep(), umap2.keep()); }
  static inline  bool isEqual(const UnionMap &umap1, const UnionMap &umap2) { return isl_union_map_is_equal(umap1.keep(), umap2.keep()); }
  static inline bool isStrictSubset(const UnionMap &umap1, const UnionMap &umap2) { return isl_union_map_is_strict_subset(umap1.keep(), umap2.keep()); }

  static inline Map extractMap(const UnionMap &umap, Space &&dim) { return Map::wrap(isl_union_map_extract_map(umap.keep(), dim.take())); }

  static inline BasicMap sample(UnionMap &&umap) { return BasicMap::enwrap(isl_union_map_sample(umap.take())); }

  static inline UnionMap lexLt(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_lt_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexLe(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_le_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexGt(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_gt_union_map(umap1.take(), umap2.take())); }
  static inline UnionMap lexGe(UnionMap &&umap1, UnionMap &&umap2) { return UnionMap::enwrap(isl_union_map_lex_ge_union_map(umap1.take(), umap2.take())); }

  static inline UnionSet wrap(UnionMap &&umap) { return UnionSet::wrap(isl_union_map_wrap(umap.take())); }//TODO: Unfortunate overloading
} // namespace isl
#endif /* ISLPP_UNIONMAP_H */
