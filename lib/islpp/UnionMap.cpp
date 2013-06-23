#include "islpp/UnionMap.h"

#include "islpp/Printer.h"
#include <isl/union_map.h>

using namespace llvm;
using namespace isl;



void UnionMap::print(llvm::raw_ostream &out) const {
  if (isNull())
    return;

  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
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
