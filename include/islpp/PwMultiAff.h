#ifndef ISLPP_PWMULTIAFF_H
#define ISLPP_PWMULTIAFF_H

#include "islpp_common.h"
#include <assert.h>
#include "Obj.h"
#include "Pw.h"
#include <isl/aff.h>
#include "Ctx.h"
#include "Spacelike.h"
#include "Id.h"
#include <llvm/Support/ErrorHandling.h>
#include "PwAff.h"
#include "Set.h"
#include "Val.h"
#include "Vec.h"
#include "Space.h"
#include "MultiAff.h"

struct isl_pw_multi_aff;

namespace isl {
} // namespace isl

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {
  template<>
  class Pw<MultiAff> : public Obj3<PwMultiAff, isl_pw_multi_aff>, public Spacelike3<PwMultiAff> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_pw_multi_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_pw_multi_aff_copy(keepOrNull()); }

  public:
    Pw() { }

    /* implicit */ Pw(ObjTy &&that) : Obj3(std::move(that)) { }
    /* implicit */ Pw(const ObjTy &that) : Obj3(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_pw_multi_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_pw_multi_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike3
    friend class isl::Spacelike3<ObjTy>;
  public:
    Space getSpace() const { return Space::wrap(isl_pw_multi_aff_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    //void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    //void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_remove_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_pw_multi_aff_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_pw_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    bool hasTupleId(isl_dim_type type) const { return isl_pw_multi_aff_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_pw_multi_aff_get_tuple_name(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_pw_multi_aff_get_tuple_id(keep(), type)); }
    //void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_pw_multi_aff_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_pw_multi_aff_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_pw_multi_aff_get_dim_id(keep(), type, pos)); }
    //void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_dim_name(take(), type, pos, s)); }

    //void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_add_dims(take(), type, count)); }
#pragma endregion

#if 0
#pragma region Low-level
  public:
    isl_pw_multi_aff *takeCopy() const LLVM_OVERRIDE { return isl_pw_multi_aff_copy(keep()); }
  protected:
    void release() LLVM_OVERRIDE;
  public:
    ~Pw() { release(); }
    Pw() : Obj2() { }
    static PwMultiAff enwrap(isl_pw_multi_aff *pwmaff) { assert(pwmaff); PwMultiAff result; result.give(pwmaff); return result; }
#pragma endregion
#endif

#pragma region Creational 
    static PwMultiAff create(Set &&set, MultiAff &&maff);
    static PwMultiAff createIdentity(Space &&space);
    static PwMultiAff createFromMultiAff(MultiAff &&maff);

    static PwMultiAff createEmpty(Space &&space);
    static PwMultiAff createFromDomain(Set &&set);

    static PwMultiAff createFromSet(Set &&set);
    static PwMultiAff createFromMap(Map &&map);

    static PwMultiAff readFromStr(Ctx *ctx, const char *str) { return enwrap(isl_pw_multi_aff_read_from_str(ctx->keep(), str)); }

    //PwMultiAff copy() const { return enwrap(isl_pw_multi_aff_copy(keep())); }
    //PwMultiAff &&move() { return std::move(*this); }
#pragma endregion


#pragma region Conversion
#if 0
    /* implicit */ Pw(const PwMultiAff &that) { give(that.takeCopy()); }
    /* implicit */ Pw(PwMultiAff &&that) { give(that.take()); }
    const PwMultiAff &operator=(const PwMultiAff &that) { give(that.takeCopy()); return *this; }
    const PwMultiAff &operator=(PwMultiAff &&that) { give(that.take()); return *this; }
#endif

    Map toMap() const;

    MultiPwAff toMultiPwAff() const;
#pragma endregion


#if 0
#pragma region Spacelike
    unsigned dim(isl_dim_type type) const { return isl_pw_multi_aff_dim(keep(), type); }

    bool hasTupleId(isl_dim_type type) const { return isl_pw_multi_aff_has_tuple_id(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_pw_multi_aff_get_tuple_id(keep(), type)); }
    void setTupleId_inplace(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER{ give(isl_pw_multi_aff_set_tuple_id(take(), type, id.take())); }
    bool hasTupleName(isl_dim_type type) const { return isl_pw_multi_aff_has_tuple_name(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_pw_multi_aff_get_tuple_name(keep(), type);  }

    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_pw_multi_aff_get_dim_id(keep(), type, pos)); }
    void setDimId_inplace(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_dim_id(take(), type, pos, id.take())); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_pw_multi_aff_get_dim_name(keep(), type, pos); }

    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned n) ISLPP_INPLACE_QUALIFIER { llvm_unreachable("Missing APU function"); }
    void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) ISLPP_INPLACE_QUALIFIER { llvm_unreachable("Missing APU function"); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_drop_dims(take(), type, first, n)); }
