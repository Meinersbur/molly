#ifndef MOLLY_AFFEXPR_H
#define MOLLY_AFFEXPR_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Obj.h"
#include "Spacelike.h"
#include "Space.h"
#include "LocalSpace.h"
#include <isl/aff.h>
#include "Aff.h"


namespace isl {

  /// Just like an isl::Aff, but on isl_dim_in and isl_dim_out dimensions
  /// Most useful to construct constraints
  /// Implementation is just an isl_aff, but on a nested space containing domain and range
  class AffExpr : private Obj<AffExpr, isl_aff>, public Spacelike<AffExpr>, public AffCommon<Aff> {

  private:
    static isl_dim_type inmapType(isl_dim_type exprType) {
      if (exprType == isl_dim_in)
        return isl_dim_set;
      return exprType;
    }


    unsigned inmapPos(isl_dim_type exprType, unsigned exprPos) const {
      if (exprType == isl_dim_out) 
        return dim(isl_dim_in) + exprPos;
      return exprPos;
    }

    isl_dim_type outmapType(isl_dim_type affType, unsigned affPos) const {
      if (affType == isl_dim_in) {
        return affPos >= dim(isl_dim_in) ? isl_dim_out : isl_dim_in;
      }
      return affType;
    }

    unsigned outmapPos(isl_dim_type affType, unsigned affPos) const {
      if (affType == isl_dim_in) {
        auto nDims = dim(isl_dim_in);
        return affPos >= nDims ? affPos-nDims : affPos;
      }
      return affPos;
    }

    AffExpr(Aff &&aff) : Obj(aff.takeOrNull()) { assert(isNull() || getSpace().isWrapping()); }
    AffExpr(__isl_take isl_aff *aff) : Obj(aff) { assert(isNull() || getSpace().isWrapping()); }

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  public:
    AffExpr() { }

    /* implicit */ AffExpr(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ AffExpr(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    void print(llvm::raw_ostream &out) const;
#pragma endregion


#pragma region isl::Spacelike
    friend class isl::Spacelike<ObjTy>;
  public:
    Space getSpace() const { return Space::enwrap(isl_aff_get_domain_space(keep())).unwrap(); }
    //LocalSpace getLocalSpace() const { return LocalSpace::wrap(isl_aff_get_local_space(keep())).unwrap(); }
    //LocalSpace getSpacelike() const { return getLocalSpace(); }
    Space getSpacelike() const { return getSpace(); }

  protected:
    //void setTupleId_internal(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER { give(isl_aff_set_tuple_id(take(), type, id.take())); }
    void setDimId_internal(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_FUNCTION { give(isl_aff_set_dim_id(take(), inmapType(type), inmapPos(type, pos), id.take())); }

  public:
    void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_aff_insert_dims(take(), inmapType(type), inmapPos(type, pos), count)); }
    //void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_aff_move_dims(take(), dst_type, dst_pos, src_type, src_pos, count)); }
    void removeDims_inplace(isl_dim_type type, unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION { give(isl_aff_drop_dims(take(), inmapType(type), inmapPos(type, first), count)); }


    // optional, default implementation exist
    unsigned dim(isl_dim_type type) const { 
      if (type==isl_dim_in || type==isl_dim_out) {
        auto space = isl_space_unwrap(isl_aff_get_domain_space(keep()));
        auto result = isl_space_dim(space, type);
        isl_space_free(space);
        return result;
      }
      return isl_aff_dim(keep(), type);
    }
    //int findDimById(isl_dim_type type, const Id &id) const { return isl_aff_find_dim_by_id(keep(), type, id.keep()); }

    //bool hasTupleId(isl_dim_type type) const { return isl_aff_has_tuple_id(keep(), type); }
    //const char *getTupleName(isl_dim_type type) const { return isl_aff_get_tuple_name(keep(), type); }
    //Id getTupleId(isl_dim_type type) const { return Id::enwrap(isl_aff_get_tuple_id(keep(), type)); }
    //void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_aff_set_tuple_name(take(), type, s)); }

    //bool hasDimId(isl_dim_type type, unsigned pos) const { return isl_aff_has_dim_id(keep(), type, pos); }
    const char *getDimName(isl_dim_type type, unsigned pos) const { return isl_aff_get_dim_name(keep(), inmapType(type), inmapPos(type, pos)); }
    //Id getDimId(isl_dim_type type, unsigned pos) const { return Id::enwrap(isl_aff_get_dim_id(keep(), type, pos)); }
    //void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { give(isl_aff_set_dim_name(take(), type, pos, s)); }

    //void addDims_inplace(isl_dim_type type, unsigned count) ISLPP_INPLACE_QUALIFIER { give(isl_aff_add_dims(take(), inmapType(type), inmapPos(count))); }
#pragma endregion


    Aff asAff() const { return Aff::enwrap(takeCopy()); }


#pragma region Creational
    static AffExpr createZero(const Space &space) { return AffExpr(space.wrap().createZeroAff()); }
    static AffExpr createVar(const Space &space, isl_dim_type type, unsigned pos) { 
      auto wrappedSpace = space.wrap();
      if (type==isl_dim_in) {
        type = isl_dim_set;
      } else if (type == isl_dim_out) {
        pos = space.getInDimCount() + pos;
      }
      return AffExpr(wrappedSpace.createVarAff(type, pos)); 
    }
#pragma endregion


  }; // class AffExpr

} // namespace isl
#endif
