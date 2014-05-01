#ifndef ISLPP_LOCALSPACE_H
#define ISLPP_LOCALSPACE_H

#include "islpp_common.h"
#include "Spacelike.h" // baseclass of LocalSpace
#include "Obj.h" // baseclass of LocalSpace
#include <isl/space.h>
#include "Islfwd.h"
#include "Ctx.h"
#include "Space.h"

struct isl_local_space;


namespace isl {

  /// A local space is essentially a space with zero or more existentially quantified variables.
  class LocalSpace : public Obj<LocalSpace, isl_local_space>, public Spacelike<LocalSpace> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_local_space_free(takeOrNull()); }
    StructTy *addref() const { return isl_local_space_copy(keepOrNull()); }

  public:
    LocalSpace() { }

    /* implicit */ LocalSpace(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ LocalSpace(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_local_space_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    ISLPP_PROJECTION_ATTRS Space getSpace() ISLPP_PROJECTION_FUNCTION{ return Space::enwrap(isl_local_space_get_space(keep())); }
    ISLPP_PROJECTION_ATTRS LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION{ return *this; }

    ISLPP_PROJECTION_ATTRS bool isParams() ISLPP_PROJECTION_FUNCTION{ return getSpace().isParams(); }
    ISLPP_PROJECTION_ATTRS bool isSet() ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_local_space_is_set(keep())); }
    ISLPP_PROJECTION_ATTRS bool isMap() ISLPP_PROJECTION_FUNCTION{ return getSpace().isMap(); }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_local_space_dim(keep(), type); }
      //ISLPP_PROJECTION_ATTRS pos_t findDimById(isl_dim_type type, const Id &id) ISLPP_PROJECTION_FUNCTION { return isl_local_space_find_dim_by_id(keep(), type, id.keep()); }

      //ISLPP_PROJECTION_ATTRS bool        hasTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_local_space_has_tuple_name(keep(), type)); }
      //ISLPP_PROJECTION_ATTRS const char *getTupleName(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_local_space_get_tuple_name(keep(), type); }
      //ISLPP_INPLACE_ATTRS    void        setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_local_space_set_tuple_name(take(), type, s)); }
      //ISLPP_PROJECTION_ATTRS bool        hasTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return checkBool(isl_local_space_has_tuple_id(keep(), type)); }
      //ISLPP_PROJECTION_ATTRS Id          getTupleId(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return Id::enwrap(isl_local_space_get_tuple_id(keep(), type)); }
    ISLPP_INPLACE_ATTRS    void        setTupleId_inplace(isl_dim_type type, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_set_tuple_id(take(), type, id.take())); }
      //ISLPP_INPLACE_ATTRS    void        resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_local_space_reset_tuple_id(take(), type)); }

    ISLPP_PROJECTION_ATTRS bool hasDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_local_space_has_dim_name(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS const char *getDimName(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return isl_local_space_get_dim_name(keep(), type, pos); }
    ISLPP_INPLACE_ATTRS void setDimName_inplace(isl_dim_type type, pos_t pos, const char *s) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_set_dim_name(take(), type, pos, s)); }
    ISLPP_PROJECTION_ATTRS bool hasDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return checkBool(isl_local_space_has_dim_id(keep(), type, pos)); }
    ISLPP_PROJECTION_ATTRS Id getDimId(isl_dim_type type, pos_t pos) ISLPP_PROJECTION_FUNCTION{ return Id::enwrap(isl_local_space_get_dim_id(keep(), type, pos)); }
    ISLPP_INPLACE_ATTRS void setDimId_inplace(isl_dim_type type, pos_t pos, Id id) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_set_dim_id(take(), type, pos, id.take())); }
      //ISLPP_INPLACE_ATTRS void resetDimId_inplace(isl_dim_type type, pos_t pos) ISLPP_INPLACE_FUNCTION { give(isl_local_space_reset_dim_id(take(), type, pos)); }

  protected:
    ISLPP_INPLACE_ATTRS void addDims_internal(isl_dim_type type, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_add_dims(take(), type, count)); }
    ISLPP_INPLACE_ATTRS void insertDims_internal(isl_dim_type type, pos_t pos, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_insert_dims(take(), type, pos, count)); }
  public:
    //ISLPP_INPLACE_ATTRS void moveDims_inplace(isl_dim_type dst_type, pos_t dst_pos, isl_dim_type src_type, pos_t src_pos, count_t count) ISLPP_INPLACE_FUNCTION { give(isl_local_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    ISLPP_INPLACE_ATTRS void removeDims_inplace(isl_dim_type type, pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{ give(isl_local_space_drop_dims(take(), type, first, count)); }
#pragma endregion


#pragma region Conversion from isl::Space
    /* implicit */ LocalSpace(Space);
    const LocalSpace &operator=(Space);

    ///* implicit */ LocalSpace(SetSpace);
    //const LocalSpace &operator=(SetSpace);

    ///* implicit */ LocalSpace(MapSpace);
    //const LocalSpace &operator=(MapSpace);
#pragma endregion


#pragma region Creational
    //LocalSpace copy() const { return LocalSpace::wrap(takeCopy()); }
    //LocalSpace &&move() { return std::move(*this); }
#pragma endregion


