#include "islpp/Spacelike.h"

#include "islpp/Dim.h"
#include "islpp/Id.h"


using namespace isl;


Id Spacelike::getDimIdOrNull(isl_dim_type type, unsigned pos) const {
  if (hasDimId(type, pos))
    return getDimId(type, pos); 
  else
    return Id(); 
} 


Dim Spacelike::addDim(isl_dim_type type) {
  addDims(type, 1);
  return Dim::wrap(*this, type, dim(type)-1);
}


Dim Spacelike::addOutDim() {
  return addDim(isl_dim_out);
}


bool Spacelike::findDim(const Dim &dim, isl_dim_type &type, unsigned &pos) {
  type = dim.getType();

  //TODO: May also check tuple names

  switch (type) {
  case isl_dim_param: {
    // Param dims are identified by id
    if (!dim.hasId())
      return false;
    auto retval = findDimById(type, dim.getId());
    if (retval<0) 
      return false; // Not found
    pos = retval;
    return true;
                      } break;
  case isl_dim_in:
  case isl_dim_out: {
    // Are identified by position
    pos = dim.getPos();

    // Consistency check
    if (this->dim(type) != dim.getTypeDims())
      return false; // These are different spaces

#ifndef NDEBUG
    auto thatName = dim.getName();
    auto thisName = getDimName(type, pos);
    assert(!!thatName == !!thisName);
    assert((thatName == thisName) || strcmp(thatName, thisName) == 0 && "Give same dimensions the same id/name");

    auto thatId = dim.hasId() ? dim.getId() : Id();
    auto thisId = hasDimId(type, pos) ? getDimId(type, pos) : Id();
    assert(thatId == thisId && "Please give same dimensions the same id");
#endif

    return true;
                    } break;
  default:
    return false;
  }
}
