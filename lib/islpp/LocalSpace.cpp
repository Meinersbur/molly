#include "islpp_impl_common.h"
#include "islpp/LocalSpace.h"

#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Id.h"
#include "islpp/Aff.h"
#include "islpp/BasicMap.h"
#include "islpp/Dim.h"
#include "islpp/Aff.h"
#include "islpp/Printer.h"

#include <isl/local_space.h>

using namespace isl;

extern inline LocalSpace isl::enwrap(__isl_take isl_local_space *ls);


LocalSpace::LocalSpace(Space that)
: Obj(isl_local_space_from_space(that.take())) {
}


const LocalSpace &LocalSpace::operator=(Space that) {
  give(isl_local_space_from_space(that.take()));
  return *this;
}



void LocalSpace::dump() const {
  isl_local_space_dump(keep());
}


BasicSet LocalSpace::emptyBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_empty(isl_local_space_get_space(takeCopy())));
}

BasicSet LocalSpace::universeBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_universe(isl_local_space_get_space(takeCopy())));
}

BasicMap LocalSpace::emptyBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_empty(isl_local_space_get_space(takeCopy())));
}

BasicMap LocalSpace::universeBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(isl_local_space_get_space(takeCopy())));
}


Aff LocalSpace::createZeroAff() const {
  return Aff::enwrap(isl_aff_zero_on_domain(takeCopy()));
}


Aff LocalSpace::createConstantAff(int v) const {
  auto result = isl_aff_zero_on_domain(takeCopy());
  result = isl_aff_set_constant_si(result, v);
  return Aff::enwrap(result);
}


Constraint LocalSpace::createEqualityConstraint() const {
  return Constraint::enwrap(isl_equality_alloc(takeCopy()));
}

Constraint LocalSpace::createInequalityConstraint() const {
  return Constraint::enwrap(isl_inequality_alloc(takeCopy()));
}


void LocalSpace::setDimName(isl_dim_type type, unsigned pos, const char *s){
  give(isl_local_space_set_dim_name(take(), type, pos, s));
}
void LocalSpace::setDimId(isl_dim_type type, unsigned pos, Id &&id) {
  give(isl_local_space_set_dim_id(take(), type, pos, id.take()));
}


Aff LocalSpace::getDiv(int pos) const {
  return Aff::enwrap(isl_local_space_get_div(keep(), pos));
}


void LocalSpace::fromDomain(){
  give(isl_local_space_from_domain(take()));
}


void LocalSpace::insertDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_local_space_insert_dims(take(), type, first, n));
}
void LocalSpace::dropDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_local_space_drop_dims(take(), type, first, n));
}


void isl::LocalSpace::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


ISLPP_EXSITU_ATTRS Aff isl::LocalSpace::createConstantAff(const Int &val) ISLPP_EXSITU_FUNCTION{
  auto result = isl_aff_zero_on_domain(takeCopy());
  result = isl_aff_set_constant(result, val.keep());
  return Aff::enwrap(result);
}


ISLPP_EXSITU_ATTRS Aff isl::LocalSpace::createVarAff(isl_dim_type type, pos_t pos) ISLPP_EXSITU_FUNCTION{
  return Aff::enwrap(isl_aff_var_on_domain(takeCopy(), type, pos));
}


BasicMap isl::lifting(LocalSpace &&ls) {
  return BasicMap::enwrap(isl_local_space_lifting(ls.take()));
}


bool isl::isEqual(const LocalSpace &ls1, const LocalSpace &ls2) {
  return isl_local_space_is_equal(ls1.keep(), ls2.keep());
}
