#include "islpp/Point.h"

#include "islpp/Ctx.h"
#include "islpp/Space.h"
#include "islpp/Int.h"

#include <isl/point.h>

using namespace isl;
using namespace std;


isl_point *Point::takeCopy() const {
  return isl_point_copy(keep());
}


void Point::give(isl_point *point) {
  assert(point);
  if (this->point)
    isl_point_free(this->point);
  this->point = point;
}


Point::~Point(void) {
  if (this->point)
    isl_point_free(this->point);
}


Point Point::createZero(Space &&space) {
  return Point::wrap(isl_point_zero(space.take()));
}


Space Point::getSpace() const {
  return Space::wrap(isl_point_get_space(keep()));
}


Ctx *Point::getCtx() const {
  return Ctx::wrap(isl_point_get_ctx(keep()));
}


bool Point::isVoid() const{
  return isl_point_is_void(keep());
}


Int Point::getCoordinate(isl_dim_type type, int pos) const {
  Int result;
  auto retval = isl_point_get_coordinate(keep(), type, pos, result.change());
  assert(retval == 0);
  return result;
}


void Point::setCoordinate(isl_dim_type type, int pos, const Int &v) {
  give(isl_point_set_coordinate(take(), type, pos, v.keep()));
}


void Point::add(isl_dim_type type, int pos, unsigned val) {
  give(isl_point_add_ui(take(), type, pos, val));
}


void Point::sub(isl_dim_type type, int pos, unsigned val) {
  give(isl_point_sub_ui(take(), type, pos, val));
}
