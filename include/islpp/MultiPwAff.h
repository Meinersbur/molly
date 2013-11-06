#ifndef ISLPP_MULTIPWAFF_H
#define ISLPP_MULTIPWAFF_H

#include "islpp_common.h"
#include "Spacelike.h" // baseclass of Multi<PwAff>
#include "Multi.h"
#include "PwAff.h"
#include "Space.h"
#include "Ctx.h"
#include <isl/aff.h>
#include <isl/multi.h>
#include <llvm/Support/ErrorHandling.h>
#include "Obj.h"

// isl/aff_type.h
struct isl_multi_pw_aff;


namespace isl {
  template<>
  class Multi<PwAff> : public Obj<MultiPwAff, isl_multi_pw_aff>, public Spacelike<MultiPwAff> {
  public:
    typedef isl_multi_pw_aff IslType;
    typedef PwAff EltType;
    typedef Multi<PwAff> MultiType;

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_multi_pw_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_multi_pw_aff_copy(keepOrNull()); }

  public:
    Multi() { }

    /* implicit */ Multi(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Multi(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_multi_pw_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_multi_pw_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION { return Space::enwrap(isl_multi_pw_aff_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS Space getSpacelike() ISLPP_PROJECTION_FUNCTION { return Space::enwrap(isl_multi_pw_aff_get_space(keep())); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION { return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION { return false; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION { return true; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_multi_pw_aff_dim(keep(), type); }
    ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_multi_pw_aff_find_dim_by_id(keep(), type, id.keep()); }

    //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_pw_aff_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_multi_pw_aff_get_tuple_name(keep(), type); }
    ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_tuple_name(take(), type, s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_pw_aff_has_tuple_id(keep(), type)); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_multi_pw_aff_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_tuple_id(take(), type, id.take())); }
    ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_reset_tuple_id(take(), type)); }

    //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_pw_aff_has_dim_name(keep(), type, pos)); }
    //ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return isl_multi_pw_aff_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_dim_name(take(), type, pos, s)); }
    //ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_multi_pw_aff_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_multi_pw_aff_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_dim_id(take(), type, pos, id.take())); }
    //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_insert_dims(take(), type, pos, count)); }
  public:
    ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_drop_dims(take(), type, first, count)); }
#pragma endregion


    Space getDomainSpace() ISLPP_EXSITU_FUNCTION { return Space::enwrap(isl_multi_pw_aff_get_domain_space(keep())); }
    Space getRangeSpace() ISLPP_EXSITU_FUNCTION { return getSpace().getRangeSpace(); }
    Space getParamsSpace() ISLPP_EXSITU_FUNCTION { return getSpace().getParamsSpace(); }


#pragma region Conversion
    /* implicit */ Multi(const MultiAff &madd); 
    const MultiPwAff &operator=(const MultiAff &madd) ISLPP_INPLACE_FUNCTION;

    /* implicit */ Multi(const PwMultiAff &pma);
    const MultiPwAff &operator=(const PwMultiAff &pma) ISLPP_INPLACE_FUNCTION;

    Map toMap() const;
    //operator Map() const { return toMap(); }

    /// Warning: exponential expansion!!
    PwMultiAff toPwMultiAff() const;
#pragma endregion


#pragma region Creational
    //static MultiType fromAff(Aff &&aff) { return wrap(isl_multi_pw_aff_from_aff(aff.take())); }
    //static MultiType create(Space &&space) { return wrap(isl_multi_pw_aff_alloc(space.take()) ); }
    static MultiType createZero(Space &&space) { return MultiPwAff::enwrap(isl_multi_pw_aff_zero(space.take())); }
    static MultiType createIdentity(Space &&space) { return MultiPwAff::enwrap(isl_multi_pw_aff_identity(space.take())); }

    //static MultiType readFromString(Ctx *ctx, const char *str) { return wrap(isl_multi_pw_aff_read_from_str(ctx->keep(), str)); } 
#pragma endregion


#pragma region Printing
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    EltType getPwAff(int pos) const { return EltType::enwrap(isl_multi_pw_aff_get_pw_aff(keep(), pos)); }
    MultiType setPwAff(int pos, PwAff &&el) const { return MultiType::enwrap(isl_multi_pw_aff_set_pw_aff(takeCopy(), pos, el.take())); }
    MultiType setPwAff(int pos, const PwAff &el) const { return MultiType::enwrap(isl_multi_pw_aff_set_pw_aff(takeCopy(), pos, el.takeCopy())); }
    void setPwAff_inplace(int pos, PwAff &&el) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_pw_aff(take(), pos, el.take())); }
    void setPwAff_inplace(int pos, const PwAff &el) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_set_pw_aff(take(), pos, el.takeCopy())); }

    void push_back(PwAff &&);