#pragma endregion
#endif

#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth, int indent) const;
#pragma endregion


#pragma region Properties
    //Ctx *getCtx() const { return Ctx::wrap(isl_pw_multi_aff_get_ctx(keep())); }
    Space getDomainSpace() const { return Space::wrap(isl_pw_multi_aff_get_domain_space(keep())); }
    //Space getSpace() const { Space::wrap(isl_pw_multi_aff_get_space(keep())); }
#pragma endregion


    PwAff getPwAff(int pos) const { return PwAff::enwrap(isl_pw_multi_aff_get_pw_aff(keep(), pos)); }
    PwMultiAff setPwAff(int pos, PwAff &&pa) const { return enwrap(isl_pw_multi_aff_set_pw_aff(takeCopy(), pos, pa.take())); }

    Set domain() const { return Set::enwrap(isl_pw_multi_aff_domain(takeCopy())); }
    PwMultiAff scale(Val &&v) const { return enwrap(isl_pw_multi_aff_scale_val(takeCopy(), v.take())); }
    //PwMultiAff scale(Vec &&v) const { return enwrap(isl_pw_multi_aff_scale_multi_val(takeCopy(), v.take())); }

    PwMultiAff projectDomainOnParams() const { return enwrap(isl_pw_multi_aff_project_domain_on_params(takeCopy())); }
    PwMultiAff alignParams(Space &&model) const { return enwrap(isl_pw_multi_aff_align_params(takeCopy(), model.take())); }

    PwMultiAff coalesce() const { return enwrap(isl_pw_multi_aff_coalesce(takeCopy())); }
    PwMultiAff gistParams(Set &&set) const { return enwrap(isl_pw_multi_aff_gist_params(takeCopy(), set.take())); }
    PwMultiAff gist(Set &&set) const { return enwrap(isl_pw_multi_aff_gist(takeCopy(), set.take())); }

    PwMultiAff pullbackMultiAff(MultiAff &&ma) const { return enwrap(isl_pw_multi_aff_pullback_multi_aff(takeCopy(), ma.take())); }
    PwMultiAff pullbackPwMultiAff(PwMultiAff &&pma) const { return enwrap(isl_pw_multi_aff_pullback_pw_multi_aff(takeCopy(), pma.take())); }

    bool foreachPiece(const std::function<bool(Set &&,MultiAff &&)> &) const;
    unsigned nPieces() const { unsigned result=0;  foreachPiece([&result] (Set&& , MultiAff&&)->bool{ result+=1; return true; } ) ; return result; }
  }; // class Pw<MultiAff>


  static inline PwMultiAff enwrap(isl_pw_multi_aff *pwmaff) { return PwMultiAff::enwrap(pwmaff); }


  static inline bool isEqual(PwMultiAff &&pma1, PwMultiAff &&pma2) { return isl_pw_multi_aff_plain_is_equal(pma1.take(), pma2.take()); }

  static inline PwMultiAff unionAdd(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_add(pma1.take(), pma2.take())); }
  static inline PwMultiAff add(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_add(pma1.take(), pma2.take())); }
  static inline PwMultiAff sub(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_sub(pma1.take(), pma2.take())); }

  static inline PwMultiAff unionLexmin(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_lexmin(pma1.take(), pma2.take())); }
  static inline PwMultiAff unionLexmax(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_lexmax(pma1.take(), pma2.take())); }

  static inline PwMultiAff rangeProduct(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_range_product(pma1.take(), pma2.take())); }
  static inline PwMultiAff flatRangeProduct(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_flat_range_product(pma1.take(), pma2.take())); }
  static inline PwMultiAff product(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_product(pma1.take(), pma2.take())); }

  static inline PwMultiAff intersectParams(PwMultiAff &&pma, Set &&set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_params(pma.take(), set.take())); }
  static inline PwMultiAff intersectDomain(PwMultiAff &&pma, Set &&set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_domain(pma.take(), set.take())); }

} // namespace isl
#endif /* ISLPP_PWMULTIAFF_H */
