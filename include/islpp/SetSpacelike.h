#ifndef ISLPP_SETSPACELIKE_H
#define ISLPP_SETSPACELIKE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Space.h"


namespace isl {

  template <typename D>
  class SetSpacelike /*: public Spacelike<D>*/ {
    typedef D SpaceTy;

  private:
    SpaceTy *getDerived() { return static_cast<D*>(this); }
    const SpaceTy *getDerived() const { return static_cast<const D*>(this); }
    SpaceTy &derived() { return *static_cast<D*>(this); }
    const SpaceTy &derived() const { return *static_cast<const D*>(this); }


  public:
    //ISLPP_PROJECTION_ATTRS count_t getParamDimCount() ISLPP_PROJECTION_FUNCTION{ return derived().dim(isl_dim_param); }
    ISLPP_PROJECTION_ATTRS count_t getDimCount() ISLPP_PROJECTION_FUNCTION{ return derived().dim(isl_dim_set); }

    ISLPP_PROJECTION_ATTRS Id getTupleId() ISLPP_PROJECTION_FUNCTION{ return derived().getTupleId(isl_dim_set); }

    ISLPP_PROJECTION_ATTRS const char* getTupleName() ISLPP_PROJECTION_FUNCTION{ return derived().getTupleName(isl_dim_set); }


    ISLPP_EXSITU_ATTRS SpaceTy setTupleId(Id id) ISLPP_EXSITU_FUNCTION{ auto result = derived().copy(); result.setTupleId_inplace(id); return result; }
    ISLPP_INPLACE_ATTRS void setTupleId_inplace(Id id) ISLPP_INPLACE_FUNCTION{ derived().setTupleId_inplace(isl_dim_set, std::move(id)); }
    ISLPP_CONSUME_ATTRS SpaceTy setTupleId_consume(Id id) ISLPP_CONSUME_FUNCTION{ setTupleId_inplace(id); return move(*this); }



    //ISLPP_PROJECTION_ATTRS Id getParamDimId(pos_t pos) ISLPP_PROJECTION_FUNCTION{ return derived().getDimId(isl_dim_param, pos); }

#if 0
    ISLPP_INPLACE_ATTRS Dim addParamDim_inplace() ISLPP_INPLACE_FUNCTION{ return addDim_inplace(isl_dim_param); }
      ISLPP_INPLACE_ATTRS Dim addParamDim_inplace(Id id) ISLPP_INPLACE_FUNCTION{
      assert(findDimById(isl_dim_param, id) == -1);
      auto result = derived().addDim_inplace(isl_dim_param);
      derived().setDimId_inplace(result.getType(), result.getPos(), id.move());
      return result;
    }
#endif

    ISLPP_EXSITU_ATTRS SpaceTy addDims(count_t count) ISLPP_EXSITU_FUNCTION{ return derived().addDims(isl_dim_set, count); }
    //ISLPP_INPLACE_ATTRS DimRange addDims_inplace(count_t count) ISLPP_INPLACE_FUNCTION{ return derived().addDims_inplace(isl_dim_set, count); }
  }; // class SetSpacelike

} // namespace molly

#endif /* ISLPP_SETSPACELIKE_H */
