#define ISLPP_VEC_CPP
#include "islpp/Vec.h"

#include "islpp/Printer.h"

using namespace isl;


extern inline Vec isl::enwrap(isl_vec *vec);
extern inline int cmpElement(const Vec &vec1, const Vec &vec2, int pos);
extern inline Vec add(const Vec &vec1, const Vec &vec2);
extern inline Vec concat(const Vec &vec1, const Vec &vec2);


void Vec::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}
std::string Vec::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}
void Vec::dump() const { 
  isl_vec_dump(keep()); 
}
void Vec::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}
