#include "islpp_impl_common.h"
#include "islpp/DimRange.h"

#include "islpp/Space.h"

using namespace isl;
using namespace llvm;
using namespace std;


DimRange DimRange::enwrap(isl_dim_type type, unsigned first, unsigned count, __isl_take isl_space *space) { 
  return DimRange(type, first, count, space); 
}


DimRange DimRange::enwrap(isl_dim_type type, unsigned first, unsigned count, Space &&space) { 
  return DimRange(type, first, count, space.take()); 
}


DimRange DimRange::enwrap(isl_dim_type type, unsigned first, unsigned count, const Space &space) { 
  return DimRange(type, first, count, space.takeCopy()); 
}


Space DimRange::getSpace() const { 
  return Space::enwrapCopy(space);
}