#pragma region Build something basic from this space
    BasicSet emptyBasicSet() const;
    BasicSet universeBasicSet() const;

    BasicMap emptyBasicMap() const;
    BasicMap universeBasicMap() const;

    Aff createZeroAff() const;
    Aff createConstantAff(int) const;
    ISLPP_EXSITU_ATTRS Aff createConstantAff(const Int &val) ISLPP_EXSITU_FUNCTION;
    ISLPP_EXSITU_ATTRS Aff createVarAff(isl_dim_type type, pos_t pos) ISLPP_EXSITU_FUNCTION;

    Constraint createEqualityConstraint() const;
    Constraint createInequalityConstraint() const;
#pragma endregion


#pragma region Spacelike
    bool hasTupleId(isl_dim_type type) const { return getSpace().hasTupleId(type); /* isl_local_space_has_tuple_id missing */ }
    Id getTupleId(isl_dim_type type) const { return getSpace().getTupleId(type); /* isl_local_space_get_tuple_id missing */ }
#pragma endregion


    //Ctx *getCtx() const;
    //bool isSet() const;
    //count_t dim(isl_dim_type type) const { return to_count_t(isl_local_space_dim(keep(), type)); }
    //bool hasDimId(isl_dim_type type, unsigned pos) const;
    //Id getDimId(isl_dim_type type, unsigned pos) const;
    //bool hasDimName(isl_dim_type type, unsigned pos) const;
    //const char *getDimName(isl_dim_type type, unsigned pos) const;
    void setDimName(isl_dim_type type, unsigned pos, const char *s);
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);
    //Space getSpace() const;
    Aff getDiv(int pos) const;

    ISLPP_EXSITU_ATTRS LocalSpace domain() ISLPP_EXSITU_FUNCTION{ return LocalSpace::enwrap(isl_local_space_domain(takeCopy())); }
    ISLPP_EXSITU_ATTRS LocalSpace range() ISLPP_EXSITU_FUNCTION{ return LocalSpace::enwrap(isl_local_space_range(takeCopy())); }
    void fromDomain();
    //void addDims(isl_dim_type type, unsigned n);
    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);



    //typedef LocalSpaceDimIter dim_iterator;
    //typedef LocalSpaceDimIter dim_const_iterator;
    //dim_const_iterator dim_begin(DimTypeFlags filter = DimType::All) const { return LocalSpaceDimIter(this, filter, (isl_dim_type)-1, 0)++; }
    //dim_const_iterator dim_end() const { return LocalSpaceDimIter(this, (DimTypeFlags)0, isl_dim_all, 0); }

    //typedef LocalSpaceDimtypeIter dimtype_iterator;
    //dimtype_iterator dimtype_begin() { return LocalSpaceDimtypeIter(this, (isl_dim_type)-1); }
    //dimtype_iterator dimtype_end() { return LocalSpaceDimtypeIter(this, isl_dim_all); }

    //protected:
    //LocalSpace getSpacelike() const { return copy(); }


    pos_t matrixOffset(isl_dim_type type, pos_t pos, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const {
      assert(0 <= pos && pos < dim(type));
      pos_t result = pos;
      if (type == c1)
        return result;
      result += dim(c1);
      if (type == c2)
        return result;
      result += dim(c2);
      if (type == c3)
        return result;
      result += dim(c3);
      return result;
    }
    pos_t matrixOffset(isl_dim_type type, pos_t pos) const {
      return matrixOffset(type, pos, isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div);
    }

    void matrixTypePos(size_t offset, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type &type, pos_t &pos)const {
      auto n1 = dim(c1);
      if (offset < n1) {
        type = c1;
        pos = offset;
        return;
      }
      offset-= n1;

      auto n2 = dim(c2);
      if (offset < n2) {
        type = c2;
        pos = offset;
        return;
      }
      offset -= n2;

      auto n3 = dim(c3);
      if (offset < n3) {
        type = c3;
        pos = offset;
        return;
      }
      offset -= n3;

      auto n4 = dim(c4);
      if (offset < n4) {
        type = c4;
        pos = offset;
        return;
      }
      offset -= n4;
      llvm_unreachable("out of range");
    }
    void matrixTypePos(size_t offset, isl_dim_type &type, pos_t &pos) const {
      matrixTypePos(offset, isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div, type, pos);
    }


  }; // class LocalSpace


  static inline LocalSpace enwrap(__isl_take isl_local_space *ls) { return LocalSpace::enwrap(ls); }
  static inline LocalSpace enwrapCopy(__isl_keep isl_local_space *ls) { return LocalSpace::enwrapCopy(ls); }


  BasicMap lifting(LocalSpace &&ls);

  bool isEqual(const LocalSpace &ls1, const LocalSpace &ls2);

  static inline LocalSpace intersect(LocalSpace ls1, LocalSpace ls2) { return LocalSpace::enwrap(isl_local_space_intersect(ls1.take(), ls2.take())); }

  static inline bool operator==(const LocalSpace &ls1, const LocalSpace &ls2) {
    return checkBool(isl_local_space_is_equal(ls1.keep(), ls2.keep()));
  }
  static inline bool operator!=(const LocalSpace &ls1, const LocalSpace &ls2) {
    return !operator==(ls1, ls2);
  }

} // namespace isl
#endif /* ISLPP_LOCALSPACE_H */
