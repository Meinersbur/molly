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
  class Pw<MultiAff> : public Obj<PwMultiAff, isl_pw_multi_aff>, public Spacelike<PwMultiAff>{

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
    //typedef isl::Obj<ObjTy, StructTy> ObjTy;
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
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_pw_multi_aff_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS Space getSpacelike() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_pw_multi_aff_get_space(keep())); }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return false; }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return true; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_pw_multi_aff_dim(keep(), type); }
      //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_pw_multi_aff_find_dim_by_id(keep(), type, id.keep()); }

    ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_pw_multi_aff_has_tuple_name(keep(), type)); }
    ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_pw_multi_aff_get_tuple_name(keep(), type); }
      //ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_set_tuple_name(take(), type, s)); }
    ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_pw_multi_aff_has_tuple_id(keep(), type)); }
    ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_pw_multi_aff_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_set_tuple_id(take(), type, id.take())); }
      //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_reset_tuple_id(take(), type)); }

      //ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_pw_multi_aff_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_pw_multi_aff_get_dim_name(keep(), type, pos); }
      //ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_set_dim_name(take(), type, pos, s)); }
      //ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_pw_multi_aff_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_pw_multi_aff_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_reset_dim_id(take(), type, pos)); }

  protected:
    //ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_add_dims(take(), type, count)); }
    //ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_insert_dims(take(), type, pos, count)); }
  public:
    //ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_pw_multi_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_drop_dims(take(), type, first, count)); }
#pragma endregion


#pragma region Creational 
      /// Use with care; isl_pw_multi_aff_alloc does not check if spaces match
    static PwMultiAff create(Set set, MultiAff maff) { return PwMultiAff::enwrap(isl_pw_multi_aff_alloc(set.takeCopy(), maff.takeCopy())); }
    static PwMultiAff createIdentity(Space &&space);
    static PwMultiAff createFromMultiAff(MultiAff &&maff);

    static PwMultiAff createEmpty(Space &&space);
    static PwMultiAff createFromDomain(Set &&set);

    static PwMultiAff createFromSet(Set &&set);
    static PwMultiAff createFromMap(Map &&map);

    static PwMultiAff readFromStr(Ctx *ctx, const char *str) { return enwrap(isl_pw_multi_aff_read_from_str(ctx->keep(), str)); }
#pragma endregion


