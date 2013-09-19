#ifndef ISLPP_MULTIAFF_H
#define ISLPP_MULTIAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include "Spacelike.h"

#include <isl/aff.h>
#include <isl/multi.h>
#include <cassert>
#include <llvm/Support/ErrorHandling.h>
#include "islpp/Multi.h"
#include "islpp/Aff.h"
#include "islpp/Space.h"
#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Vec.h"
#include "islpp/Int.h"
#include "islpp/Set.h"
#include "islpp/LocalSpace.h"
#include "islpp/Spacelike.h"
#include "Obj.h"

struct isl_multi_aff;

namespace llvm {
} // namespace llvm

namespace isl {
  class Aff;
} // namespace isl


namespace isl {
  typedef Multi<Aff> MultiAff;

  template<>
  class Multi<Aff> : public Obj<MultiAff, isl_multi_aff>, public Spacelike<MultiAff> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_multi_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_multi_aff_copy(keepOrNull()); }

  public:
    Multi() { }

    /* implicit */ Multi(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Multi(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_multi_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_multi_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_multi_aff_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_tuple_id(take(), type, id.take())); }
    //void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_multi_aff_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_multi_aff_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_multi_aff_get_tuple_name(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_multi_aff_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_id(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_get_dim_name(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_multi_aff_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_add_dims(take(), type, count)); }
#pragma endregion


    Space getDomainSpace() ISLPP_EXSITU_QUALIFIER { return Space::enwrap(isl_multi_aff_get_domain_space(keep())); }
    Space getRangeSpace() ISLPP_EXSITU_QUALIFIER { return getSpace().getRangeSpace(); }
    Space getParamsSpace() ISLPP_EXSITU_QUALIFIER { return getSpace().getParamsSpace(); }


#pragma region Conversion
    BasicMap toBasicMap() const;
    Map toMap() const;  
    PwMultiAff toPwMultiAff() const;
    MultiPwAff toMultiPwAff() const;
#pragma endregion


#pragma region Creational
    static Multi<Aff> fromAff(Aff &&aff) { return MultiAff::enwrap(isl_multi_aff_from_aff(aff.take())); }
    static Multi<Aff> createZero(Space &&space) { return MultiAff::enwrap(isl_multi_aff_zero(space.take())); }
    static Multi<Aff> createIdentity(const Space &space) { return MultiAff::enwrap(isl_multi_aff_identity(space.takeCopy())); }
    static Multi<Aff> createIdentity(Space &&space) { return MultiAff::enwrap(isl_multi_aff_identity(space.take())); }

    static Multi<Aff> readFromString(Ctx *ctx, const char *str) { return MultiAff::enwrap(isl_multi_aff_read_from_str(ctx->keep(), str)); } 
#pragma endregion


#pragma region Printing
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    Aff getAff(int pos) const { return Aff::enwrap(isl_multi_aff_get_aff(keep(), pos)); }
    MultiAff setAff(int pos, Aff &&el) { return MultiAff::enwrap(isl_multi_aff_set_aff(take(), pos, el.take())); }
    void setAff_inplace(int pos, const Aff &el) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_aff(take(), pos, el.takeCopy())); }
    void setAff_inplace(int pos, Aff &&el) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_set_aff(take(), pos, el.take())); }
    void push_back(Aff &&aff);
    void push_back(const Aff &aff) { return push_back(aff.copy()); }
#pragma endregion


    void scale(Int f) { give(isl_multi_aff_scale(take(), f.keep())); };
    //void scaleVec(Vec &&v) { give(isl_multi_aff_scale_vec(take(), v.take())); }

    ISLPP_EXSITU_PREFIX MultiAff alignParams(Space model) ISLPP_EXSITU_QUALIFIER { return MultiAff::enwrap(isl_multi_aff_align_params(takeCopy(), model.take())); }
    void alignParams_inplace(Space model) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_align_params(take(), model.take())); }
    ISLPP_CONSUME_PREFIX MultiAff alignParams_consume(Space model) ISLPP_CONSUME_QUALIFIER { return MultiAff::enwrap(isl_multi_aff_align_params(take(), model.take())); }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    ISLPP_CONSUME_PREFIX MultiAff alignParams(Space model) && { return MultiAff::enwrap(isl_multi_aff_align_params(take(), model.take())); }
