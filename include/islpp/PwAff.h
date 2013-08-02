#ifndef ISLPP_PWAFF_H
#define ISLPP_PWAFF_H

#include "islpp_common.h"
#include "Multi.h"
#include "Pw.h"
#include "Id.h"
#include <cassert>
#include <string>
#include <functional>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/aff.h>
#include "Obj.h"
#include "Spacelike.h"
#include "Ctx.h"
#include "Space.h"
#include "Set.h"

struct isl_pw_aff;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
  class Space;
  class Aff;
  class Set;
  class LocalSpace;
  class Id;
  class Int;
  class Map;
} // namespace isl


namespace isl {

  template<> 
  class Pw<Aff> : public Obj3<Pw<Aff>, isl_pw_aff>, public Spacelike3<Pw<Aff>> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_pw_aff_free(takeOrNull()); }
    StructTy *addref() const { return isl_pw_aff_copy(keepOrNull()); }

  public:
    Pw() { }
    //static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    /* implicit */ Pw(const ObjTy &that) : Obj3(that) { }
    /* implicit */ Pw(ObjTy &&that) : Obj3(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_pw_aff_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_pw_aff_dump(keep()); }
#pragma endregion


#pragma region isl::Spacelike3
   friend class isl::Spacelike3<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_pw_aff_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_set_dim_id(take(), type, pos, id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_pw_aff_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_pw_aff_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_pw_aff_has_tuple_id(keep(), type); }
    //const char *getTupleName(isl_dim_type type) const { return isl_pw_aff_get_tuple_name(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_pw_aff_get_tuple_id(keep(), type)); }
    //void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_set_tuple_name(take(), type, s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_pw_aff_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_pw_aff_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_pw_aff_get_dim_id(keep(), type, pos)); }
    //void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Creational
    static PwAff createFromAff(Aff &&aff); 
    static PwAff createEmpty(Space &&space);
    static PwAff create(const Set &set, const Aff &aff) { return PwAff::enwrap(isl_pw_aff_alloc(set.takeCopy(), aff.takeCopy())); }
    static PwAff create(Set &&set, Aff &&aff);
    static PwAff createZeroOnDomain(LocalSpace &&space);
    static PwAff createVarOnDomain(LocalSpace &&ls, isl_dim_type type, unsigned pos);
    static PwAff createIndicatorFunction(Set &&set);

    static PwAff readFromStr(Ctx *ctx, const char *str);

    //PwAff copy() const { return PwAff::enwrap(takeCopy()); }
    //PwAff &&move() { return std::move(*this); }
#pragma endregion


#pragma region Conversion
    // from Aff
    Pw(const Aff &aff) : Obj3( isl_pw_aff_from_aff(aff.takeCopy()) ) {}
    Pw(Aff &&aff) : Obj3(isl_pw_aff_from_aff(aff.take())) {}
    const PwAff &operator=(const Aff &aff) LLVM_LVALUE_FUNCTION { give(isl_pw_aff_from_aff(aff.takeCopy())); return *this; }
    const PwAff &operator=(Aff &&aff) LLVM_LVALUE_FUNCTION { give(isl_pw_aff_from_aff(aff.take())); return *this; }

    Map toMap() const;

    //Expr toExpr() const;
