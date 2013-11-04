#include "islpp/Map.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/UnionMap.h"
#include "islpp/MultiPwAff.h"
#include "islpp/DimRange.h"

#include <isl/map.h>
#include <llvm/Support/raw_ostream.h>
#include "islpp/Point.h"

using namespace isl;
using namespace llvm;
using namespace std;




#if 0
MultiPwAff Map::toMultiPwAff() const {
  return toPwMultiAff().toMultiPwAff();
}
#endif

UnionMap Map::toUnionMap() const {
  return UnionMap::enwrap(isl_union_map_from_map(takeCopy()));
}
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
UnionMap Map::toUnionMap() && {
  return UnionMap::enwrap(isl_union_map_from_map(take()));
}
#endif




Map Map::readFrom(Ctx *ctx, const char *str) {
  return Map::enwrap(isl_map_read_from_str(ctx->keep(), str));
}

#if 0
Map Map::fromMultiPwAff(MultiPwAff &&mpaff) {
  return Map::enwrap(isl_map_from_pw_multi_aff(mpaff.toPwMultiAff().take()));
}
#endif

void Map::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void Map::dump() const { 
  isl_map_dump(keep()); 
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

#if 0
Map Map::chainNested(const Map &map) const {
  return this->wrap().chainNested(map);
}
#endif

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
  assert(dims.isValid());

  auto tupleSpace = getTupleSpace(type);
  auto expandDomainSpace = Space::createMapFromDomainAndRange(map.getDomainSpace(), tupleSpace); // { B -> (A, B', C) }
  auto expandDomain = expandDomainSpace.equalBasicMap(isl_dim_in, 0, dims.getCount(), isl_dim_out, dims.getBeginPos()); // { B -> (A, B', C) | B=B' }

  auto resultSpace = Space::createMapFromDomainAndRange(tupleSpace, map.getRangeSpace()).wrap(); // { ((A, B, C) -> E) }
  auto expandRangeSpace = Space::createMapFromDomainAndRange(map.getRangeSpace(), resultSpace); // { E -> ((A, B, C) -> E') }
  auto expandRange = expandRangeSpace.equalBasicMap(isl_dim_in, 0, map.getOutDimCount(), isl_dim_out, tupleSpace.getSetDimCount()); // { E -> ((A, B, C) -> E') | E=E' }

  auto expandedMap = map.applyDomain(expandDomain).applyRange(expandRange); // { (A, B, C) -> ((A, B, C) -> E)  }
  expandedMap.intersect_inplace(expandedMap.getSpace().equalBasicMap(tupleSpace.getSetDimCount()));
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


void Map::projectOutSubspace_inplace(isl_dim_type type, const Space &subspace) ISLPP_INPLACE_FUNCTION {
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


    void Map::moveSubspaceAppendToRange_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION {
      auto myspace = getSpace();
      auto dimrange = myspace.findSubspace(isl_dim_in, subspace);
      assert(dimrange.isValid());

        auto newDomainSpace = getDomainSpace().removeSubspace(subspace);
        auto newRangeSpace = Space::createMapFromDomainAndRange(getRangeSpace(), subspace).wrap();
      auto newSpace = Space::createMapFromDomainAndRange(newDomainSpace, newRangeSpace);

      moveDims_inplace(isl_dim_out, getOutDimCount(), isl_dim_in, dimrange.getBeginPos(), dimrange.getCount());
      cast_inplace(newSpace);
    }


   void Map::moveSubspacePrependToRange_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION {
       auto myspace = getSpace();
      auto dimrange = myspace.findSubspace(isl_dim_in, subspace);
      assert(dimrange.isValid());

        auto newDomainSpace = getDomainSpace().removeSubspace(subspace);
        auto newRangeSpace = Space::createMapFromDomainAndRange(subspace, getRangeSpace()).wrap();
      auto newSpace = Space::createMapFromDomainAndRange(newDomainSpace, newRangeSpace);

      moveDims_inplace(isl_dim_out, 0, isl_dim_in, dimrange.getBeginPos(), dimrange.getCount());
      cast_inplace(newSpace);
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


void isl::Map::dumpExplicit( int maxElts /*= 8*/ )const{
  printExplicit(llvm::dbgs(), maxElts);
  llvm::dbgs() << "\n";
}


void isl::Map::dumpExplicit()const{
  dumpExplicit(8);
}


void isl::Map::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/) const{
  auto wrapped = this->wrap();
  auto nInDims = getInDimCount();
   auto nOutDims = getOutDimCount();

  os << "{ ";
  int count=0;
  auto omittedsome = wrapped.foreachPoint([&count,maxElts,&os,nInDims,nOutDims](Point p) -> bool {
    if (count >= maxElts)
      return true;

    if (count>0) {
      os << ", ";
    }
    if (nInDims != 1) 
      os << "[";
    for (auto i = nInDims-nInDims;i<nInDims;i+=1) {
      if (i>0)
        os << ",";
      os << p.getCoordinate(isl_dim_set, i);
    }
    if (nInDims != 1) 
      os << "]";

    os << " -> ";

    if (nOutDims != 1) 
      os << "[";
    for (auto i = nOutDims-nOutDims;i<nOutDims;i+=1) {
      if (i>0)
        os << ",";
      os << p.getCoordinate(isl_dim_set, nInDims+i);
    }
    if (nOutDims != 1) 
      os << "]";

    count+=1;
    return false;
  });

  if (omittedsome) {
    os << ", ...";
  }
  os << " }";
}


std::string isl::Map::toStringExplicit( int maxElts /*= 8*/ )
{
  std::string str;
  llvm::raw_string_ostream os(str);
  printExplicit(os, maxElts);
  os.flush();
  return str; // NRVO
}


#ifndef NDEBUG
std::string isl::Map::toString() const {
  return ObjBaseTy::toString();
}
#endif


ISLPP_EXSITU_ATTRS Map isl::Map::intersectDomain( UnionSet uset ) ISLPP_EXSITU_FUNCTION {
  return intersectDomain(uset.extractSet(getDomainSpace()));
}


ISLPP_EXSITU_ATTRS Map isl::Map::intersectRange( UnionSet uset ) ISLPP_EXSITU_FUNCTION {
  auto set = uset.extractSet(getRangeSpace());
  return Map::enwrap(isl_map_intersect_range(takeCopy(), set.take()));
}
