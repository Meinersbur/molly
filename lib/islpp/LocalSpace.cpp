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


LocalSpaceDimtypeIter::iterator &LocalSpaceDimtypeIter::operator++/*preincrement*/() {
  while (true) {
    current = (isl_dim_type)((unsigned)current+1);
    if (current >= isl_dim_all) 
      break;
    auto len = owner->dim(current);
    if (len > 0)
      break;
  }
  return *this;
}


LocalSpaceDimIter::value_type LocalSpaceDimIter::operator*() const {
  return Dim::wrap(owner->keep(), currentType, currentPos);
}


LocalSpaceDimIter &LocalSpaceDimIter::operator++/*preincrement*/() {
  currentPos += 1;
  if (currentPos < currentTypeSize) {
    return *this;
  } else {
    currentPos = 0;
    while (true) {
      while (true) {
        currentType = (isl_dim_type)((int)currentType+1);
        if (currentType == isl_dim_all)
          return *this;
        if ((1 << currentType) & typeFilter) 
          break;
      }
      currentTypeSize = owner->dim(currentType);
      if (currentTypeSize > 0)
        return *this;
    }
  }
}


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




Aff LocalSpace::createZeroAff() const {
  return enwrap(isl_aff_zero_on_domain(takeCopy()));
}

Ctx *LocalSpace::getCtx() const {
  return Ctx::wrap(isl_local_space_get_ctx(keep()));
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
  return Id::wrap(isl_local_space_get_dim_id(keep(), type, pos));
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
  return Space::wrap(isl_local_space_get_space(keep()));
}
Aff LocalSpace::getDiv(int pos) const {
  return Aff::wrap(isl_local_space_get_div(keep(), pos));
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
  return BasicMap::wrap(isl_local_space_lifting(ls.take()));
}

bool isl::isEqual(const LocalSpace &ls1, const LocalSpace &ls2) {
  return isl_local_space_is_equal(ls1.keep(), ls2.keep());       
}

LocalSpace isl:: intersect( LocalSpace &&ls1,  LocalSpace &&ls2) {
  return LocalSpace::wrap(isl_local_space_intersect(ls1.take(), ls2.take()));
}
