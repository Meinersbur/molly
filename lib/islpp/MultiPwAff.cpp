#include "islpp/MultiPwAff.h"

#include "islpp/Printer.h"
#include "islpp/Map.h"
#include <llvm/Support/raw_ostream.h>

using namespace isl;

#if 0
// Missing in isl
static __isl_give isl_map* isl_map_from_multi_pw_aff(__isl_take isl_multi_pw_aff *mpwaff) {
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
#endif

Map MultiPwAff::toMap() const {
 auto space = getDomainSpace().fromDomain();
 auto result = space.universeMap();

 auto nDims = getOutDimCount();
   for (auto i = nDims-nDims; i < nDims; i+=1) {
     auto elt = getPwAff(i);
     result = flatRangeProduct(result.move(), elt.toMap());
   }

   result.cast_inplace(getSpace());
   return result;
}


PwMultiAff MultiPwAff::toPwMultiAff() const {
  auto space = getDomainSpace().fromDomain();
  auto result = PwMultiAff::createFromDomain(getDomainSpace().universeSet());
  //space.createEmptyPwMultiAff();

  auto nDims = getOutDimCount();
  for (auto i = nDims-nDims; i < nDims; i+=1) {
    auto elt = getPwAff(i);
    result = flatRangeProduct(result.move(), elt.toPwMultiAff());
  }

  result.cast_inplace(getSpace());
  return result;
}


void Multi<PwAff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}

void Multi<PwAff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


/* implicit */  Multi<PwAff>:: Multi(const MultiAff &madd) 
  : Obj(madd.isValid() ? madd.toMultiPwAff().take() : nullptr) { 
}


const MultiPwAff &Multi<PwAff>::operator=(const MultiAff &madd) ISLPP_INPLACE_QUALIFIER { 
  give(madd.isValid() ? madd.toMultiPwAff().take() : nullptr); 
  return *this;
}


/* implicit */ Multi<PwAff>::Multi(const PwMultiAff &pma) 
  : Obj(pma.isValid() ? pma.toMultiPwAff().take() : nullptr) {}


const MultiPwAff &Multi<PwAff>::operator=(const PwMultiAff &pma) ISLPP_INPLACE_QUALIFIER {
  give(pma.isValid() ? pma.toMultiPwAff().take() : nullptr);
  return *this;
}


void Multi<PwAff>::push_back(PwAff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_pw_aff_list_alloc(isl_multi_pw_aff_get_ctx(keep()), n+1);
  for (auto i = n-n; i < n; i+=1) {
    list = isl_pw_aff_list_set_pw_aff(list, i, isl_multi_pw_aff_get_pw_aff(keep(), i));
  }
  list = isl_pw_aff_list_set_pw_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_pw_aff_from_pw_aff_list(space.take(), list));
}


void MultiPwAff::sublist_inplace(pos_t first, count_t count) ISLPP_INPLACE_QUALIFIER {
  auto nOutDims = getOutDimCount();
  removeDims_inplace(isl_dim_out, first+count, nOutDims-first-count);
  removeDims_inplace(isl_dim_out, 0, first);
}


MultiPwAff MultiPwAff::sublist(const Space &subspace) ISLPP_EXSITU_QUALIFIER {
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());

  auto resultSpace = Space::createMapFromDomainAndRange(getDomainSpace(), subspace);
  auto result = resultSpace.createZeroMultiPwAff();
  auto nOutDims = range.getCount();
  for (auto i = nOutDims-nOutDims; i < nOutDims; i+=1) {
    auto aff = this->getPwAff(i+range.getFirst());
    result.setPwAff_inplace(i, aff);
  }

  return result;
}

#if 0
void MultiPwAff::sublist_inplace(const Space &subspace) ISLPP_INPLACE_QUALIFIER {
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());

#if 1
  sublist_inplace(range.getFirst(), range.getCount());
  // Unfortunately, ISL hast no cast as required here
  //cast_inplace(Space::createMapFromDomainAndRange(getDomainSpace(), subspace));
  setOutTupleId_inplace(subspace.getSetTupleId());
  assert(getRangeSpace() == subspace);
#else
  auto id = getRangeSpace().mapsTo(subspace).createIdentityMultiAff();
  *this = id.pullback(this->toPwMultiAff());
#endif
}
#endif

#if 0
MultiPwAff MultiPwAff::pullback(const MultiPwAff &that) ISLPP_EXSITU_QUALIFIER {
  auto nInDims = getInDimCount();
  assert(nInDims == that.getOutDimCount());
  assert( ::matchesSpace(this->getDomainSpace(), that.getRangeSpace()) );

  auto resultSpace = Space::createMapFromDomainAndRange(that.getDomainSpace(), getRangeSpace());
  auto result = resultSpace.createZeroMultiPwAff();
  for (auto i = nInDims-nInDims; i < nInDims; i+=1) {
    auto thisAff = this->getPwAff(i);
    auto pullbackAff = thisAff.pullback(that);
    result.setPwAff_inplace(i, pullbackAff);
  }

  return result;
}
#endif


Map MultiPwAff::reverse() ISLPP_EXSITU_QUALIFIER {
  return toMap().reverse();
}


MultiPwAff MultiPwAff::pullback(const PwMultiAff &pma) ISLPP_EXSITU_QUALIFIER {
  auto nOutDims = getOutDimCount();
  auto resultSpace = Space::createMapFromDomainAndRange(pma.getDomainSpace(), getRangeSpace());
  auto result = resultSpace.createZeroMultiPwAff();

  for (auto i = nOutDims-nOutDims; i < nOutDims; i+=1) {
    auto pwaff = getPwAff(i);
    auto pullbacked = pwaff.pullback(pma);
    result.setPwAff_inplace(i, pullbacked);
  }

  return result;
}


MultiPwAff MultiPwAff::cast(Space space) ISLPP_EXSITU_QUALIFIER {
  assert(getOutDimCount() == space.getOutDimCount());
  assert(getInDimCount() == space.getInDimCount());

  space.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(space); 

  auto result = space.move().createZeroMultiPwAff();
  auto nOutDims = result.getOutDimCount();
  for (auto i = nOutDims-nOutDims; i<nOutDims; i+=1) {
    result.setPwAff_inplace(i, meAligned.getPwAff(i));
  }
  return result;
}
