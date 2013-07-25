#ifndef ISLPP_UNIONSET_H
#define ISLPP_UNIONSET_H

#include "islpp_common.h"
#include <cassert>
#include "Union.h"
#include "Obj.h"
#include <isl/union_set.h>
#include "Ctx.h"
#include "BasicSet.h"
#include "Set.h"
#include "Space.h"
#include <functional>
#include <vector>

struct isl_union_set;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  // Basically a disctionary from isl::Space to isl::Set with that space
  template<>
  class Union<Set> : public Obj3<Union<Set>,isl_union_set> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_union_set_free(takeOrNull()); }
    StructTy *addref() const { return isl_union_set_copy(keepOrNull()); }

  public:
    Union() { }

    /* implicit */ Union(const ObjTy &that) : Obj3(that) { }
    /* implicit */ Union(ObjTy &&that) : Obj3(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_union_set_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_union_set_dump(keep()); }
#pragma endregion


#pragma region Creational
    static UnionSet fromBasicSet(const BasicSet &bset) { return UnionSet::enwrap(isl_union_set_from_basic_set(bset.takeCopy())); }
    static UnionSet fromSet(const Set &set) { return UnionSet::enwrap(isl_union_set_from_set(set.takeCopy())); }
    static UnionSet empty(const Space &space) { return  UnionSet::enwrap(isl_union_set_empty(space.takeCopy())); }

    static UnionSet readFromFile(Ctx *ctx, FILE *input) { return UnionSet::enwrap(isl_union_set_read_from_file(ctx->keep(), input)); }
    static UnionSet readFromStr(Ctx *ctx, const char *str) { return UnionSet::enwrap(isl_union_set_read_from_str(ctx->keep(), str)); }
#pragma endregion


#pragma region Conversion
    Union(Set &&set) : Obj3(isl_union_set_from_set(set.take())) {}
    Union(const Set &set) : Obj3(isl_union_set_from_set(set.takeCopy())) {}
    const UnionSet &operator=(Set &&set) { reset(isl_union_set_from_set(set.take()));  return *this; }
    const UnionSet &operator=(const Set &set) { reset(isl_union_set_from_set(set.takeCopy()));  return *this; }
#pragma endregion


    Space getSpace() const { return Space::enwrap(isl_union_set_get_space(keep())); }
    UnionSet universe() const { return UnionSet::enwrap(isl_union_set_universe(takeCopy())); }
    Set params() const { return Set::enwrap(isl_union_set_params(takeCopy())); }
    UnionSet detectEqualities() const {return UnionSet::enwrap(isl_union_set_detect_equalities(takeCopy())); }
    UnionSet affineHull() const {return UnionSet::enwrap(isl_union_set_affine_hull(takeCopy())); }
    UnionSet polyhedralHull() const {return UnionSet::enwrap(isl_union_set_polyhedral_hull(takeCopy())); }
    UnionSet simpleHull() const {return UnionSet::enwrap(isl_union_set_simple_hull(takeCopy())); }
    UnionSet coalesce() const {return UnionSet::enwrap(isl_union_set_coalesce(takeCopy())); }
    UnionSet computeDivs() const {return UnionSet::enwrap(isl_union_set_compute_divs(takeCopy())); }
    UnionSet lexmin() const {return UnionSet::enwrap(isl_union_set_lexmin(takeCopy())); }
    UnionSet lexmax() const {return UnionSet::enwrap(isl_union_set_lexmax(takeCopy())); }

    UnionSet addSet(const Set &set) const { return UnionSet::enwrap(isl_union_set_add_set(takeCopy(), set.takeCopy())); }
    UnionSet unite(const UnionSet &uset2) const { return UnionSet::enwrap(isl_union_set_union(takeCopy(), uset2.takeCopy())); }

    UnionSet substract(const UnionSet &uset2) const { return UnionSet::enwrap(isl_union_set_subtract(takeCopy(), uset2.takeCopy())); }
    UnionSet intersect(const UnionSet &uset2) const { return UnionSet::enwrap(isl_union_set_intersect(takeCopy(), uset2.takeCopy())); }
    UnionSet intersectParams(const Set &set) const { return UnionSet::enwrap(isl_union_set_intersect_params(takeCopy(), set.takeCopy())); }
    UnionSet product(const UnionSet &uset2) const { return UnionSet::enwrap(isl_union_set_product(takeCopy(), uset2.takeCopy())); }

    UnionSet gist(const UnionSet &context) const { return UnionSet::enwrap(isl_union_set_product(takeCopy(), context.takeCopy())); }
    UnionSet gistParams(const Set &set) const { return UnionSet::enwrap(isl_union_set_gist_params(takeCopy(), set.takeCopy())); }

    UnionSet apply(const UnionMap &umap) const;

    bool isParams() const { return isl_union_set_is_params(keep()); }
    bool isEmpty() const { return isl_union_set_is_empty(keep()); }

    bool isSubset(const UnionSet &uset2) const { return isl_union_set_is_subset(keep(), uset2.keep()); }
    bool isEqual(const UnionSet &uset2) const { return isl_union_set_is_equal(keep(), uset2.keep()); }
    bool isStrictSubset(const UnionSet &uset2) const { return isl_union_set_is_strict_subset(keep(), uset2.keep()); }

    unsigned nSet() const { auto result = isl_union_set_n_set(keep()); assert(result >= 0); return result; }
    bool foreachSet(const std::function<bool(isl::Set)> &func) const ;
    std::vector<Set> getSets() const;

    bool contains(const Space &space) const { return isl_union_set_contains(keep(), space.keep()); } 

    Set extractSet(const Space &space) const { return Set::enwrap(isl_union_set_extract_set(keep(), space.takeCopy())); }

    bool foreachPoint(const std::function<bool(isl::Point)> &func) const;

    BasicSet sample() const { return BasicSet::wrap(isl_union_set_sample(takeCopy())); }
    UnionSet lift(const UnionMap &umap) const { return UnionSet::enwrap(isl_union_set_lift(takeCopy())); }

    UnionMap lt(const UnionSet &uset2) const;
    UnionMap le(const UnionSet &uset2) const;
    UnionMap gt(const UnionSet &uset2) const;
    UnionMap ge(const UnionSet &uset2) const;

    UnionSet coefficients() const { return UnionSet::enwrap(isl_union_set_coefficients(takeCopy())); }
    UnionSet solutions() const { return UnionSet::enwrap(isl_union_set_coefficients(takeCopy())); }
  }; // class UnionSet

} // namespace isl
#endif /* ISLPP_UNIONSET_H */