#endif

    void gistParams(Set &&context) { give(isl_multi_aff_gist_params(take(), context.take())); }
    void gist(Set &&context) { give(isl_multi_aff_gist(take(), context.take())); }
    void lift() { give(isl_multi_aff_lift(take(), nullptr)); }
    LocalSpace lift(LocalSpace &context) { 
      isl_local_space *ls = nullptr;
      give(isl_multi_aff_lift(take(), &ls));
      return LocalSpace::enwrap(ls);
    }


#pragma region Derived
    PwMultiAff restrictDomain(Set &&set) const;
    PwMultiAff restrictDomain(const Set &set) const;
#pragma endregion


    void neg_inplace() ISLPP_INPLACE_QUALIFIER;
    MultiAff neg() const { auto result = copy(); result.neg_inplace(); return result; }

    /// Return a subset of affs (name analogous to substr, sublist, not subtraction)
    //TODO: rename to sublist
    MultiAff subMultiAff(unsigned first, unsigned count) const { auto result = copy(); result.subMultiAff_inplace(first, count); return result; }
    void subMultiAff_inplace(unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER;
    MultiAff sublist(const Space &subspace) ISLPP_EXSITU_QUALIFIER { auto result = copy(); result.sublist_inplace(subspace); return result; }
    void sublist_inplace(const Space &subspace) ISLPP_INPLACE_QUALIFIER;

    //TODO: unclear semantics, remove
    MultiAff embedAsSubspace(const Space &framespace) const;

    /// Function composition
    MultiAff pullback(const MultiAff &maff2) ISLPP_EXSITU_QUALIFIER { return MultiAff::enwrap(isl_multi_aff_pullback_multi_aff(takeCopy(), maff2.takeCopy())); }
    void pullback_inplace(const MultiAff &maff2) ISLPP_INPLACE_QUALIFIER { give(isl_multi_aff_pullback_multi_aff(take(), maff2.takeCopy())); }

    PwMultiAff pullback(const PwMultiAff &mpa) ISLPP_EXSITU_QUALIFIER;

    /// Would be ineffective
    //MultiPwAff pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER;

    PwMultiAff applyRange(const PwMultiAff &pma) ISLPP_EXSITU_QUALIFIER;

    MultiAff cast(Space space) ISLPP_EXSITU_QUALIFIER;
    void cast_inplace(Space space) ISLPP_INPLACE_QUALIFIER { obj_give(cast(space)); }
  }; // class MultiAff


  static inline MultiAff enwrap(__isl_take isl_multi_aff *obj) { return MultiAff::enwrap(obj); }
  static inline MultiAff enwrapCopy(__isl_keep isl_multi_aff *obj) { return MultiAff::enwrapCopy(obj); }

  static inline void compatibilize(/*inout*/MultiAff &maff1, /*inout*/MultiAff &maff2) {
    maff1.alignParams_inplace(maff2.getSpace());
    maff2.alignParams_inplace(maff1.getSpace());
  }

  static inline bool plainIsEqual(const Multi<Aff> &maff1, const Multi<Aff> &maff2) { return isl_multi_aff_plain_is_equal(maff1.keep(), maff2.keep()); }
  static inline Multi<Aff> add(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_add(maff1.take(), maff2.take())); }
  static inline Multi<Aff> sub(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_sub(maff1.take(), maff2.take())); }

  static inline Multi<Aff> rangeSplice(Multi<Aff> &&maff1, unsigned pos, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_range_splice(maff1.take(), pos, maff2.take())); }
  static inline Multi<Aff> splice(Multi<Aff> &&maff1, unsigned in_pos, unsigned out_pos, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_splice(maff1.take(), in_pos, out_pos, maff2.take())); }

  static inline MultiAff rangeProduct(MultiAff maff1, MultiAff maff2) { compatibilize(maff1, maff2); return MultiAff::enwrap(isl_multi_aff_range_product(maff1.take(), maff2.take())); }
  static inline Multi<Aff> flatRangeProduct(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_flat_range_product(maff1.take(), maff2.take())); }
  static inline Multi<Aff> product(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_product(maff1.take(), maff2.take())); }

  static inline Multi<Aff> pullbackMultiAff(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_pullback_multi_aff(maff1.take(), maff2.take())); }

  static inline Set lexLeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::enwrap(isl_multi_aff_lex_le_set(maff1.take(), maff2.take())); }
  static inline Set lexGeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::enwrap(isl_multi_aff_lex_ge_set(maff1.take(), maff2.take())); }
} // namespace isl

#endif /* ISLPP_MULTIAFF_H */