#pragma endregion


    //void scale(Int f) { give(isl_multi_pw_aff_scale(take(), f.keep())); };
    //void scaleVec(Vec &&v) { give(isl_multi_pw_aff_scale_vec(take(), v.take())); }

    //void alignParams(Space &&model) { give(isl_multi_pw_aff_align_params(take(), model.take())); }
    //void gistParams(Set &&context) { give(isl_multi_pw_aff_gist_params(take(), context.take())); }
    //void gist(Set &&context) { give(isl_multi_pw_aff_gist(take(), context.take())); }
    //void lift() { give(isl_multi_pw_aff_lift(take(), nullptr)); }


    EltType operator[](unsigned pos) const { return getPwAff(pos); } 

    MultiPwAff sublist(pos_t first, count_t count) ISLPP_EXSITU_FUNCTION { auto result = copy(); result.sublist_inplace(first, count); return result; }
      void sublist_inplace(pos_t first, count_t count) ISLPP_INPLACE_FUNCTION;
    MultiPwAff sublist(const Space &subspace) ISLPP_EXSITU_FUNCTION;
    
    
    //{ auto result = copy(); result.sublist_inplace(subspace); return result; }
    //void sublist_inplace(const Space &subspace) ISLPP_INPLACE_QUALIFIER;
  
   ISLPP_EXSITU_ATTRS MultiPwAff pullback(MultiPwAff that) ISLPP_EXSITU_FUNCTION { return MultiPwAff::enwrap(isl_multi_pw_aff_pullback_multi_pw_aff(takeCopy(), that.take())); }
   ISLPP_INPLACE_ATTRS void pullback_inplace(MultiPwAff that) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_pullback_multi_pw_aff(take(), that.take())); }

   ISLPP_EXSITU_ATTRS MultiPwAff pullback(MultiAff ma) ISLPP_EXSITU_FUNCTION;
   ISLPP_INPLACE_ATTRS void pullback_inplace(MultiAff ma) ISLPP_INPLACE_FUNCTION;


    Map reverse() ISLPP_EXSITU_FUNCTION;
    //MultiPwAff applyRange(const PwMultiAff &) ISLPP_EXSITU_QUALIFIER;

  
    MultiPwAff alignParams(Space space) ISLPP_EXSITU_FUNCTION { return MultiPwAff::enwrap(isl_multi_pw_aff_align_params(takeCopy(), space.take())); }
    ISLPP_INPLACE_ATTRS void alignParams_inplace(Space space) ISLPP_INPLACE_FUNCTION { give(isl_multi_pw_aff_align_params(take(), space.take())); }

  ISLPP_EXSITU_ATTRS  MultiPwAff castRange(Space rangeSpace) ISLPP_EXSITU_FUNCTION;
  ISLPP_INPLACE_ATTRS  void castRange_inplace(Space rangeSpace) ISLPP_INPLACE_FUNCTION { give(castRange(std::move(rangeSpace)).take()); }

   ISLPP_INPLACE_ATTRS  void castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION;
   ISLPP_EXSITU_ATTRS MultiPwAff castDomain(Space domainSpace) ISLPP_EXSITU_FUNCTION { auto result = copy(); result.castDomain_inplace(std::move(domainSpace)); return result; }


    /// { B' -> B }.embedInRangeSpace({ [A, B, C] }) = { [A, B', C] -> [A, B, C] }
    /// TODO: Rename to something like applySubspace
    ISLPP_EXSITU_ATTRS MultiPwAff embedIntoRangeSpace(Space frameRangeSpace) ISLPP_EXSITU_FUNCTION;

    /// { B -> B' }.embedAsSubspace({ [A, B, C] }) = { [A, B, C] -> [A, B', C] }
    // TODO: Find better name
   ISLPP_EXSITU_ATTRS MultiPwAff embedIntoDomainSpace(Space framespace) ISLPP_EXSITU_FUNCTION;

   ISLPP_EXSITU_ATTRS Map projectOut(isl_dim_type type, pos_t first, count_t count) ISLPP_EXSITU_FUNCTION;
}; // class MultiAff


  static inline MultiPwAff enwrap(__isl_take isl_multi_pw_aff *obj) { return Multi<PwAff>::enwrap(obj); }
  static inline MultiPwAff enwrapCopy(__isl_keep isl_multi_pw_aff *obj) { return Multi<PwAff>::enwrapCopy(obj); }

  static inline void compatibilize(MultiPwAff &lhs, MultiPwAff &rhs) {
    lhs.alignParams_inplace(rhs.getSpace());
    rhs.alignParams_inplace(lhs.getSpace());
  }

  //static inline bool plainIsEqual(const Multi<PwAff> &maff1, const Multi<PwAff> &maff2) { return isl_multi_pw_aff_plain_is_equal(maff1.keep(), maff2.keep()); }
  //static inline MultiPwAff add(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_add(maff1.take(), maff2.take())); }
  //static inline MultiPwAff sub(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_sub(maff1.take(), maff2.take())); }

  static inline MultiPwAff rangeSplice(Multi<PwAff> &&maff1, unsigned pos, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_range_splice(maff1.take(), pos, maff2.take())); }
  static inline MultiPwAff splice(Multi<PwAff> &&maff1, unsigned in_pos, unsigned out_pos, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_splice(maff1.take(), in_pos, out_pos, maff2.take())); }

  static inline MultiPwAff rangeProduct(MultiPwAff maff1, MultiPwAff maff2) { compatibilize(maff1, maff2); return Multi<PwAff>::enwrap(isl_multi_pw_aff_range_product(maff1.take(), maff2.take())); }
  //static inline MultiPwAff rangeProduct(MultiPwAff maff1, MultiPwAff maff2, MultiPwAff maff3) { return rangeProduct(rangeProduct(maff1.move(), maff2.move()), maff3.move()); }

  static inline MultiPwAff flatRangeProduct(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_flat_range_product(maff1.take(), maff2.take())); }
  //static inline Multi<PwAff> product(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_product(maff1.take(), maff2.take())); }

  //static inline Multi<PwAff> pullbackMultiAff(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_pullback_multi_pw_aff(maff1.take(), maff2.take())); }

  //static inline Set lexLeSet(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Set::wrap(isl_multi_pw_aff_lex_le_set(maff1.take(), maff2.take())); }
  //static inline Set lexGeSet(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Set::wrap(isl_multi_pw_aff_lex_ge_set(maff1.take(), maff2.take())); }



} // namespace isl
#endif /* ISLPP_MULTIPWAFF_H */
