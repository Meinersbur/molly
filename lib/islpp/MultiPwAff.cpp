#include "islpp/MultiPwAff.h"

#include "islpp/Printer.h"
#include <llvm/Support/raw_ostream.h>

using namespace isl;


void Multi<PwAff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}
std::string Multi<PwAff>::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}
void Multi<PwAff>::dump() const { 
  isl_multi_pw_aff_dump(keep()); 
}
void Multi<PwAff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
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
