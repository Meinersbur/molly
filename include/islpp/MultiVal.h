#ifndef ISLPP_MULTIVAL_H
#define ISLPP_MULTIVAL_H

#include "islpp_common.h"
#include "Islfwd.h"

#include "Ctx.h"
#include "Multi.h"
#include "Val.h"
#include "Obj.h"
#include "Spacelike.h"
#include <isl/val.h>

#include "Space.h"
#include "ValList.h"

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {

  template<>
  class Multi<Val> : public Obj<MultiVal, isl_multi_val> , public Spacelike<MultiVal> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_multi_val_free(takeOrNull()); }
    StructTy *addref() const { return isl_multi_val_copy(keepOrNull()); }

  public:
    Multi() { }

    /* implicit */ Multi(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Multi(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_multi_val_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_multi_val_get_space(keep())); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_set_dim_id(take(), type, pos, id.take())); }

    // optional
    bool isSet() const { return false; }
    bool isMap() const { return true; }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { return isl_multi_val_dim(keep(), type); }
    int findDimById(isl_dim_type type, const Id &id) const { return isl_multi_val_find_dim_by_id(keep(), type, id.keep()); }

    bool hasTupleId(isl_dim_type type) const { return isl_multi_val_has_tuple_id(keep(), type); }
    const char *getTupleName(isl_dim_type type) const { return isl_multi_val_get_tuple_name(keep(), type); }
    Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_multi_val_get_tuple_id(keep(), type)); }
    void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_multi_val_has_dim_id(keep(), type, pos); }
    //const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_multi_val_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_multi_val_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Construction
    static MultiVal zero(Space space) { return MultiVal::enwrap(isl_multi_val_zero(space.take())); }

    static MultiVal fromValList(Space space, ValList list) { return MultiVal::enwrap(isl_multi_val_from_val_list(space.take(), list.take())); }
    static MultiVal fromRange(MultiVal multi) { return MultiVal::enwrap(isl_multi_val_from_range(multi.take())); }
#pragma endregion

    //ISLPP_EXSITU_ATTRS count_t dim(isl_dim_type type) ISLPP_EXSITU_FUNCTION { return isl_multi_val_dim(keep(), type); }

    //ISLPP_EXSITU_ATTRS Space getSpace() ISLPP_EXSITU_FUNCTION { return Space::enwrap(isl_multi_val_get_space(keep())); }
    ISLPP_EXSITU_ATTRS Space getDomainSpace() ISLPP_EXSITU_FUNCTION { return Space::enwrap(isl_multi_val_get_domain_space(keep())); }

    //ISLPP_EXSITU_ATTRS int findDimById(isl_dim_type type, const Id &id) ISLPP_EXSITU_FUNCTION { return isl_multi_val_find_dim_by_id(keep(), type, id.keep()); }

    ISLPP_EXSITU_ATTRS Val getVal(int pos) ISLPP_EXSITU_FUNCTION { return Val::enwrap(isl_multi_val_get_val(keep(), pos)); }
    ISLPP_EXSITU_ATTRS MultiVal setVal(int pos, Val el) ISLPP_EXSITU_FUNCTION { return MultiVal::enwrap(isl_multi_val_set_val(takeCopy(), pos, el.take())); }
    ISLPP_INPLACE_ATTRS void setVal_inplace(int pos, Val el) ISLPP_INPLACE_FUNCTION { give(isl_multi_val_set_val(take(), pos, el.take())); }

    ISLPP_EXSITU_ATTRS char *toStr() ISLPP_EXSITU_FUNCTION { return isl_multi_val_to_str(keep()); }
  }; // class Multi<Val>

  static inline MultiVal enwrap(__isl_take isl_multi_val *obj) { return MultiVal::enwrap(obj); }
  static inline MultiVal enwrapCopy(__isl_keep isl_multi_val *obj) { return MultiVal::enwrapCopy(obj); }


  static bool plainIsEqual(const MultiVal &lhs, const MultiVal &rhs) { return checkBool(isl_multi_val_plain_is_equal(lhs.keep(), rhs.keep())); }

  static MultiVal rangeSplice(MultiVal multi1, unsigned pos, MultiVal multi2) { return MultiVal::enwrap(isl_multi_val_range_splice(multi1.take(), pos, multi2.take())); }
  static MultiVal splice(MultiVal multi1, unsigned in_pos, unsigned out_pos, MultiVal multi2) { return MultiVal::enwrap(isl_multi_val_splice(multi1.take(), in_pos, out_pos, multi2.take())); }

  static MultiVal flatRangeProduct(MultiVal multi1, MultiVal multi2) { return MultiVal::enwrap(isl_multi_val_flat_range_product(multi1.take(), multi2.take())); }
  static MultiVal rangeProduct(MultiVal multi1, MultiVal multi2) { return MultiVal::enwrap(isl_multi_val_range_product(multi1.take(), multi2.take())); }
  static MultiVal product(MultiVal multi1, MultiVal multi2) { return MultiVal::enwrap(isl_multi_val_range_product(multi1.take(), multi2.take())); }
  static MultiVal scale(MultiVal multi, Val val) { return MultiVal::enwrap(isl_multi_val_scale_val(multi.take(), val.take())); }
  static MultiVal scale(MultiVal multi, MultiVal mv) { return MultiVal::enwrap(isl_multi_val_scale_multi_val(multi.take(), mv.take())); }
  static MultiVal scaleDown(MultiVal multi, MultiVal mv) { return MultiVal::enwrap(isl_multi_val_scale_down_multi_val(multi.take(), mv.take())); }

  static MultiVal alignParams(MultiVal multi, Space space) { return MultiVal::enwrap(isl_multi_val_align_params(multi.take(), space.take())); }

 static MultiVal add(MultiVal mv, Val v) { return MultiVal::enwrap(isl_multi_val_add_val(mv.take(), v.take())); }
 static MultiVal mod(MultiVal mv, Val v) { return MultiVal::enwrap(isl_multi_val_add_val(mv.take(), v.take())); }

} // namespace isl
#endif /* ISLPP_MULTIVAL_H */
