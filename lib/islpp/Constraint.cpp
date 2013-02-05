#include "islpp/Constraint.h"

#include "islpp/LocalSpace.h"

#include <isl/constraint.h>

using namespace isl;
using namespace std;

Constraint::Constraint(isl_constraint *constraint) {
  this->constraint = constraint;
}


Constraint Constraint::wrap(isl_constraint *constraint) {
  return Constraint(constraint);
}


 Constraint::~Constraint() {
  if (this->constraint) isl_constraint_free(this->constraint); 
 }


Constraint Constraint::createEquality(LocalSpace &&space) {
  return Constraint(isl_equality_alloc(space.take()));
}


 Constraint Constraint::createInequality(LocalSpace &&space) {
   return wrap(isl_inequality_alloc(space.take()));
 }


void Constraint::setCoefficient(enum isl_dim_type type, int pos, int v) {
  give(isl_constraint_set_coefficient_si(take(), type, pos, v));
}


void Constraint::setConstant(int v) {
  give(isl_constraint_set_constant_si(take(), v));
}
