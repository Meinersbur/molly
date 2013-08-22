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


MultiPwAff Map::toMultiPwAff() const {
  return toPwMultiAff().toMultiPwAff();
}


UnionMap Map::toUnionMap() const {
  return UnionMap::enwrap(isl_union_map_from_map(takeCopy()));
}
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
UnionMap Map::toUnionMap() && {
  return UnionMap::enwrap(isl_union_map_from_map(take()));
}
#endif




Map Map::readFrom(Ctx *ctx, const char *str) {
  return Map::enwrap(isl_map_read_from_str(ctx->keep() , str));
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
  return Map::enwrap(isl_map_from_multi_pw_aff(mpaff.take()));
}


void Map::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


Map Map::createFromUnionMap(UnionMap &&umap) {
  return Map::enwrap(isl_map_from_union_map(umap.take()));
}


static int foreachBasicMapCallback(__isl_take isl_basic_map *bmap, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(BasicMap&&)>*>(user);
  auto retval = func( BasicMap::enwrap(bmap) );
  return retval ? -1 : 0;
}
bool Map::foreachBasicMap(std::function<bool(BasicMap&&)> func) const {
  auto retval = isl_map_foreach_basic_map(keep(), foreachBasicMapCallback, &func);
  return retval!=0;
}


static int enumBasicMapCallback(__isl_take isl_basic_map *map, void *user) {
  auto list = static_cast< std::vector<BasicMap> *>(user);
  list->push_back(BasicMap::enwrap(map));
  return 0;
}
std::vector<BasicMap> Map::getBasicMaps() const {
  std::vector<BasicMap> result;
  //result.reserve(isl_map_n_basic_maps(keep()));
  auto retval = isl_map_foreach_basic_map(keep(), enumBasicMapCallback, &result);
  assert(retval==0);
  return result;
}


Map BasicMap::toMap() const {
  return Map::enwrap(isl_map_from_basic_map(takeCopy()));
}


    Map Map:: chainNested(const Map &map) const {
      return this->wrap().chainNested(map);
    }


    Map Map::chainNested(const Map &map, unsigned tuplePos) const {
      return this->wrap().chainNested(map, tuplePos);
    }


Map isl:: join(const Set &domain, const Set &range, unsigned firstDomainDim, unsigned firstRangeDim, unsigned countEquate) {
  auto cartesian = product(domain, range).unwrap();
  cartesian.intersect(cartesian.getSpace().equalBasicMap(isl_dim_in, firstDomainDim, countEquate, isl_dim_out, firstRangeDim));
  return cartesian;
}


Map isl::naturalJoin(const Set &domain, const Set &range) {
  auto cartesian = product(domain, range).unwrap();

  auto domainSpace =  domain.getSpace();
  auto rangeSpace = range.getSpace();
  auto productSpace = cartesian.getSpace();

  auto domainSpaces = domainSpace.flattenNestedSpaces();
  unsigned domainPos = 0;
  for (auto &space : domainSpaces) {
    unsigned first,count;
    if (! rangeSpace.findTuple(isl_dim_set, space.getTupleId(isl_dim_set), first, count))
      continue;
    assert(count = space.getSetDimCount());
    cartesian.intersect(productSpace.equalBasicMap(isl_dim_in, domainPos, count, isl_dim_out, first));
    domainPos += space.getSetDimCount();
  }

  auto nDims = domainSpace.getSetDimCount();
  for (auto i = nDims-nDims; i < nDims; i+=1) {
    if (!domainSpace.hasDimId(isl_dim_set, i))
      continue;

    auto id = domain.getSetTupleId();
    auto pos = rangeSpace.findDimById(isl_dim_set, id); // This assumes that thare is at most one match of this id in rangeSpace
    if (pos < 0)
      continue;

    cartesian.equate_inplace(isl_dim_in, i, isl_dim_out, pos);
  }

  return cartesian;
}