#pragma region Conversion
    /* implicit */ Pw(Aff aff) : Obj(aff.isValid() ? aff.toPwMultiAff() : PwMultiAff()) {}
    const PwMultiAff &operator=(Aff aff) { obj_give(aff.isValid() ? aff.toPwMultiAff() : PwMultiAff()); return *this; }

    /* implicit */ Pw(MultiAff maff) : Obj(maff.isValid() ? maff.toPwMultiAff() : PwMultiAff()) {}
    const PwMultiAff &operator=(MultiAff maff) { obj_give(maff.isValid() ? maff.toPwMultiAff() : PwMultiAff()); return *this; }

    /// WARNING: This has exponential runtime; consider not having a automatic conversion
    /* implicit */ Pw(MultiPwAff that);
    const PwMultiAff &operator=(MultiPwAff that);


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
    void setPwAff_inplace(int pos, PwAff &&pa) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_set_pw_aff(take(), pos, pa.take())); }

      /// The set of elements for which this affine mapping is defined
    Set domain() const { return Set::enwrap(isl_pw_multi_aff_domain(takeCopy())); }
    Set getDomain() const { return Set::enwrap(isl_pw_multi_aff_domain(takeCopy())); }
    Set getRange() const;

    Space getRangeSpace() ISLPP_EXSITU_FUNCTION{ return getSpace().getRangeSpace(); }


    PwMultiAff scale(Val &&v) const { return enwrap(isl_pw_multi_aff_scale_val(takeCopy(), v.take())); }
    //PwMultiAff scale(Vec &&v) const { return enwrap(isl_pw_multi_aff_scale_multi_val(takeCopy(), v.take())); }

    PwMultiAff projectDomainOnParams() const { return enwrap(isl_pw_multi_aff_project_domain_on_params(takeCopy())); }
    PwMultiAff alignParams(Space model) ISLPP_EXSITU_FUNCTION{ return enwrap(isl_pw_multi_aff_align_params(takeCopy(), model.take())); }
    void alignParams_inplace(Space model) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_align_params(take(), model.take())); }

    PwMultiAff gistParams(Set &&set) const { return enwrap(isl_pw_multi_aff_gist_params(takeCopy(), set.take())); }
    ISLPP_INPLACE_ATTRS void gist_inplace(Set context)ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_gist(take(), context.take())); }
    ISLPP_EXSITU_ATTRS PwMultiAff gist(Set context) ISLPP_EXSITU_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_gist(takeCopy(), context.take())); }

    /// this(ma(x))
    PwMultiAff pullback(MultiAff ma) const { return enwrap(isl_pw_multi_aff_pullback_multi_aff(takeCopy(), ma.take())); }
    void pullback_inplace(MultiAff ma) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_pullback_multi_aff(take(), ma.take())); }

    PwMultiAff pullback(PwMultiAff pma) const { return enwrap(isl_pw_multi_aff_pullback_pw_multi_aff(takeCopy(), pma.take())); }
    void pullback_inplace(PwMultiAff pma) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_pullback_pw_multi_aff(takeCopy(), pma.take())); }

      //PwMultiAff pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER;
    ISLPP_EXSITU_ATTRS PwMultiAff applyRange(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION{ return pma.pullback(*this); }
    ISLPP_EXSITU_ATTRS Map applyRange(Map map) ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS Map applyDomain(Map map) ISLPP_EXSITU_FUNCTION;

    bool foreachPiece(const std::function<bool(Set &&, MultiAff &&)> &) const;
    //bool foreachPiece(const std::function<void(Set ,MultiAff , bool &stop)> &) const;
    unsigned nPieces() const;
    ISLPP_EXSITU_ATTRS std::vector<std::pair<Set, MultiAff>> getPieces() ISLPP_EXSITU_FUNCTION;

    Map reverse() const;
    PwMultiAff neg() const;

    PwMultiAff operator-() const { return neg(); }

    PwMultiAff unionAdd(const PwMultiAff &pma2) ISLPP_EXSITU_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_union_add(takeCopy(), pma2.takeCopy())); }
    void unionAdd_inplace(const PwMultiAff &pma2) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_union_add(take(), pma2.takeCopy())); }

      // Same isl::Map operations
    Set wrap() const;

    PwMultiAff projectOut(unsigned first, unsigned count) const;
    PwMultiAff projectOut(const DimRange &range) const;
    PwMultiAff projectOutSubspace(const Space &subspace) const;

    ISLPP_EXSITU_ATTRS PwMultiAff sublist(pos_t first, count_t count) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS PwMultiAff sublist(Space subspace) ISLPP_EXSITU_FUNCTION;

    ISLPP_EXSITU_ATTRS PwMultiAff cast(Space space) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS PwMultiAff cast(Space domainSpace, Space rangeSpace) ISLPP_EXSITU_FUNCTION{ return cast(Space::createMapFromDomainAndRange(domainSpace, rangeSpace)); }
    void cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{ give(cast(space).take()); }

    ISLPP_EXSITU_ATTRS PwMultiAff castDomain(Space domainSpace) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS PwMultiAff castRange(Space rangeSpace) ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void castRange_inplace(Space rangeSpace) ISLPP_INPLACE_FUNCTION{ obj_give(castRange(rangeSpace)); }

      //void flatRangeProduct_inplace(PwMultiAff that) ISLPP_INPLACE_QUALIFIER { give(isl_pw_multi_aff_flat_range_product(take(), )); }

    void printExplicit(llvm::raw_ostream &os, int maxElts = 8, bool newlines = false, bool formatted = false, bool sorted = true) const;
    void dumpExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const;
    void dumpExplicit() const; // In order do be callable without arguments from debugger
    std::string toStringExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const;
    std::string toStringExplicit() const;

    std::string toString() const;

    ISLPP_EXSITU_ATTRS PwMultiAff intersectDomain(Set domain) ISLPP_EXSITU_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_domain(takeCopy(), domain.take())); }
    ISLPP_INPLACE_ATTRS void intersectDomain_inplace(Set set) ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_intersect_domain(take(), set.take())); }
    ISLPP_CONSUME_ATTRS PwMultiAff intersectDomain_consume(Set domain) ISLPP_CONSUME_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_domain(take(), domain.take())); }

    ISLPP_PROJECTION_ATTRS Set range() ISLPP_PROJECTION_FUNCTION;

    ISLPP_EXSITU_ATTRS PwMultiAff coalesce() ISLPP_EXSITU_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_coalesce(takeCopy())); }
    ISLPP_INPLACE_ATTRS void coalesce_inplace() ISLPP_INPLACE_FUNCTION{ give(isl_pw_multi_aff_coalesce(take())); }
    ISLPP_CONSUME_ATTRS PwMultiAff coalesce_consume() ISLPP_CONSUME_FUNCTION{ return PwMultiAff::enwrap(isl_pw_multi_aff_coalesce(take())); }

    ISLPP_PROJECTION_ATTRS bool isEmpty() ISLPP_PROJECTION_FUNCTION{ return getDomain().isEmpty(); }

      /// this: B[] -> D[]
      /// framedomainspace: A[] B[] C[]
      /// result: (A[] B[] C[]) -> (A[] D[] C[]) 
      /// MultiAff::embedAsSubspace
    ISLPP_EXSITU_ATTRS  PwMultiAff embedIntoDomain(Space framedomainspace) ISLPP_EXSITU_FUNCTION;

    ISLPP_PROJECTION_ATTRS uint64_t getComplexity() ISLPP_PROJECTION_FUNCTION;
    ISLPP_PROJECTION_ATTRS uint64_t getOpComplexity() ISLPP_PROJECTION_FUNCTION;

    ISLPP_EXSITU_ATTRS PwMultiAff simplify() ISLPP_EXSITU_FUNCTION;
    ISLPP_INPLACE_ATTRS void simplify_inplace() ISLPP_INPLACE_FUNCTION{ give(simplify().take()); }

    ISLPP_INPLACE_ATTRS void gistUndefined_inplace() ISLPP_INPLACE_FUNCTION;
    ISLPP_EXSITU_ATTRS PwMultiAff gistUndefined() ISLPP_EXSITU_FUNCTION{ auto result = copy(); result.gistUndefined_inplace(); return result; }
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

  static inline PwMultiAff product(PwMultiAff pma1, PwMultiAff pma2) { return PwMultiAff::enwrap(isl_pw_multi_aff_product(pma1.take(), pma2.take())); }


  static inline PwMultiAff intersectParams(PwMultiAff pma, Set set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_params(pma.take(), set.take())); }
  static inline PwMultiAff intersectDomain(PwMultiAff pma, Set set) { return PwMultiAff::enwrap(isl_pw_multi_aff_intersect_domain(pma.take(), set.take())); }

  //PwMultiAff operator/(PwMultiAff pma, const Int &divisor);

} // namespace isl
#endif /* ISLPP_PWMULTIAFF_H */
