#include "islpp/BasicSet.h"

#include "islpp/Constraint.h"
#include "islpp/Space.h"
#include "CstdioFile.h"

#include <llvm/Support/raw_ostream.h>
#include <isl/set.h>
//#include <stdio.h>

using namespace isl;
using namespace std;

isl_basic_set *BasicSet::takeCopy() const { assert(set); return isl_basic_set_copy(this->set); }

void BasicSet::give(isl_basic_set *set) { 
  if (set)
    isl_basic_set_free(set);
  this->set = set; 
}

BasicSet::~BasicSet() { 
  if (set) 
    isl_basic_set_free(set);
}

BasicSet BasicSet::create(const Space &space) {
  return BasicSet::wrap(isl_basic_set_universe(space.copy().take()));
}

BasicSet BasicSet::create(Space &&space) {
  return BasicSet::wrap(isl_basic_set_universe(space.take()));
}

BasicSet BasicSet::createEmpty(const Space &space){
  return  BasicSet::wrap(isl_basic_set_empty(space.copy().take()));
}
BasicSet BasicSet::createEmpty(Space &&space){
  return BasicSet::wrap(isl_basic_set_empty(space.take()));
}


BasicSet BasicSet::createUniverse(const Space &space){
  return  BasicSet::wrap(isl_basic_set_universe(space.copy().take()));
}
BasicSet BasicSet::createUniverse(Space &&space){
  return BasicSet::wrap(isl_basic_set_universe(space.take()));
}

void BasicSet::print(llvm::raw_ostream &out) const { 
  molly::CstdioFile tmp;
  isl_basic_set_print(this->keep(), tmp.getFileDescriptor(), 0, "prefix", "suffic", ISL_FORMAT_ISL);
  out << tmp.readAsStringAndClose();
}

std::string BasicSet::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}

void BasicSet::dump() const { 
  print(llvm::errs());
}


void BasicSet::addConstraint(Constraint &&constraint) {
  set = isl_basic_set_add_constraint(this->take(), constraint.take());
}


void BasicSet::projectOut(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_basic_set_project_out(take(), type, first, n));
}


BasicSet BasicSet::params() {
  return BasicSet::wrap(isl_basic_set_params(takeCopy()));
}


BasicSet isl::params(BasicSet &&params) {
  return BasicSet::wrap(isl_basic_set_params(params.take()));
}
