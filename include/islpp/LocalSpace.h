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
    Space getSpace() const { return Space::enwrap(isl_local_space_get_space(keep())); }
    LocalSpace getSpacelike() const { return *this; }

  protected:
    void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_FUNCTION { give(isl_local_space_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_FUNCTION { give(isl_local_space_set_dim_id(take(), type, pos, id.take())); }

    // optional
    //bool isSet() const { return isSetSpace(); }
    //bool isMap() const { return isMapSpace(); }

  public:
    //void resetTupleId_inplace(isl_dim_type type) ISLPP_INPLACE_FUNCTION { give(isl_local_space_reset_tuple_id(take(), type)); }
    //void resetDimId_inplace(isl_dim_type type, unsigned pos) ISLPP_INPLACE_FUNCTION { give(isl_local_space_reset_dim_id(take(), type, pos)); }

    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_local_space_insert_dims(take(), type, pos, count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_local_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_local_space_drop_dims(take(), type, first, count)); }


    // optional, default implementation exist
    count_t dim(isl_dim_type type) const { return isl_local_space_dim(keep(), type); }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_local_space_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_local_space_has_tuple_id(keep(), type); }
    //const char *getTupleName(isl_dim_type type) const { return isl_local_space_get_tuple_name(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_local_space_get_tuple_id(keep(), type)); }
    //void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_local_space_set_tuple_name(take(), type, s)); }

    bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_local_space_has_dim_id(keep(), type, pos); }
    bool hasDimName(isl_dim_type type, unsigned pos) const { return isl_local_space_has_dim_name(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_local_space_get_dim_name(keep(), type, pos); }
    Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_local_space_get_dim_id(keep(), type, pos)); }
    void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_FUNCTION { give(isl_local_space_set_dim_name(take(), type, pos, s)); }

    void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_local_space_add_dims(take(), type, count)); }
#pragma endregion


#pragma region Conversion from isl::Space
    /* implicit */ LocalSpace(Space &&);
    /* implicit */ LocalSpace(const Space &);

    const LocalSpace &operator=(Space &&);
    const LocalSpace &operator=(const Space &that);
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

    Constraint createEqualityConstraint() const;
    Constraint createInequalityConstraint() const;
#pragma endregion


#pragma region Spacelike
    bool hasTupleId(isl_dim_type type) const { return getSpace().hasTupleId(type); /* isl_local_space_has_tuple_id missing */ }
    Id getTupleId(isl_dim_type type) const { return getSpace().getTupleId(type); /* isl_local_space_get_tuple_id missing */ }
#pragma endregion


    //Ctx *getCtx() const;
    bool isSet() const;
    //count_t dim(isl_dim_type type) const { return to_count_t(isl_local_space_dim(keep(), type)); }
    //bool hasDimId(isl_dim_type type, unsigned pos) const;
    //Id getDimId(isl_dim_type type, unsigned pos) const;
    //bool hasDimName(isl_dim_type type, unsigned pos) const;
    //const char *getDimName(isl_dim_type type, unsigned pos) const;
    void setDimName(isl_dim_type type, unsigned pos, const char *s);
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);
    //Space getSpace() const;
    Aff getDiv(int pos) const;

    void domain();
    void range();
    void fromDomain();
    void addDims(isl_dim_type type, unsigned n);
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
  }; // class LocalSpace

  
 static inline LocalSpace enwrap(__isl_take isl_local_space *ls) { return LocalSpace::enwrap(ls); }
 static inline LocalSpace enwrapCopy(__isl_take isl_local_space *ls) { return LocalSpace::enwrapCopy(ls); }


  BasicMap lifting(LocalSpace &&ls);

  bool isEqual(const LocalSpace &ls1, const LocalSpace &ls2);
  LocalSpace intersect( LocalSpace &&ls1,  LocalSpace &&ls2);

} // namepsace isl
#endif /* ISLPP_LOCALSPACE_H */
