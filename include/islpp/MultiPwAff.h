#ifndef ISLPP_MULTIPWAFF_H
#define ISLPP_MULTIPWAFF_H

#include "islpp_common.h"
#include "Spacelike.h"
#include "Multi.h"

#include "PwAff.h"
#include "Space.h"
#include "Ctx.h"
#include <isl/aff.h>
#include <isl/multi.h>
#include <cassert>
#include <llvm/Support/ErrorHandling.h>
#include "Obj.h"

// isl/aff_type.h
struct isl_multi_pw_aff;


namespace isl {
  template<>
  class Multi<PwAff> : public Obj3<MultiPwAff, isl_multi_pw_aff>, public Spacelike3<MultiPwAff> {
  public:
    typedef isl_multi_pw_aff IslType;
    typedef PwAff EltType;
    typedef Multi<PwAff> MultiType;

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_multi_pw_aff_free(takeOrNull()); }

  public:
    Multi() { }

    /* implicit */ Multi(ObjTy &&that) : Obj3(std::move(that)) { }
    /* implicit */ Multi(const ObjTy &that) : Obj3(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

  public:
    StructTy *takeCopyOrNull() const { return isl_multi_pw_aff_copy(keepOrNull()); }

    Ctx *getCtx() const { return Ctx::wrap(isl_multi_pw_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_multi_pw_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike3
    friend class isl::Spacelike3<ObjTy>;
  public:
    Space getSpace() const { return Space::wrap(isl_multi_pw_aff_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_set_tuple_id(take(), type, id.take())); }
    //void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    //void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_remove_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_multi_pw_aff_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_pw_aff_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_multi_pw_aff_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_multi_pw_aff_get_tuple_name(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_multi_pw_aff_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_pw_aff_has_dim_id(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_pw_aff_get_dim_name(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_multi_pw_aff_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_multi_pw_aff_add_dims(take(), type, count)); }
#pragma endregion


#if 0
#ifndef NDEBUG
  private:
    std::string _printed;
#endif

  public:
    typedef isl_multi_pw_aff IslType;
    typedef PwAff EltType;
    typedef Multi<PwAff> MultiType;

#pragma region Low-level
  private:
    IslType *multi;

  public: // Public because otherwise we had to add a lot of friends
    IslType *take() { assert(multi); IslType *result = multi; multi = nullptr; return result; }
    IslType *takeCopy() const { assert(multi); return isl_multi_pw_aff_copy(multi); }
    IslType *keep() const { assert(multi); return multi; }
  protected:
    void give(IslType *multi) { assert(multi); if (this->multi) isl_multi_pw_aff_free(this->multi); this->multi = multi;
#ifndef NDEBUG
    this->_printed = toString();
#endif
    }

    //explicit Multi(IslType *multi) : multi(multi) { }
  public:
    static MultiType wrap(IslType *multi) { MultiType result; result.give(multi); return result; }
#pragma endregion

  public:
    Multi() : multi(nullptr) {}
    Multi(const MultiType &that) : multi(nullptr) {give(that.takeCopy());}
    Multi(MultiType &&that) : multi(nullptr) { give(that.take()); }
    ~Multi() {
      if (this->multi)
        isl_multi_pw_aff_free(this->multi); 
#ifndef NDEBUG
      this->multi = nullptr;
#endif
    }

    const MultiType &operator=(const MultiType &that) { give(that.takeCopy()); return *this; }
    const MultiType &operator=(MultiType &&that) { give(that.take()); return *this; }

    Ctx *getCtx() const { return Ctx::wrap(isl_multi_pw_aff_get_ctx(keep())); }
#endif

    //Space getSpace() const { return Space::wrap(isl_multi_pw_aff_get_space(keep())); }
    Space getDomainSpace() const { return Space::wrap(isl_multi_pw_aff_get_domain_space(keep())); }


#pragma region Conversion
    Map toMap() const;
#pragma endregion


#pragma region Creational
    //static MultiType fromAff(Aff &&aff) { return wrap(isl_multi_pw_aff_from_aff(aff.take())); }
    //static MultiType create(Space &&space) { return wrap(isl_multi_pw_aff_alloc(space.take()) ); }
    static MultiType createZero(Space &&space) { return MultiPwAff::enwrap(isl_multi_pw_aff_zero(space.take())); }
    static MultiType createIdentity(Space &&space) { return MultiPwAff::enwrap(isl_multi_pw_aff_identity(space.take())); }

    //static MultiType readFromString(Ctx *ctx, const char *str) { return wrap(isl_multi_pw_aff_read_from_str(ctx->keep(), str)); } 

        //MultiType copy() const { return wrap(takeCopy()); }
#pragma endregion

#if 0
#pragma region Dimensions
    unsigned dim(enum isl_dim_type type) const { return isl_multi_pw_aff_dim(keep(), type); }

    //bool hasTupleName(isl_dim_type type) const { return isl_multi_aff_has_tuple_name(keep(), type); } 
    const char *getTupleName(isl_dim_type type) const { return isl_multi_pw_aff_get_tuple_name(keep(), type); }
    void setTupleName(isl_dim_type type, const char *s) { give(isl_multi_pw_aff_set_tuple_name(take(), type, s)); }

    //bool hasTupleId(isl_dim_type type) const { return isl_multi_aff_has_tuple_id(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::wrap(isl_multi_aff_get_tuple_id(keep(), type)); }
    void setTupleId(isl_dim_type type, Id &&id) { llvm_unreachable("API function missing"); }

    //bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_name(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_aff_get_dim_name(keep(), type, pos); }
    void setDimName(isl_dim_type type, unsigned pos, const char *s) { give(isl_multi_pw_aff_set_dim_name(take(), type, pos, s)); }
    //int findDimByName(isl_dim_type type, const char *name) const { return isl_multi_aff_find_dim_by_name(keep(), type, name); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_aff_has_dim_id(keep(), type, pos); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::wrap(isl_multi_aff_get_dim_id(keep(), type, pos)); }
    //void setDimId(isl_dim_type type, unsigned pos, Id &&id) { give(isl_multi_aff_set_dim_id(take(), type, pos, id.take())); }
    void setDimId(isl_dim_type type, unsigned pos, Id &&id) { llvm_unreachable("API function missing"); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    void addDims(isl_dim_type type, unsigned n) { give(isl_multi_pw_aff_add_dims(take(), type, n)); }
    void insertDims(isl_dim_type type, unsigned pos, unsigned n) { give(isl_multi_pw_aff_insert_dims(take(), type, pos, n)); }
    //void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { give(isl_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n)); }
    void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) { llvm_unreachable("API function missing"); }
    //void dropDims(isl_dim_type type, unsigned first, unsigned n) { give(isl_multi_pw_aff_drop_dims(take(), type, first, n)); }

    void removeDims(isl_dim_type type, unsigned first, unsigned n) { llvm_unreachable("API function missing"); }
#pragma endregion
#endif

#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Multi
    EltType getPwAff(int pos) const { return EltType::enwrap(isl_multi_pw_aff_get_pw_aff(keep(), pos)); }
    MultiType setPwAff(int pos, PwAff &&el) const { return MultiType::enwrap(isl_multi_pw_aff_set_pw_aff(takeCopy(), pos,  el.take())); }
    void setPwAff_inplace(int pos, PwAff &&el) { give(isl_multi_pw_aff_set_pw_aff(take(), pos,  el.take())); }

    void push_back(PwAff &&);
#pragma endregion


    //void scale(Int f) { give(isl_multi_pw_aff_scale(take(), f.keep())); };
    //void scaleVec(Vec &&v) { give(isl_multi_pw_aff_scale_vec(take(), v.take())); }

    //void alignParams(Space &&model) { give(isl_multi_pw_aff_align_params(take(), model.take())); }
    //void gistParams(Set &&context) { give(isl_multi_pw_aff_gist_params(take(), context.take())); }
    //void gist(Set &&context) { give(isl_multi_pw_aff_gist(take(), context.take())); }
    //void lift() { give(isl_multi_pw_aff_lift(take(), nullptr)); }
  }; // class MultiAff

  //static inline bool plainIsEqual(const Multi<PwAff> &maff1, const Multi<PwAff> &maff2) { return isl_multi_pw_aff_plain_is_equal(maff1.keep(), maff2.keep()); }
  //static inline Multi<PwAff> add(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_add(maff1.take(), maff2.take())); }
  //static inline Multi<PwAff> sub(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_sub(maff1.take(), maff2.take())); }

  static inline Multi<PwAff> rangeSplice(Multi<PwAff> &&maff1, unsigned pos, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_range_splice(maff1.take(), pos, maff2.take())); }
  static inline Multi<PwAff> splice(Multi<PwAff> &&maff1, unsigned in_pos, unsigned out_pos, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_splice(maff1.take(), in_pos, out_pos, maff2.take())); }

  static inline Multi<PwAff> rangeProduct(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_range_product(maff1.take(), maff2.take())); }
  static inline Multi<PwAff> flatRangeProduct(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::enwrap(isl_multi_pw_aff_flat_range_product(maff1.take(), maff2.take())); }
  //static inline Multi<PwAff> product(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_product(maff1.take(), maff2.take())); }

  //static inline Multi<PwAff> pullbackMultiAff(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Multi<PwAff>::wrap(isl_multi_pw_aff_pullback_multi_pw_aff(maff1.take(), maff2.take())); }

  //static inline Set lexLeSet(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Set::wrap(isl_multi_pw_aff_lex_le_set(maff1.take(), maff2.take())); }
  //static inline Set lexGeSet(Multi<PwAff> &&maff1, Multi<PwAff> &&maff2) { return Set::wrap(isl_multi_pw_aff_lex_ge_set(maff1.take(), maff2.take())); }

  static inline Multi<PwAff> enwrap(isl_multi_pw_aff *obj) { return Multi<PwAff>::enwrap(obj); }

} // namespace isl

#endif /* ISLPP_MULTIPWAFF_H */
