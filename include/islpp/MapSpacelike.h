#ifndef ISLPP_MAPSPACELIKE_H
#define ISLPP_MAPSPACELIKE_H

#include "islpp_common.h"
#include "Spacelike.h"


namespace isl {

  template <typename D>
  class MapSpacelike : public Spacelike<Map>/*TODO: get rid of this*/ {
    typedef D SpaceTy;

  private:
    SpaceTy *getDerived() { return static_cast<D*>(this); }
    const SpaceTy *getDerived() const { return static_cast<const D*>(this); }
    SpaceTy &derived() { return *static_cast<D*>(this); }
    const SpaceTy &derived() const { return *static_cast<const D*>(this); }

#pragma region To be implementend by Derived
    //ISLPP_PROJECTION_ATTRS MapSpace getSpace() ISLPP_PROJECTION_FUNCTION;
    //ISLPP_PROJECTION_ATTRS Space/MapSpace/LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION;

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return derived().getSpacelike().dim(type); }
#pragma endregion

  public:
    ISLPP_PROJECTION_ATTRS count_t getParamDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_param); }
    ISLPP_PROJECTION_ATTRS count_t getInDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_in); }
    ISLPP_PROJECTION_ATTRS count_t getDomainDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_in); }
    ISLPP_PROJECTION_ATTRS count_t getOutDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_out); }
    ISLPP_PROJECTION_ATTRS count_t getRangeDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_out); }


  }; // class MapSpacelike

} // namespace molly

#endif /* ISLPP_MAPSPACELIKE_H */
