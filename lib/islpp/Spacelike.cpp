#include "islpp_impl_common.h"
#include "islpp/Spacelike.h"

#include "islpp/Space.h"

using namespace isl;


bool isl:: spacelike_matchesSpace(const Space &space, const Space &that) {
  return space.matchesSpace(that);
}


bool isl::spacelike_matchesMapSpace(const Space &space, const Space &domainSpace, const Space &rangeSpace) {
  return space.matchesMapSpace(domainSpace, rangeSpace);
}


bool isl:: spacelike_matchesSetSpace(const Space &space,  const Space &that)  {
  return space.matchesSetSpace(that);
}


bool isl:: spacelike_matchesMapSpace(const Space &space, const Space &that) {
  return space.matchesMapSpace(that);
}
