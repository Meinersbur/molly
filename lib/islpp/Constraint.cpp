#include "islpp/Constraint.h"

#include "islpp/LocalSpace.h"
#include "islpp/Int.h"
#include "islpp/Aff.h"

#include <isl/constraint.h>

using namespace isl;
using namespace std;


isl_constraint *Constraint::takeCopy() const {
  return isl_constraint_copy(constraint);
}


void Constraint::give(isl_constraint *constraint) {
  if (this->constraint)
    isl_constraint_free(this->constraint); 
  this->constraint = constraint;
}


Constraint::~Constraint() {
  if (this->constraint) 
    isl_constraint_free(this->constraint); 
}


Constraint Constraint::createEquality(LocalSpace &&space) {
  return Constraint::wrap(isl_equality_alloc(space.take()));
}


Constraint Constraint::createInequality(LocalSpace &&space) {
  return Constraint::wrap(isl_inequality_alloc(space.take()));
}

void  Constraint::setConstant(const Int &v) {
  give(isl_constraint_set_constant(take(), v.keep() ) );
}

void Constraint::setConstant(int v) {
  give(isl_constraint_set_constant_si(take(), v));
}



void Constraint::setCoefficient(isl_dim_type type, int pos, const Int & v) {
  give(isl_constraint_set_coefficient(take(), type, pos, v.keep()));
}


void Constraint::setCoefficient( isl_dim_type type, int pos, int v) {
  give(isl_constraint_set_coefficient_si(take(), type, pos, v));
}




bool Constraint::isEquality() const{
  return isl_constraint_is_equality(keep());
}
bool Constraint::isLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_constraint_is_lower_bound(keep(), type, pos);
}
bool Constraint::isUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_constraint_is_upper_bound(keep(), type, pos);
}
Int Constraint::getConstant() const {
  Int result;
  isl_constraint_get_constant(keep(), result.change());
  return result;
}
Int Constraint::getCoefficient(isl_dim_type type, int pos) const {
  Int result;
  isl_constraint_get_coefficient(keep(), type, pos, result.change());
  return result;
}
bool Constraint::involvesDims(isl_dim_type type, unsigned first, unsigned n) const {
  return isl_constraint_involves_dims(keep(), type, first, n);
}
Aff Constraint::getDiv(int pos) const {
  return Aff::wrap(isl_constraint_get_div(keep(), pos));
}
const char *Constraint::getDimName(isl_dim_type type, unsigned pos) const {
  return isl_constraint_get_dim_name( keep(), type, pos); 
}
Aff Constraint::getBound(isl_dim_type type, int pos) const{
  return Aff::wrap(isl_constraint_get_bound(keep(), type, pos));
}
Aff Constraint::getAff() const {
  return Aff::wrap(isl_constraint_get_aff(keep()));
}
