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
  class Pw<MultiAff> : public Obj<PwMultiAff, isl_pw_multi_aff>, public Spacelike<PwMultiAff> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_pw_multi_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_pw_multi_aff_copy(keepOrNull()); }

  public:
    Pw() { }

    /* implicit */ Pw(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Pw(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_pw_multi_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_pw_multi_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_pw_multi_aff_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    //void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    count_t dim(isl_dim_type type) const { return isl_pw_multi_aff_dim(keep(), type); }
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


#pragma region Creational 
    static PwMultiAff create(const Set &set, const MultiAff &maff) { return PwMultiAff::enwrap(isl_pw_multi_aff_alloc(set.takeCopy(), maff.takeCopy())); }
    static PwMultiAff create(Set &&set, MultiAff &&maff);
    static PwMultiAff createIdentity(Space &&space);
    static PwMultiAff createFromMultiAff(MultiAff &&maff);

    static PwMultiAff createEmpty(Space &&space);
    static PwMultiAff createFromDomain(Set &&set);

    static PwMultiAff createFromSet(Set &&set);
    static PwMultiAff createFromMap(Map &&map);

    static PwMultiAff readFromStr(Ctx *ctx, const char *str) { return enwrap(isl_pw_multi_aff_read_from_str(ctx->keep(), str)); }
#pragma endregion


#pragma region Conversion
    Map toMap() const;
    //operator Map() const { return toMap(); }

    MultiPwAff toMultiPwAff() const;
    //operator MultiPwAff() const { return toMultiPwAff(); }
#pragma endregion


#pragma region Printing
    void printProperties(llvm::raw_ostream &out, int depth, int indent) const;
#pragma endregion


#pragma region Properties
    Space getDomainSpace() const { return Space::enwrap(isl_pw_multi_aff_get_domain_space(keep())); }
#pragma endregion


    PwAff getPwAff(int pos) const { return PwAff::enwrap(isl_pw_multi_aff_get_pw_aff(keep(), pos)); }
    PwMultiAff setPwAff(int pos, PwAff &&pa) const { return enwrap(isl_pw_multi_aff_set_pw_aff(takeCopy(), pos, pa.take())); }
    void setPwAff_inplace(int pos, PwAff &&pa) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_set_pw_aff(take(), pos, pa.take())); }

    /// The set of elements for which this affine mapping is defined
    Set domain() const { return Set::enwrap(isl_pw_multi_aff_domain(takeCopy())); }
    Set getDomain() const { return Set::enwrap(isl_pw_multi_aff_domain(takeCopy())); }
    Set getRange() const;

    Space getRangeSpace() ISLPP_EXSITU_QUALIFIER { return getSpace().getRangeSpace(); }


    PwMultiAff scale(Val &&v) const { return enwrap(isl_pw_multi_aff_scale_val(takeCopy(), v.take())); }
    //PwMultiAff scale(Vec &&v) const { return enwrap(isl_pw_multi_aff_scale_multi_val(takeCopy(), v.take())); }

    PwMultiAff projectDomainOnParams() const { return enwrap(isl_pw_multi_aff_project_domain_on_params(takeCopy())); }
    PwMultiAff alignParams(Space model) ISLPP_EXSITU_QUALIFIER { return enwrap(isl_pw_multi_aff_align_params(takeCopy(), model.take())); }
    void alignParams_inplace(Space model) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_align_params(take(), model.take())); }

    PwMultiAff coalesce() const { return enwrap(isl_pw_multi_aff_coalesce(takeCopy())); }
    PwMultiAff gistParams(Set &&set) const { return enwrap(isl_pw_multi_aff_gist_params(takeCopy(), set.take())); }
    PwMultiAff gist(Set &&set) const { return enwrap(isl_pw_multi_aff_gist(takeCopy(), set.take())); }

    /// this(ma(x))
    PwMultiAff pullback(MultiAff ma) const { return enwrap(isl_pw_multi_aff_pullback_multi_aff(takeCopy(), ma.take())); }
    void pullback_inplace(MultiAff ma) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_pullback_multi_aff(take(), ma.take())); }

    PwMultiAff pullback(PwMultiAff pma) const { return enwrap(isl_pw_multi_aff_pullback_pw_multi_aff(takeCopy(), pma.take())); }
    void pullback_inplace(PwMultiAff pma) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_pullback_pw_multi_aff(takeCopy(), pma.take())); }

    //PwMultiAff pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER;
    PwMultiAff applyRange(const PwMultiAff &pma) ISLPP_EXSITU_QUALIFIER { return pma.pullback(*this); }

    bool foreachPiece(const std::function<bool(Set &&,MultiAff &&)> &) const;
    //bool foreachPiece(const std::function<void(Set ,MultiAff , bool &stop)> &) const;
    unsigned nPieces() const { unsigned result=0;  foreachPiece([&result] (Set&& , MultiAff&&)->bool { result+=1; return true; } ) ; return result; }

    Map reverse() const;
    PwMultiAff neg() const;

    PwMultiAff operator-() const { return neg(); }

    PwMultiAff unionAdd(const PwMultiAff &pma2) ISLPP_EXSITU_QUALIFIER { return PwMultiAff::enwrap(isl_pw_multi_aff_union_add(takeCopy(), pma2.takeCopy())); }
    void unionAdd_inplace(const PwMultiAff &pma2) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_union_add(take(), pma2.takeCopy())); }

    // Same isl::Map operations
    Set wrap() const;

    PwMultiAff projectOut(unsigned first, unsigned count) const;
    PwMultiAff projectOut(const DimRange &range) const;
    PwMultiAff projectOutSubspace(const Space &subspace) const;

    ISLPP_EXSITU_PREFIX PwMultiAff sublist(pos_t first, count_t count) ISLPP_EXSITU_QUALIFIER;
    ISLPP_EXSITU_PREFIX PwMultiAff sublist(Space subspace) ISLPP_EXSITU_QUALIFIER;

    PwMultiAff cast(Space space) ISLPP_EXSITU_QUALIFIER;
    void cast_inplace(Space space) ISLPP_INPLACE_QUALIFIER { give(cast(space).take()); }

    //void flatRangeProduct_inplace(PwMultiAff that) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_flat_range_product(take(), )); }
  }; // class Pw<MultiAff>


  static inline PwMultiAff enwrap(isl_pw_multi_aff *pwmaff) { return PwMultiAff::enwrap(pwmaff); }


  static inline bool isEqual(PwMultiAff &&pma1, PwMultiAff &&pma2) { return isl_pw_multi_aff_plain_is_equal(pma1.take(), pma2.take()); }

  static inline PwMultiAff unionAdd(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_add(pma1.take(), pma2.take())); }
  static inline PwMultiAff add(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_add(pma1.take(), pma2.take())); }
  static inline PwMultiAff sub(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_sub(pma1.take(), pma2.take())); }

  static inline PwMultiAff unionLexmin(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_lexmin(pma1.take(), pma2.take())); }
  static inline PwMultiAff unionLexmax(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_union_lexmax(pma1.take(), pma2.take())); }

  static inline PwMultiAff rangeProduct(PwMultiAff pma1, PwMultiAff pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_range_product(pma1.take(), pma2.take())); }
  static inline PwMultiAff flatRangeProduct(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_flat_range_product(pma1.take(), pma2.take())); }
  static inline PwMultiAff product(PwMultiAff &&pma1, PwMultiAff &&pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_product(pma1.take(), pma2.take())); }

  static inline PwMultiAff intersectParams(PwMultiAff &&pma, Set &&set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_params(pma.take(), set.take())); }
  static inline PwMultiAff intersectDomain(PwMultiAff &&pma, Set &&set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_domain(pma.take(), set.take())); }

} // namespace isl
#endif /* ISLPP_PWMULTIAFF_H */
