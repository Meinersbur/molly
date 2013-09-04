#include "islpp_impl_common.h"
#include "islpp/LocalSpace.h"

#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Id.h"
#include "islpp/Aff.h"
#include "islpp/BasicMap.h"
#include "islpp/Dim.h"
#include "islpp/Aff.h"

#include <isl/local_space.h>

using namespace isl;

extern inline LocalSpace isl::enwrap(__isl_take isl_local_space *ls);


isl_local_space *LocalSpace::takeCopy() const {
  return isl_local_space_copy(space);
}


void LocalSpace::give(isl_local_space *space)  {
  if (this->space)
    isl_local_space_free(this->space);
  this->space = space; 
}


LocalSpace::~LocalSpace() {
  if (space)
    isl_local_space_free(space);
}


const LocalSpace &LocalSpace::operator=(const Space &that) {
  assert(!this->space);
  this->space = isl_local_space_from_space(isl_space_copy(that.keep()));
  return *this; 
}



LocalSpace::LocalSpace(Space &&that) {
  this->space = isl_local_space_from_space(that.take());
}


const LocalSpace &LocalSpace::operator=(Space &&that) {
  isl_local_space_free(this->space);
  this->space = isl_local_space_from_space(that.take());
  return *this;
}

BasicSet  LocalSpace::emptyBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_empty(isl_local_space_get_space( takeCopy())));
}

BasicSet  LocalSpace::universeBasicSet() const {
  return BasicSet::enwrap(isl_basic_set_universe(isl_local_space_get_space( takeCopy())));
}

BasicMap  LocalSpace::emptyBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_empty(isl_local_space_get_space( takeCopy())));
}

BasicMap  LocalSpace::universeBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(isl_local_space_get_space( takeCopy())));
}


Aff LocalSpace::createZeroAff() const {
  return enwrap(isl_aff_zero_on_domain(takeCopy()));
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

Ctx *LocalSpace::getCtx() const {
  return Ctx::enwrap(isl_local_space_get_ctx(keep()));
}


bool LocalSpace::isSet() const {
  return isl_local_space_is_set(keep());
}
int LocalSpace::dim(isl_dim_type type) const{
  return isl_local_space_dim(keep(), type);
}
bool LocalSpace::hasDimId(isl_dim_type type, unsigned pos) const {
  return isl_local_space_has_dim_id(keep(), type, pos);
}
Id LocalSpace::getDimId(isl_dim_type type, unsigned pos) const{
  return Id::enwrap(isl_local_space_get_dim_id(keep(), type, pos));
}
bool LocalSpace::hasDimName(isl_dim_type type, unsigned pos) const{
  return isl_local_space_has_dim_name(keep(), type, pos);
}
const char *LocalSpace::getDimName(isl_dim_type type, unsigned pos) const{
  return isl_local_space_get_dim_name(keep(), type, pos);
}
void LocalSpace::setDimName(isl_dim_type type, unsigned pos, const char *s){
  give(isl_local_space_set_dim_name(take(), type, pos, s));
}
void LocalSpace::setDimId(isl_dim_type type, unsigned pos, Id &&id) {
  give(isl_local_space_set_dim_id(take(), type, pos, id.take()));
}
Space LocalSpace::getSpace() const {
  return Space::enwrap(isl_local_space_get_space(keep()));
}
Aff LocalSpace::getDiv(int pos) const {
  return Aff::enwrap(isl_local_space_get_div(keep(), pos));
}


void LocalSpace::domain(){
  give(isl_local_space_domain(take()));
}
void LocalSpace::range(){
  give(isl_local_space_range(take()));
}
void LocalSpace::fromDomain(){
  give(isl_local_space_from_domain(take()));
}
void LocalSpace::addDims(isl_dim_type type, unsigned n){
  give(isl_local_space_add_dims(take(), type, n));
}
void LocalSpace::insertDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_local_space_insert_dims(take(), type, first, n));
}
void LocalSpace::dropDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_local_space_drop_dims(take(), type, first, n));
}


BasicMap isl::lifting(LocalSpace &&ls) {
  return BasicMap::enwrap(isl_local_space_lifting(ls.take()));
}


bool isl::isEqual(const LocalSpace &ls1, const LocalSpace &ls2) {
  return isl_local_space_is_equal(ls1.keep(), ls2.keep());       
}


LocalSpace isl:: intersect( LocalSpace &&ls1,  LocalSpace &&ls2) {
  return LocalSpace::wrap(isl_local_space_intersect(ls1.take(), ls2.take()));
}
