#include "islpp_impl_common.h"
#include "islpp/Spacelike.h"

#include "islpp/Space.h"

using namespace isl;


bool isl::spacelike_matchesMapSpace(const Space &space, const Space &domainSpace, const Space &rangeSpace) {
  assert(domainSpace.isSetSpace());
  assert(rangeSpace.isSetSpace());
  if (!space.isMapSpace())
    return false;
  return space.matches(isl_dim_in, domainSpace, isl_dim_set) && space.matches(isl_dim_out, rangeSpace, isl_dim_set);
}
