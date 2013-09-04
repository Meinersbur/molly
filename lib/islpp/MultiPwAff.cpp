#include "islpp/MultiPwAff.h"

#include "islpp/Printer.h"
#include "islpp/Map.h"
#include <llvm/Support/raw_ostream.h>

using namespace isl;


Map Multi<PwAff>::toMap() const {
  return Map::fromMultiPwAff(this->copy());
}


PwMultiAff Multi<PwAff>::toPwMultiAff() const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_from_map( toMap().take() ));
}


void Multi<PwAff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}

void Multi<PwAff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


    /* implicit */  Multi<PwAff>:: Multi(const MultiAff &madd) 
      : Obj(madd.isValid() ? madd.toMultiPwAff().take() : nullptr) { 
    }


    MultiPwAff &Multi<PwAff>::operator=(const MultiAff &madd) { 
      give(madd.isValid() ? madd.toMultiPwAff().take() : nullptr); 
return *this;
    }


void Multi<PwAff>::push_back(PwAff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_pw_aff_list_alloc(isl_multi_pw_aff_get_ctx(keep()), n+1);
  for (auto i = n-n; i < n; i+=1) {
    list = isl_pw_aff_list_set_pw_aff(list, i, isl_multi_pw_aff_get_pw_aff(keep(), i));
  }
  list = isl_pw_aff_list_set_pw_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_pw_aff_from_pw_aff_list(space.take(), list));
}


