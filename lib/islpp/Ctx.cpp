#include "islpp/Ctx.h"

#include "islpp/BasicSet.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Constraint.h"

#include <isl/ctx.h>
#include <isl/options.h>
#include <isl/space.h>

using namespace isl;
using namespace std;


Ctx::Ctx() {
  this->ctx = isl_ctx_alloc();
#ifndef NDEBUG
  setOnError(OnError::Abort);
#endif
}


Ctx::~Ctx() {
  if (ctx)
    isl_ctx_free(ctx);
}

isl_error Ctx::getLastError() const {
  return isl_ctx_last_error(keep());
}


void Ctx::resetLastError() {
  isl_ctx_reset_error(keep());
}


void Ctx::setOnError(OnErrorEnum val){
  isl_options_set_on_error(keep(), val);
}


OnErrorEnum Ctx::getOnError() const {
  return static_cast<OnErrorEnum>( isl_options_get_on_error(keep()));
}


Space Ctx::createSpace(unsigned nparam, unsigned dim) {
  isl_space *space = isl_space_set_alloc(ctx, nparam, dim);
  return Space::wrap(space);
}


BasicSet Ctx::createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths) {
  auto dims = lengths.size();
  Space space = createSpace(0, dims);
  BasicSet set = BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = Constraint::createInequality(space.copy());
    ge.setConstant(0);
    ge.setCoefficient(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = Constraint::createInequality(move(space));
    lt.setConstant(lengths[d]);
    lt.setCoefficient(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }
  return set;
}
