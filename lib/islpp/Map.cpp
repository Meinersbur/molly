#include "islpp/Map.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"

#include <isl/map.h>
#include <llvm/Support/raw_ostream.h>


using namespace isl;
using namespace llvm;
using namespace std;

isl_map *Map::takeCopy() const {
  assert(map);
  return isl_map_copy(map);
}


void Map::give(isl_map *map) {
  if (this->map)
    isl_map_free(this->map);
  this->map = map;
}


Map Map::readFrom(Ctx *ctx, const char *str) {
  return Map::wrap(isl_map_read_from_str(ctx->keep() , str));
}


void Map::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}
std::string Map::toString() const {
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return buf;
}
void Map::dump() const {
  print(llvm::errs());
}


Ctx *Map::getCtx() const {
  return Ctx::wrap(isl_map_get_ctx(map));
}


bool Map::isEmpty() const {
  return isl_map_is_empty(keep());
}

static int foearchBasicMapCallback(__isl_take isl_basic_map *bmap, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(BasicMap&&)>*>(user);
  auto retval = func( BasicMap::wrap(bmap) );
  return retval ? -1 : 0;
}
bool Map::foreachBasicMap(std::function<bool(BasicMap&&)> func) const {
  auto retval = isl_map_foreach_basic_map(keep(), foearchBasicMapCallback, &func);
  return retval!=0;
}