#pragma endregion


    Space getDomainSpace() const;
    bool isEmpty() const;


    bool involvesDim(isl_dim_type type, unsigned first, unsigned n) const;
    bool isCst() const;

    void alignParams(Space &&model);


    void neg();
    void ceil();
    void floor();
    void mod(const Int &mod);

    void intersectParams(Set &&set);
    void intersetDomain(Set &&set);

    void scale(const Int &f);
    void scaleDown(const Int &f);

    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);

    void coalesce();
    void gist(Set &&context);
    void gistParams(Set &&context);

    void pullback(MultiAff &&ma);
    void pullback(PwMultiAff &&pma);

    int nPiece() const;
    bool foreachPiece(std::function<bool(Set,Aff)> fn) const;
    std::vector<std::pair<Set,Aff>> getPieces() const;

    //void addDisjoint_inplace() ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_add_disjoint(  )); }

    void unionMin_inplace(const PwAff &pwaff2) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_union_min(take(), pwaff2.takeCopy())); }
    PwAff unionMin(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_min(takeCopy(), pwaff2.takeCopy())); }

    void unionMax_inplace(const PwAff &pwaff2) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_union_max(take(), pwaff2.takeCopy())); }
    PwAff unionMax(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_max(takeCopy(), pwaff2.takeCopy())); }

    void unionAdd_inplace(const PwAff &pwaff2) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_union_add(take(), pwaff2.takeCopy())); }
    PwAff unionAdd(const PwAff &pwaff2) const { return PwAff::enwrap(isl_pw_aff_union_add(takeCopy(), pwaff2.takeCopy())); }

    // FIXME: getDomain()?
    Set domain() const { return Set::enwrap(isl_pw_aff_domain(takeCopy())); }
    Set getDomain() const { return Set::enwrap(isl_pw_aff_domain(takeCopy())); }

    void mul_inplace (const PwAff &multiplier) ISLPP_INPLACE_QUALIFIER { give(isl_pw_aff_mul(take(), multiplier.takeCopy())); }
    PwAff mul(const PwAff &multiplier) const { return PwAff::enwrap(isl_pw_aff_mul(takeCopy(), multiplier.takeCopy())); }
  }; // class PwAff


  static inline PwAff enwrap(__isl_take isl_pw_aff *obj) { return PwAff::enwrap(obj); }
  static inline PwAff enwrapCopy(__isl_keep isl_pw_aff *obj) { return PwAff::enwrapCopy(obj); }


  bool plainIsEqual(PwAff pwaff1, PwAff pwaff2);

  PwAff unionMin(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff unionMax(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff unionAdd(PwAff &&pwaff1, PwAff &&pwaff2);

  Set domain(PwAff &&pwaff);

  PwAff min(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff max(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff mul(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff div(PwAff &&pwaff1, PwAff &&pwaff2);

  static inline PwAff add(PwAff &&pwaff1, PwAff &&pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.take(), pwaff2.take())); }
  static inline PwAff add(PwAff &&pwaff1, const PwAff &pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.take(), pwaff2.takeCopy())); }
  static inline  PwAff add(const PwAff &pwaff1, PwAff &&pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.takeCopy(), pwaff2.take())); }
  static inline   PwAff add(const PwAff &pwaff1, const PwAff &pwaff2) { return PwAff::enwrap(isl_pw_aff_add(pwaff1.takeCopy(), pwaff2.takeCopy())); }
  PwAff add(PwAff &&pwaff1, int pwaff2);
  static inline PwAff add(const PwAff &lhs, int rhs) {  return add(lhs.copy(), rhs);  }
  static inline PwAff add(int lhs, PwAff && rhs) {  return add(std::move(rhs), lhs);  }
  static inline PwAff add(int lhs, const PwAff & rhs) {  return add(rhs, lhs);  }
  static inline PwAff operator+(PwAff &&pwaff1, PwAff &&pwaff2) { return add(std::move(pwaff1), std::move(pwaff2)); }
  static inline PwAff operator+(PwAff &&pwaff1, const PwAff &pwaff2) { return add(std::move(pwaff1), pwaff2); }
  static inline PwAff operator+(const PwAff &pwaff1, PwAff &&pwaff2) { return add(pwaff1, std::move(pwaff2)); }
  static inline PwAff operator+(const PwAff &pwaff1, const PwAff &pwaff2) { return add(pwaff1, pwaff2); }
  static inline PwAff operator+(PwAff &&lhs, int rhs) { return add(std::move(lhs), rhs); }
  static inline PwAff operator+(const PwAff &lhs, int rhs) { return add(lhs, rhs); }
  static inline PwAff operator+(int lhs, PwAff && rhs) { return add(lhs, std::move(rhs));  }
  static inline PwAff operator+(int lhs, const PwAff & rhs) { return add(lhs, rhs);  }

  static inline PwAff &operator+=(PwAff &lhs, const PwAff &rhs) { lhs.give(isl_pw_aff_add(lhs.take(), rhs.takeCopy())); return lhs; }

  PwAff sub(PwAff &&pwaff1, PwAff &&pwaff2);
  PwAff sub(const PwAff &pwaff1, PwAff &&pwaff2);
  PwAff sub(PwAff &&pwaff1, const PwAff &pwaff2);
  PwAff sub(const PwAff &pwaff1, const PwAff &pwaff2);
  PwAff sub(PwAff &&lhs, int rhs);
  static inline PwAff sub(const PwAff &lhs, int rhs) {  return sub(lhs, rhs);  }
  static inline PwAff sub(int lhs, PwAff &&rhs){  return sub(lhs, std::move(rhs));  }
  static inline PwAff sub(int lhs, const PwAff & rhs) {  return sub(lhs, rhs);  }
  static inline PwAff operator-(PwAff &&pwaff1, PwAff &&pwaff2) { return sub(std::move(pwaff1), std::move(pwaff2)); }
  static inline PwAff operator-(PwAff &&pwaff1, const PwAff &pwaff2) { return sub(std::move(pwaff1), pwaff2); }
  static inline PwAff operator-(const PwAff &pwaff1, PwAff &&pwaff2) { return sub(pwaff1, std::move(pwaff2)); }
  static inline PwAff operator-(const PwAff &pwaff1, const PwAff &pwaff2) { return sub(pwaff1, pwaff2); }
  static inline PwAff operator-(PwAff &&lhs, int rhs) { return sub(std::move(lhs), rhs); }
  static inline PwAff operator-(const PwAff &lhs, int rhs) { return sub(lhs, rhs); }
  static inline PwAff operator-(int lhs, PwAff && rhs) {  return sub(lhs, std::move(rhs));  }
  static inline PwAff operator-(int lhs, const PwAff & rhs) {  return sub(lhs, rhs);  }

  static inline PwAff operator-(const Aff &lhs, const PwAff &rhs) { return PwAff::enwrap(isl_pw_aff_sub( isl_pw_aff_from_aff(lhs.takeCopy()), rhs.takeCopy() )); }

  static inline const PwAff operator*(const PwAff &lhs, const PwAff &rhs) { return PwAff::enwrap(isl_pw_aff_mul(lhs.takeCopy(), rhs.takeCopy())); }
  static inline const PwAff &operator*=(PwAff &lhs, const PwAff &rhs) { lhs.mul_inplace(rhs); return lhs; }

  PwAff tdivQ(PwAff &&pa1, PwAff &&pa2);
  PwAff tdivR(PwAff &&pa1, PwAff &&pa2);

  PwAff cond(PwAff &&cond, PwAff &&pwaff_true, PwAff &&pwaff_false);

  /// Return a set containing those elements in the domain of pwaff where it is non-negative.
  Set nonnegSet(PwAff &pwaff); 
  Set zeroSet(PwAff &pwaff); // alternative name: preimageOfZero
  Set nonXeroSet(PwAff &pwaff);

  Set eqSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set neSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set leSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set ltSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set geSet(PwAff &&pwaff1, PwAff &&pwaff2);
  Set gtSet(PwAff &&pwaff1, PwAff &&pwaff2);

  static inline PwAff setTupleId(PwAff &&pwaff, isl_dim_type type, Id &&id) { return PwAff::enwrap(isl_pw_aff_set_tuple_id(pwaff.take(), type, id.take())); }
} // namespace isl
#endif /* ISLPP_PWAFF_H */
