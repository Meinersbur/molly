#include "islpp/UnionMap.h"

#include "islpp/Printer.h"
#include <isl/union_map.h>


using namespace isl;


isl_union_map *UnionMap::takeCopy() const {
  assert(map);
  return isl_union_map_copy(map);
}


void UnionMap:: give(isl_union_map *map) {
  assert(map);
  if (this->map)
    isl_union_map_free(this->map);
}


UnionMap::~UnionMap() {
  if (this->map)
    isl_union_map_free(this->map);
#ifndef NDEBUG
  this->map = NULL;
#endif
}


void UnionMap::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


std::string UnionMap::toString() const {
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return buf;
}


void UnionMap::dump() const {
  isl_union_map_dump(keep());
}
