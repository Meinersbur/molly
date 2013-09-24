#include "islpp_impl_common.h"
#include "islpp/Dim.h"

#include "islpp/Spacelike.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include <isl/space.h>
#include <isl/map.h>
#include <llvm/Support/ErrorHandling.h>

using namespace isl;


#if 0
     Dim Dim::enwrap(const Space &space, isl_dim_type type, unsigned pos) {
       return Dim(type, pos, space.hasDimId(type, pos) ? space.getDimId(type, pos) : Id(), space.dim(type), space.hasTupleId(type) ? space.getTupleId(type) : Id());
     }


     Dim Dim::enwrap(const LocalSpace &space, isl_dim_type type, unsigned pos) {
         return Dim(type, pos, space.hasDimId(type, pos) ? space.getDimId(type, pos) : Id(), space.dim(type), space.hasTupleId(type) ? space.getTupleId(type) : Id());
     }


  

#else
Dim Dim::enwrap( isl_dim_type type, unsigned pos,  Space space) {
  return Dim( type, pos, space.take());
}


Dim Dim::enwrap( isl_dim_type type, unsigned pos, isl_space *space) {
  return Dim(type, pos, isl_space_copy(space));
}

#endif
