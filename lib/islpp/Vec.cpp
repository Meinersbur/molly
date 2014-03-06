#define ISLPP_VEC_CPP
#include "islpp/Vec.h"

#include "islpp/Printer.h"
#include "../isl_sample.h"
#include "islpp/BasicSet.h"

using namespace isl;


extern inline Vec isl::enwrap(isl_vec *vec);
extern inline int isl::cmpElement(const Vec &vec1, const Vec &vec2, int pos);
extern inline Vec isl::add(const Vec &vec1, const Vec &vec2);
extern inline Vec isl::concat(const Vec &vec1, const Vec &vec2);


void Vec::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}

#if 0
std::string Vec::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}


void Vec::dump() const { 
  isl_vec_dump(keep()); 
}
#endif

void Vec::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}

isl::BasicSet isl::Vec::toBasicSet() const {
  auto nDims = getSize();
  auto result = getCtx()->createSetSpace(0, nDims).createUniverseBasicSet();  

  for (auto i = nDims-nDims;i<nDims;i+=1) {
    result.fix_inplace(isl_dim_set, i, getElement(i));
  }
  return result;
}
