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


bool Map::findDim(const Dim &dim, isl_dim_type &type, unsigned &pos) {
  type = dim.getType();

  switch (type) {
  case isl_dim_param: {
    // Param dims are identified by id
    if (!dim.hasId())
      return false;
    auto retval = isl_map_find_dim_by_id(keep(), type, dim.getId().keep());
    if (retval<0) 
      return false; // Not found
    pos = retval;
    return true;
                      } break;
  case isl_dim_in:
  case isl_dim_out: {
    // Are identified by position
    pos = dim.getPos();

    // Consistency check
    if (this->dim(type) != dim.getTypeDims())
      return false; // These are different spaces

#ifndef NDEBUG
    auto thatName = dim.getName();
    auto thisName = isl_map_get_dim_name(keep(), type, pos);
    assert(strcmp(thatName, thisName) == 0 && "Give same dimensions the same id/name");

    auto thatId = dim.getId();
    auto thisId = Id::wrap(isl_map_get_dim_id(keep(), type, pos));
    assert(thatId == thisId && "Give same dimensions the same id");
#endif

    return true;
                    } break;
  default:
    return false;
  }
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
