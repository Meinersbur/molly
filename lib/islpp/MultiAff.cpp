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

}
