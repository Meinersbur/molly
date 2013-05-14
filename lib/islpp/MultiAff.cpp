#include "islpp/MultiAff.h"

#include "islpp/Printer.h"

using namespace isl;


 isl_multi_aff *Multi<Aff>::takeCopy() const {
   assert(this->maff);
   return isl_multi_aff_copy(this->maff);
 }



void Multi<Aff>::give(isl_multi_aff *aff) {
  if (this->maff)
    isl_multi_aff_free(this->maff);
  this->maff = maff;
}


Multi<Aff>::~Multi() {
  if (this->maff)
    isl_multi_aff_free(this->maff);
#ifndef NDEBUG
  //TODO: Ifndef NVALGRIND mark as uninitialized
  this->maff = nullptr;
#endif
}


void Multi<Aff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}
std::string Multi<Aff>::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}
void Multi<Aff>::dump() const { 
  isl_multi_aff_dump(keep()); 
}
void Multi<Aff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


void Multi<Aff>::append(Aff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_aff_list_alloc(isl_multi_aff_get_ctx(keep()), n+1);
  for (auto i = n-n; i < n; i+=1) {
    list = isl_aff_list_set_aff(list, i, isl_multi_aff_get_aff(keep(), i));
  }
  list = isl_aff_list_set_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_aff_from_aff_list(space.take(), list));
}
