#include "islpp/MultiAff.h"

#include "islpp/Printer.h"
#include <isl/map.h>
#include "islpp/BasicMap.h"
#include "islpp/Map.h"
#include "islpp/DimRange.h"

using namespace isl;


PwMultiAff Multi<Aff>::toPwMultiAff() const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_from_multi_aff(takeCopy()));
}


MultiPwAff Multi<Aff>::toMultiPwAff() const {
  auto result = getSpace().createZeroMultiPwAff();
  auto nDims = getOutDimCount();
  for (auto i = nDims-nDims; i < nDims; i+=1) {
    result.setPwAff_inplace(i, getAff(i));
  }
  return result;
}


void Multi<Aff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


void Multi<Aff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


void Multi<Aff>::push_back(Aff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_aff_list_alloc(isl_multi_aff_get_ctx(keep()), n+1);
  for (auto i = n-n; i < n; i+=1) {
    list = isl_aff_list_set_aff(list, i, isl_multi_aff_get_aff(keep(), i));
  }
  list = isl_aff_list_set_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_aff_from_aff_list(space.take(), list));
}

#if 0
// Missing in isl
static __isl_give isl_map* isl_map_from_multi_pw_aff(__isl_take isl_multi_pw_aff *mpwaff) {
  if (!mpwaff)
    return NULL;

  isl_space *space = isl_space_domain(isl_multi_pw_aff_get_space(mpwaff));
  isl_map *map = isl_map_universe(isl_space_from_domain(space));

  unsigned n = isl_multi_pw_aff_dim(mpwaff, isl_dim_out);
  for (unsigned i = 0; i < n; ++i) {
    isl_pw_aff *pwaff = isl_multi_pw_aff_get_pw_aff(mpwaff, i); 
    isl_map *map_i = isl_map_from_pw_aff(pwaff);
    map = isl_map_flat_range_product(map, map_i);
  }

  isl_multi_pw_aff_free(mpwaff);
  return map;
}
#endif

BasicMap Multi<Aff>::toBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_from_multi_aff(takeCopy()));
}


Map Multi<Aff>::toMap() const {
  return Map::enwrap(isl_map_from_multi_aff(takeCopy()));
}


PwMultiAff Multi<Aff>::restrictDomain(Set &&set) const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_alloc(set.take(), takeCopy()));
}


PwMultiAff Multi<Aff>::restrictDomain(const Set &set) const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_alloc(set.takeCopy(), takeCopy()));
}


void Multi<Aff>::neg_inplace() ISLPP_INPLACE_FUNCTION {
  auto size = getOutDimCount();
  for (auto i = size-size; i < size; i+=1) {
    setAff_inplace(i, getAff(i).neg());
  }
}


void Multi<Aff>::subMultiAff_inplace(unsigned first, unsigned count) ISLPP_INPLACE_FUNCTION {
  auto nOutDims = getOutDimCount();
  removeDims_inplace(isl_dim_out, first+count, nOutDims-first-count);
  removeDims_inplace(isl_dim_out, 0, first);
}


void isl::Multi<Aff>::dump() const {
  isl_multi_aff_dump(keep());
}


ISLPP_CONSUME_ATTRS BasicMap isl::Multi<Aff>::reverse_consume() ISLPP_CONSUME_FUNCTION {
  return BasicMap::enwrap(isl_basic_map_reverse(isl_basic_map_from_multi_aff(take())));
}


ISLPP_EXSITU_ATTRS BasicMap isl::Multi<Aff>::reverse() ISLPP_EXSITU_FUNCTION {
  return copy().reverse_consume();
}


ISLPP_EXSITU_ATTRS MultiAff isl::Multi<Aff>::embedIntoDomain(Space framedomainspace) ISLPP_EXSITU_FUNCTION {
  auto subspace = getDomainSpace();
  auto myspace = getSpace();
  auto range = myspace.findSubspace(isl_dim_in, subspace);
  auto islctx = getCtx();

  auto before = islctx->createSetSpace(range.getCountBefore()).createIdentityMultiAff();
  auto after = islctx->createSetSpace(range.getCountAfter()).createIdentityMultiAff();
  auto prod = isl::product(before, *this, after);
  auto targetSpace = framedomainspace.replaceSubspace(getDomainSpace(), getRangeSpace());
  return prod.cast(framedomainspace, targetSpace);
}


void MultiAff::sublist_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION {
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());
  subMultiAff_inplace(range.getFirst(), range.getCount());

  // FIXME: There is no isl_multi_aff_cast(take(), Space::createMapFromDomainAndRange(getDomainSpace(), subspace))
  setOutTupleId_inplace(subspace.getSetTupleId());
}


MultiAff MultiAff::embedAsSubspace(const Space &framespace) const {
  auto subspace = getDomainSpace();
  auto myspace = getSpace();
  auto range = myspace.findSubspace(isl_dim_in, subspace);

  auto result = framespace.createIdentityMultiAff();
  auto count = range.getCount();
  for (auto i = count-count; i < count; i+=1) {
    result.setAff_inplace(range.relativePos(i), getAff(i));
  }
  return result;
}


PwMultiAff MultiAff::pullback(PwMultiAff mpa) ISLPP_EXSITU_FUNCTION {
  return PwMultiAff::enwrap(isl_pw_multi_aff_pullback_pw_multi_aff( isl_pw_multi_aff_from_multi_aff(takeCopy()), mpa.take()  ));
}


PwMultiAff MultiAff::applyRange(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION {
  return pma.pullback(*this);
}


MultiAff MultiAff::cast(Space space) ISLPP_EXSITU_FUNCTION {
  assert(space.getInDimCount() == this->getInDimCount());
  assert(space.getOutDimCount() == this->getOutDimCount());

  auto domainSpace = space.getDomainSpace();
  auto rangeSpace = space.getRangeSpace();
  auto resultSpace = domainSpace.mapsTo(rangeSpace);
  auto translateSpace = domainSpace.mapsTo(getDomainSpace());
  auto translate = translateSpace.createIdentityMultiAff();
  auto result = resultSpace.createZeroMultiAff();

  auto nOutDims = getOutDimCount();
  for (auto i = nOutDims-nOutDims; i<nOutDims;i+=1) {
    auto aff = getAff(i);
    auto backpulled = aff.pullback(translate);
    result.setAff_inplace(i, backpulled);
  }

  return result;
}


ISLPP_INPLACE_ATTRS void MultiAff::castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION {
  assert(domainSpace.getSetDimCount() == this->getInDimCount());

  auto translateSpace = Space::createMapFromDomainAndRange(domainSpace, getDomainSpace());
  auto translate = translateSpace.move().createIdentityMultiAff();
  pullback_inplace(translate.move());
}


ISLPP_EXSITU_ATTRS MultiAff MultiAff::castRange(Space rangeSpace) ISLPP_EXSITU_FUNCTION {
  assert(rangeSpace.getSetDimCount() == this->getOutDimCount());

  auto resultSpace = Space::createMapFromDomainAndRange(getDomainSpace(), rangeSpace);
  auto result = resultSpace.createZeroMultiAff();

  auto nOutDims = getOutDimCount();
  for (auto i = nOutDims-nOutDims; i<nOutDims;i+=1) {
    result.setAff_inplace(i, getAff(i));
  }
  return result;
}


ISLPP_EXSITU_ATTRS BasicSet MultiAff::getDomain() ISLPP_EXSITU_FUNCTION {
  return getDomainSpace().createUniverseBasicSet();
}


ISLPP_EXSITU_ATTRS BasicSet isl::MultiAff::getRange() ISLPP_EXSITU_FUNCTION {
  return toBasicMap().getRange();
}
