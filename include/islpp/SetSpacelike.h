#ifndef ISLPP_SETSPACELIKE_H
#define ISLPP_SETSPACELIKE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Space.h"


namespace isl {

  template <typename D>
  class SetSpacelike : public Spacelike<Set>/*TODO: get rid of this*/ {
    typedef D SpaceTy;

  private:
    SpaceTy *getDerived() { return static_cast<D*>(this); }
    const SpaceTy *getDerived() const { return static_cast<const D*>(this); }
    SpaceTy &derived() { return *static_cast<D*>(this); }
    const SpaceTy &derived() const { return *static_cast<const D*>(this); }

#pragma region To be implementend by Derived
    //ISLPP_PROJECTION_ATTRS SetSpace getSpace() ISLPP_PROJECTION_FUNCTION;
    //ISLPP_PROJECTION_ATTRS Space/SetSpace/LocalSpace getSpacelike() ISLPP_PROJECTION_FUNCTION;

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return derived().getSpacelike().dim(type); }
#pragma endregion

  public:
    ISLPP_PROJECTION_ATTRS count_t getParamDimCount() ISLPP_PROJECTION_FUNCTION{ return derived().dim(isl_dim_param); }
    ISLPP_PROJECTION_ATTRS count_t getDimCount() ISLPP_PROJECTION_FUNCTION { return derived().dim(isl_dim_set); }
    }; // class SetSpacelike

  } // namespace molly

#endif /* ISLPP_SETSPACELIKE_H */
