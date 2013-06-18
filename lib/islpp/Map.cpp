#include "islpp/Map.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/UnionMap.h"
#include "islpp/MultiPwAff.h"

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
  if (this->map && this->map != map)
    isl_map_free(this->map);
  this->map = map;
#ifndef NDEBUG
  this->_printed = toString();
#endif
}


Map Map::readFrom(Ctx *ctx, const char *str) {
  return Map::wrap(isl_map_read_from_str(ctx->keep() , str));
}


// Missing in isl
__isl_give isl_map* isl_map_from_multi_pw_aff(__isl_take isl_multi_pw_aff *mpwaff) {
  if (!mpwaff)
    return NULL;

  isl_space *space = isl_space_domain(isl_multi_pw_aff_get_space(mpwaff));
  isl_map *map = isl_map_universe(isl_space_from_domain(space));

  unsigned n = isl_multi_pw_aff_dim(mpwaff, isl_dim_out);
  for (int i = 0; i < n; ++i) {
    isl_pw_aff *pwaff = isl_multi_pw_aff_get_pw_aff(mpwaff, i); 
    isl_map *map_i = isl_map_from_pw_aff(pwaff);
    map = isl_map_flat_range_product(map, map_i);
  }

  isl_multi_pw_aff_free(mpwaff);
  return map;
}


Map Map::fromMultiPwAff(MultiPwAff &&mpaff) {
  return wrap(isl_map_from_multi_pw_aff(mpaff.take()));
}


void Map::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}
std::string Map::toString() const {
  if (!keep())
    return string();
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return stream.str();
}
void Map::dump() const {
  print(llvm::errs());
}


Map Map::createFromUnionMap(UnionMap &&umap) {
  return Map::wrap(isl_map_from_union_map(umap.take()));
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


Map BasicMap::toMap() const {
  return enwrap(isl_map_from_basic_map(takeCopy()));
}
