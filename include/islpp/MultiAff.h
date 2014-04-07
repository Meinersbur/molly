#ifndef ISLPP_MULTIAFF_H
#define ISLPP_MULTIAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include "Spacelike.h"
#include "ParamSpace.h"
#include "MapSpace.h"

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
  class MultiAffSubscript;

  template<>
  class Multi<Aff> : public Obj<MultiAff, isl_multi_aff>, public Spacelike<MultiAff>{

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
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_multi_aff_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS Space getSpacelike() ISLPP_PROJECTION_FUNCTION{ return getSpace(); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return true; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_multi_aff_dim(keep(), type); }
    ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION{ return isl_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

      //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_aff_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_multi_aff_get_tuple_name(keep(), type); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_tuple_name(take(), type, s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_multi_aff_has_tuple_id(keep(), type)); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_multi_aff_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_tuple_id(take(), type, id.take())); }
    ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_reset_tuple_id(take(), type)); }

      //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_aff_has_dim_name(keep(), type, pos)); }
      //ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return isl_multi_aff_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_dim_name(take(), type, pos, s)); }
      //ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_aff_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_multi_aff_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_multi_aff_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_insert_dims(take(), type, pos, count)); }
  public:
    //ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_drop_dims(take(), type, first, count)); }
#pragma endregion


    Space getDomainSpace() ISLPP_EXSITU_FUNCTION{ return Space::enwrap(isl_multi_aff_get_domain_space(keep())); }
    Space getRangeSpace() ISLPP_EXSITU_FUNCTION{ return getSpace().getRangeSpace(); }
    Space getParamsSpace() ISLPP_EXSITU_FUNCTION{ return getSpace().getParamsSpace(); }


#pragma region Conversion
    BasicMap toBasicMap() const;
    Map toMap() const;
    PwMultiAff toPwMultiAff() const;
    MultiPwAff toMultiPwAff() const;
#pragma endregion


#pragma region Creational
    static MultiAff fromAff(Aff aff) { return MultiAff::enwrap(isl_multi_aff_from_aff(aff.take())); }
    static MultiAff createZero(Space space) { return MultiAff::enwrap(isl_multi_aff_zero(space.take())); }
    static MultiAff createIdentity(Space space) { return MultiAff::enwrap(isl_multi_aff_identity(space.take())); }

    static MultiAff readFromString(Ctx *ctx, const char *str) { return MultiAff::enwrap(isl_multi_aff_read_from_str(ctx->keep(), str)); }
#pragma endregion


#pragma region Printing
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    Aff getAff(int pos) const { return Aff::enwrap(isl_multi_aff_get_aff(keep(), pos)); }
    ISLPP_EXSITU_ATTRS MultiAff setAff(int pos, Aff &&el) ISLPP_EXSITU_FUNCTION{ return MultiAff::enwrap(isl_multi_aff_set_aff(takeCopy(), pos, el.take())); }
    void setAff_inplace(int pos, const Aff &el) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_aff(take(), pos, el.takeCopy())); }
    void setAff_inplace(int pos, Aff &&el) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_set_aff(take(), pos, el.take())); }
    ISLPP_DEPRECATED void push_back(Aff &&aff);
    //ISLPP_DEPRECATED void push_back(const Aff &aff) { return push_back(aff.copy()); }

    MultiAffSubscript operator[](pos_t pos);
    Aff operator[](pos_t pos) const { return getAff(pos); }
