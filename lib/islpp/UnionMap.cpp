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


static int foreachMapCallback(__isl_take isl_map *map, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Map)>*>(user);
  auto retval = func(Map::wrap(map));
  return retval ? -1 : 0;
}
bool UnionMap::foreachMap(const std::function<bool(isl::Map)> &func) const {
  auto retval = isl_union_map_foreach_map(keep(), foreachMapCallback, (void*)&func);
  return retval!=0;
}
