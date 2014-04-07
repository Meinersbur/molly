#include "islpp_impl_common.h"
#include "islpp/ParamSpace.h"

#include "islpp/MultiAff.h"

using namespace isl;


isl::MultiAff isl::ParamSpace::createIdentityMultiAff(count_t dims) {
  auto mapspace = isl_space_add_dims(takeCopy(), isl_dim_in, dims);
  mapspace = isl_space_add_dims(mapspace, isl_dim_out, dims);
  return MultiAff::enwrap(isl_multi_aff_identity(mapspace));
}
