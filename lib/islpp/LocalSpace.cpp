#include "islpp/LocalSpace.h"

#include "islpp/Space.h"

#include <isl/local_space.h>


using namespace isl;

LocalSpace::LocalSpace(isl_local_space *space) {
  this->space = space;
}

LocalSpace::~LocalSpace() {
  isl_local_space_free(space);
}


LocalSpace::LocalSpace(const LocalSpace &that){
  this->space = isl_local_space_copy(that.space);
}
const LocalSpace &LocalSpace::operator=(const LocalSpace &that){
  if (this != &that) {
      isl_local_space_free(this->space);
      this->space = isl_local_space_copy(that.space);
  }
  return *this;
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


LocalSpace LocalSpace::clone() {
  return LocalSpace(*this);
}

