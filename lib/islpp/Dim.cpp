#include "islpp_impl_common.h"
#include "islpp/Dim.h"

#include "islpp/Spacelike.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include <isl/space.h>
#include <isl/map.h>
#include <llvm/Support/ErrorHandling.h>

using namespace isl;


Dim Dim::enwrap(const Space &space, isl_dim_type type, unsigned pos) {
  return Dim(space.takeCopy(), type, pos);
}


Dim Dim::enwrap(isl_space *space, isl_dim_type type, unsigned pos) {
  return Dim(space, type, pos);
}


Dim Dim::enwrap(const LocalSpace &localspace, isl_dim_type type, unsigned pos) {
  return Dim(localspace.takeCopy(), type, pos);
}


Dim Dim::enwrap(isl_local_space *localspace, isl_dim_type type, unsigned pos) {
  return Dim(localspace, type, pos);
}