#pragma endregion


    void scale(Int f) { give(isl_multi_aff_scale(take(), f.keep())); };
    //void scaleVec(Vec &&v) { give(isl_multi_aff_scale_vec(take(), v.take())); }

    ISLPP_EXSITU_ATTRS MultiAff alignParams(Space model) ISLPP_EXSITU_FUNCTION{ return MultiAff::enwrap(isl_multi_aff_align_params(takeCopy(), model.take())); }
    void alignParams_inplace(Space model) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_align_params(take(), model.take())); }
    ISLPP_CONSUME_ATTRS MultiAff alignParams_consume(Space model) ISLPP_CONSUME_FUNCTION{ return MultiAff::enwrap(isl_multi_aff_align_params(take(), model.take())); }
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
    ISLPP_CONSUME_ATTRS MultiAff alignParams(Space model) && { return MultiAff::enwrap(isl_multi_aff_align_params(take(), model.take())); }
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


    void neg_inplace() ISLPP_INPLACE_FUNCTION;
    MultiAff neg() const { auto result = copy(); result.neg_inplace(); return result; }

    /// Return a subset of affs (name analogous to substr, sublist, not subtraction)
    //TODO: rename to sublist
    MultiAff subMultiAff(unsigned first, unsigned count) const { auto result = copy(); result.subMultiAff_inplace(first, count); return result; }
    void subMultiAff_inplace(unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION;
    MultiAff sublist(const Space &subspace) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.sublist_inplace(subspace); return result; }
    void sublist_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION;

    //TODO: unclear semantics, remove
    MultiAff embedAsSubspace(const Space &framespace) const;

    ISLPP_EXSITU_ATTRS MultiAff embedIntoDomain(Space framedomainspace) ISLPP_EXSITU_FUNCTION;


    /// Function composition
    MultiAff pullback(MultiAff maff2) ISLPP_EXSITU_FUNCTION{ return MultiAff::enwrap(isl_multi_aff_pullback_multi_aff(takeCopy(), maff2.take())); }
    void pullback_inplace(MultiAff maff2) ISLPP_INPLACE_FUNCTION{ give(isl_multi_aff_pullback_multi_aff(take(), maff2.take())); }

    PwMultiAff pullback(PwMultiAff mpa) ISLPP_EXSITU_FUNCTION;

    /// Would be ineffective
    //MultiPwAff pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER;

    PwMultiAff applyRange(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION;

    MultiAff cast(Space space) ISLPP_EXSITU_FUNCTION;
    void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ obj_give(cast(space)); }

    ISLPP_EXSITU_ATTRS MultiAff cast(Space domainSpace, Space rangeSpace) ISLPP_EXSITU_FUNCTION{ return cast(Space::createMapFromDomainAndRange(domainSpace, rangeSpace)); }

    ISLPP_EXSITU_ATTRS MultiAff castDomain(Space rangeSpace) ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.castDomain_inplace(rangeSpace); return result; }
    ISLPP_INPLACE_ATTRS void castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS MultiAff castRange(Space rangeSpace) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void castRange_inplace(Space rangeSpace) ISLPP_INPLACE_FUNCTION{ obj_give(castRange(rangeSpace)); }

    ISLPP_EXSITU_ATTRS BasicSet getDomain() ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS BasicSet getRange() ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS BasicMap reverse() ISLPP_EXSITU_FUNCTION;
    ISLPP_CONSUME_ATTRS BasicMap reverse_consume() ISLPP_CONSUME_FUNCTION;
  }; // class MultiAff


  class MultiAffSubscript {
    Multi<Aff> &parent;
    pos_t pos;

  private:
    MultiAffSubscript() LLVM_DELETED_FUNCTION;

  public:
    MultiAffSubscript(Multi<Aff> &parent, pos_t pos) : parent(parent), pos(pos) {}

    operator Aff() const { return parent.getAff(pos); }
    void operator=(Aff arg) { parent.setAff_inplace(pos, std::move(arg)); }
  }; // class MultiAffSubscript


  inline MultiAffSubscript Multi<Aff>::operator[](pos_t pos) {
    return MultiAffSubscript(*this, pos);
  }



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
  static inline MultiAff flatRangeProduct(MultiAff maff1, MultiAff maff2) { return MultiAff::enwrap(isl_multi_aff_flat_range_product(maff1.take(), maff2.take())); }
  static inline MultiAff product(MultiAff maff1, MultiAff maff2) { return MultiAff::enwrap(isl_multi_aff_product(maff1.take(), maff2.take())); }

  static inline Multi<Aff> pullbackMultiAff(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Multi<Aff>::enwrap(isl_multi_aff_pullback_multi_aff(maff1.take(), maff2.take())); }

  static inline Set lexLeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::enwrap(isl_multi_aff_lex_le_set(maff1.take(), maff2.take())); }
  static inline Set lexGeSet(Multi<Aff> &&maff1, Multi<Aff> &&maff2) { return Set::enwrap(isl_multi_aff_lex_ge_set(maff1.take(), maff2.take())); }
} // namespace isl

#endif /* ISLPP_MULTIAFF_H */
