#include "islpp/Map.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/UnionMap.h"
#include "islpp/MultiPwAff.h"
#include "islpp/DimRange.h"

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


Map Map::chainNested(isl_dim_type type, const Map &map) const {
  // assume type==isl_dim_in
  // this = { (A, B, C) -> D }
  // map = { B -> E }
  // return { ((A, B, C) -> E) -> D }

  auto space = getSpace(); // { (A, B, C) -> D }
  auto seekNested = map.getDomainSpace(); // { B }

  auto dims = space.findSubspace(type, seekNested);
  if (dims.isNull()) {
    assert(!"Nested space not found");
    return Map();
  }

  auto expandDomainSpace = Space::createMapFromDomainAndRange(map.getDomainSpace(), getTupleSpace(type)); // { B -> (A, B', C) }
  auto expandDomain = expandDomainSpace.equalBasicMap(isl_dim_in, 0, dims.getCount(), isl_dim_out, dims.getBeginPos()); // { B -> (A, B', C) | B=B' }

  auto resultSpace = Space::createMapFromDomainAndRange(getTupleSpace(type), map.getRangeSpace()); // { (A, B, C) -> E }
  auto expandRangeSpace = Space::createMapFromDomainAndRange(map.getRangeSpace(), resultSpace); // { E -> ((A, B, C) -> E') }
  auto expandRange = expandRangeSpace.equalBasicMap(isl_dim_in, 0, map.getOutDimCount(), isl_dim_out, resultSpace.getInDimCount()); // { E -> ((A, B, C) -> E') | E=E' }

  auto expandedMap = map.applyDomain(expandDomain).applyRange(expandRange); // { (A, B, C) -> ((A, B, C) -> E)  }
  if (type==isl_dim_in)
    return applyDomain(expandedMap);
  else if (type==isl_dim_out)
    return applyRange(expandedMap);
  else
    llvm_unreachable("invalid dim type");
}


Map Map::applyNested(isl_dim_type type, const Map &map) const {
  auto space = getSpace();
  auto typeSpace = space.extractTuple(type);
  auto seekNested = map.getDomainSpace();

  auto dims = space.findSubspace(type, seekNested);
  assert(dims.isValid());

  auto nPrefixDims = dims.getBeginPos();
  auto nDomainDims = map.getInDimCount();
  auto nRangeDims = map.getOutDimCount();
  auto nPosfixDims = typeSpace.getSetDimCount() - dims.getEndPos();
  assert(nPrefixDims + nDomainDims + nPosfixDims == typeSpace.getSetDimCount());

  //auto tuple = space.extractTuple(type);
  auto applySpace = Space::createMapFromDomainAndRange(typeSpace, typeSpace.replaceSubspace(seekNested, map.getRangeSpace()));
  assert(applySpace.getInDimCount() == nPrefixDims + nDomainDims + nPosfixDims);
  assert(applySpace.getOutDimCount() == nPrefixDims + nRangeDims + nPosfixDims);

  auto apply = map.insertDims(isl_dim_in, 0, nPrefixDims);
  apply.insertDims_inplace(isl_dim_out, 0, nPrefixDims);
  apply.addDims_inplace(isl_dim_in, nPosfixDims);
  apply.addDims_inplace(isl_dim_out, nPosfixDims);

  apply.intersect_inplace(apply.getSpace().equalBasicMap(isl_dim_in, 0, nPrefixDims, isl_dim_out, 0 ));
  apply.intersect_inplace(apply.getSpace().equalBasicMap(isl_dim_in, nPrefixDims + nDomainDims, nPosfixDims, isl_dim_out,nPrefixDims + nRangeDims));
  apply.cast_inplace(applySpace);

  if (type==isl_dim_in)
    return applyDomain(apply);
  else
    return applyRange(apply);
}


void Map::projectOutSubspace_inplace(isl_dim_type type, const Space &subspace) ISLPP_INPLACE_QUALIFIER {
  auto myspace = getTupleSpace(type);
  auto dims = getSpace().findSubspace(type, subspace);
  assert(dims.isValid());
  assert(dims.getType() == type);

  auto shrinkSpace = myspace.removeSubspace(subspace); 
  this->projectOut_inplace(type, dims.getBeginPos(), dims.getCount());

  if (isl_dim_in == type)
    this->cast_inplace(Space::createMapFromDomainAndRange(shrinkSpace, getRangeSpace()));
  else
    this->cast_inplace(Space::createMapFromDomainAndRange(getDomainSpace(), shrinkSpace));
}


Map Map::projectOutSubspace(isl_dim_type type, const Space &subspace) const {
  auto result = copy();
  result.projectOutSubspace_inplace(type, subspace);
  return result;
}


Map isl::join(const Set &domain, const Set &range, unsigned firstDomainDim, unsigned firstRangeDim, unsigned countEquate) {
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
