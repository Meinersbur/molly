#include "islpp/MultiPwAff.h"

#include "islpp/Printer.h"
#include "islpp/Map.h"
#include <llvm/Support/raw_ostream.h>

using namespace isl;


Map MultiPwAff::toMap() const {
  auto space = getDomainSpace().fromDomain();
  auto result = space.universeMap();

  auto nDims = getOutDimCount();
  for (auto i = nDims - nDims; i < nDims; i += 1) {
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
  for (auto i = nDims - nDims; i < nDims; i += 1) {
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


/* implicit */  Multi<PwAff>::Multi(const MultiAff &madd)
: Obj(madd.isValid() ? madd.toMultiPwAff().take() : nullptr) {
}


const MultiPwAff &Multi<PwAff>::operator=(const MultiAff &madd) ISLPP_INPLACE_FUNCTION{
  give(madd.isValid() ? madd.toMultiPwAff().take() : nullptr);
  return *this;
}


/* implicit */ Multi<PwAff>::Multi(const PwMultiAff &pma)
: Obj(pma.isValid() ? pma.toMultiPwAff().take() : nullptr) {}


const MultiPwAff &Multi<PwAff>::operator=(const PwMultiAff &pma) ISLPP_INPLACE_FUNCTION{
  give(pma.isValid() ? pma.toMultiPwAff().take() : nullptr);
  return *this;
}


/* implicit */ Multi<PwAff>::Multi(PwAff pa)
: Obj(pa.isValid() ? pa.toMultiPwAff().take() : nullptr) {
}


ISLPP_INPLACE_ATTRS const MultiPwAff &Multi<PwAff>::operator=(PwAff pa) ISLPP_INPLACE_FUNCTION{
  reset(pa.isValid() ? pa.toMultiPwAff().take() : nullptr);
  return *this;
}


void Multi<PwAff>::push_back(PwAff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_pw_aff_list_alloc(isl_multi_pw_aff_get_ctx(keep()), n + 1);
  for (auto i = n - n; i < n; i += 1) {
    list = isl_pw_aff_list_set_pw_aff(list, i, isl_multi_pw_aff_get_pw_aff(keep(), i));
  }
  list = isl_pw_aff_list_set_pw_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_pw_aff_from_pw_aff_list(space.take(), list));
}


ISLPP_EXSITU_ATTRS MultiPwAff isl::Multi<PwAff>::pullback(MultiAff ma) ISLPP_EXSITU_FUNCTION
{
  return MultiPwAff::enwrap(isl_multi_pw_aff_pullback_multi_aff(takeCopy(), ma.take()));
}


ISLPP_EXSITU_ATTRS MultiPwAff isl::Multi<PwAff>::pullback(MultiPwAff that) ISLPP_EXSITU_FUNCTION{
  // isl_multi_pw_aff_pullback_multi_pw_aff has exponential blowup because it calls isl_pw_multi_aff_from_multi_pw_aff
#if 0
  //return MultiPwAff::enwrap(isl_multi_pw_aff_pullback_multi_pw_aff(takeCopy(), that.take()));
  auto refresult = MultiPwAff::enwrap(isl_multi_pw_aff_pullback_multi_pw_aff(takeCopy(), that.takeCopy()));
#endif
  auto self = *this;
  isl::compatibilize(self, that);

  // Algorithm that avoids blowup if conditions that make the the pieces are projected away
  auto paramSpace = self.getParamsSpace();
  auto domainSpace = that.getDomainSpace();
  auto midSpace = self.getDomainSpace();
  assert(isl::matchesSpace(midSpace, that.getRangeSpace()));
  auto rangeSpace = self.getRangeSpace();
  auto domainDims = domainSpace.getDimCount();
  auto midDims = midSpace.getDimCount();
  auto rangeDims = rangeSpace.getDimCount();
  //auto resultSpace = Space::createMapFromDomainAndRange(domainSpace, rangeSpace);

  self.cast_inplace();
  that.cast_inplace();

  self.insertDims_inplace(isl_dim_in, 0, domainDims);
  auto tmpSpace = self.getDomainSpace();

  auto result = getCtx()->createMapSpace(domainDims, rangeDims).alignParams(midSpace).createEmptyMultiPwAff();

  for (auto i = rangeDims - rangeDims; i < rangeDims; i += 1) {
    auto secondpwaff = self.getPwAff(i); // { [*,*,c,d] -> [e(c,b)] }








    for (auto jp = midDims; jp >0; jp -= 1) {
      auto j = jp - 1;
      auto firstpwaff = that.getPwAff(j); // { [a,b] -> [d(a,b)] }
      firstpwaff.addDims_inplace(isl_dim_in, j); // { [a,b,c] -> [d(a,b)] }

      auto eqaff = paramSpace.createIdentityMultiAff(domainDims + j); // { [a,b,c] -> [a,b,c] }
      auto subspaceaff = isl::flatRangeProduct(eqaff, firstpwaff); // { [a,b,c] -> [a,b,c,d(a,b)] }

      secondpwaff.pullback_inplace(subspaceaff); // { [a,b,c] -> [f(c,d(a,b),e(a,b))] }
      secondpwaff.coalesce_inplace(); // to avoid exponential explosion when the output is not
#ifndef NDEBUG
      auto c = secondpwaff.getComplexity();
#endif



















    }

    result[i] = secondpwaff;
  }

  auto castedResult = result.cast(domainSpace, rangeSpace);
  //assert(castedResult == refresult);
  return castedResult;
}


ISLPP_EXSITU_ATTRS MultiPwAff isl::Multi<PwAff>::pullback(PwMultiAff that) ISLPP_EXSITU_FUNCTION{
  return MultiPwAff::enwrap(isl_multi_pw_aff_pullback_pw_multi_aff(takeCopy(), that.take()));
}


ISLPP_INPLACE_ATTRS void isl::Multi<PwAff>::pullback_inplace(MultiAff ma) ISLPP_INPLACE_FUNCTION{
  give(isl_multi_pw_aff_pullback_multi_aff(take(), ma.take()));
}


ISLPP_INPLACE_ATTRS void isl::Multi<PwAff>::pullback_inplace(PwMultiAff that) ISLPP_INPLACE_FUNCTION{
  give(isl_multi_pw_aff_pullback_pw_multi_aff(take(), that.take()));
}



ISLPP_INPLACE_ATTRS void isl::Multi<PwAff>::castDomain_inplace(SetSpace domainSpace) ISLPP_INPLACE_FUNCTION{
  auto domainMap = Space::createMapFromDomainAndRange(domainSpace, getDomainSpace()).createIdentityMultiAff();
  pullback_inplace(std::move(domainMap));
}


ISLPP_INPLACE_ATTRS void isl::Multi<PwAff>::castDomain_inplace() ISLPP_INPLACE_FUNCTION{
  // give(isl_multi_pw_aff_move_dims(take(), isl_dim_in, 0, isl_dim_param, 0, 0));
  //give(isl_multi_pw_aff_insert_dims(take(), isl_dim_in, 0, 0));
  castDomain_inplace(getCtx()->createSetSpace(getInDimCount()));
}



ISLPP_EXSITU_ATTRS Set isl::Multi<PwAff>::getDomain() ISLPP_EXSITU_FUNCTION{
  return Set::enwrap(isl_multi_pw_aff_domain(takeCopy()));
}


ISLPP_EXSITU_ATTRS Set isl::Multi<PwAff>::getRange() ISLPP_EXSITU_FUNCTION{
  return getDomain().apply(toMap());
}


ISLPP_EXSITU_ATTRS Map isl::Multi<PwAff>::applyRange(Map map) ISLPP_EXSITU_FUNCTION{
  return toMap().applyRange(map);
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::Multi<PwAff>::applyRange(PwMultiAff pma) ISLPP_EXSITU_FUNCTION{
  return pma.pullback(*this);
}


void isl::Multi<PwAff>::dump() const {
  if (!isValid())
    fprintf(stderr, "NULL");
  isl_multi_pw_aff_dump(keep());
}


std::string isl::Multi<PwAff>::toStringExplicit() const  {
  return toStringExplicit(8, false, false, true);
}


std::string isl::Multi<PwAff>::toStringExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const  {
  return toMap().toStringExplicit(maxElts, newlines, formatted, sorted);
}


void isl::Multi<PwAff>::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted /*= true*/) const{
  toMap().printExplicit(os, maxElts, newlines, formatted, sorted);
}


void isl::Multi<PwAff>::dumpExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const  {
  toMap().dumpExplicit(maxElts, newlines, formatted, sorted);
}


void isl::Multi<PwAff>::dumpExplicit() const  {
  dumpExplicit(8, false, false, true);
}


ISLPP_EXSITU_ATTRS MultiPwAff isl::MultiPwAff::embedIntoDomainSpace(Space framespace) ISLPP_EXSITU_FUNCTION{
  assert(framespace.isSet());
  auto domainSpace = getDomainSpace();
  auto rangeSpace = getRangeSpace();
  auto nRangeDims = getOutDimCount();
  auto range = framespace.findSubspace(isl_dim_set, domainSpace);
  assert(range.isValid());

  auto outSpace = framespace.replaceSubspace(domainSpace, rangeSpace);
  auto resultSpace = Space::createMapFromDomainAndRange(framespace, outSpace);
  auto result = resultSpace.createZeroMultiPwAff();
  pos_t i = 0;
  auto nFirst = range.getFirst();
  for (; i < nFirst; i += 1) {
    result.setPwAff_inplace(i, framespace.createAffOnVar(i));
  }
  auto nEnd = range.getEnd();
  for (; i < nEnd; i += 1) {
    result.setPwAff_inplace(i, this->getPwAff(i - nFirst));
  }
  auto nDims = resultSpace.getOutDimCount();
  for (; i < nDims; i += 1) {
    result.setPwAff_inplace(i, framespace.createAffOnVar(nFirst + nRangeDims + (i - nEnd)));
  }
  assert(i == framespace.getSetDimCount());

  return result;
}


ISLPP_EXSITU_ATTRS MultiPwAff isl::MultiPwAff::embedIntoRangeSpace(Space frameRangeSpace) ISLPP_EXSITU_FUNCTION{
  assert(frameRangeSpace.isSet());
  auto domainSpace = getDomainSpace();
  auto rangeSpace = getRangeSpace();
  auto nDomainDims = getInDimCount();
  auto nRangeDims = getOutDimCount();
  auto range = frameRangeSpace.findSubspace(isl_dim_set, rangeSpace);
  assert(range.isValid());

  auto inSpace = frameRangeSpace.replaceSubspace(rangeSpace, domainSpace);
  auto resultSpace = Space::createMapFromDomainAndRange(inSpace, frameRangeSpace);
  auto result = resultSpace.createZeroMultiPwAff();
  auto nFirst = range.getFirst();
  for (auto i = nFirst - nFirst; i < nFirst; i += 1) {
    result.setPwAff_inplace(i, inSpace.createAffOnVar(i));
  }
  auto nPostfix = frameRangeSpace.getSetDimCount() - range.getEnd();
  for (auto i = nDomainDims - nDomainDims; i < nDomainDims; i += 1) {
    auto aff = this->getPwAff(i);
    aff.insertDims_inplace(isl_dim_in, 0, nFirst);
    aff.addDims_inplace(isl_dim_in, nPostfix);
    aff.castDomain_inplace(inSpace);
    result.setPwAff_inplace(nFirst + i, aff);
  }

  for (auto i = nPostfix - nPostfix; i < nPostfix; i += 1) {
    result.setPwAff_inplace(nFirst + nDomainDims + i, inSpace.createAffOnVar(nFirst + nRangeDims + i));
  }

  return result;
}


ISLPP_EXSITU_ATTRS Map isl::MultiPwAff::projectOut(isl_dim_type type, pos_t first, count_t count) ISLPP_EXSITU_FUNCTION{
  return toMap().projectOut(type, first, count);
}


void MultiPwAff::sublist_inplace(pos_t first, count_t count) ISLPP_INPLACE_FUNCTION{
  auto nOutDims = getOutDimCount();
  removeDims_inplace(isl_dim_out, first + count, nOutDims - first - count);
  removeDims_inplace(isl_dim_out, 0, first);
}


MultiPwAff MultiPwAff::sublist(const Space &subspace) ISLPP_EXSITU_FUNCTION{
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());

  auto resultSpace = Space::createMapFromDomainAndRange(getDomainSpace(), subspace);
  auto result = resultSpace.createZeroMultiPwAff();
  auto nOutDims = range.getCount();
  for (auto i = nOutDims - nOutDims; i < nOutDims; i += 1) {
    auto aff = this->getPwAff(i + range.getFirst());
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


Map MultiPwAff::reverse() ISLPP_EXSITU_FUNCTION{
  return toMap().reverse();
}

#if 0
ISLPP_EXSITU_ATTRS MultiPwAff MultiPwAff::pullback(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION {
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
#endif

ISLPP_EXSITU_ATTRS MultiPwAff MultiPwAff::castRange(SetSpace space) ISLPP_EXSITU_FUNCTION{
  assert(getOutDimCount() == space.getSetDimCount());

  space.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(space);

  auto result = Space::createMapFromDomainAndRange(meAligned.getDomainSpace(), space).createZeroMultiPwAff();
  auto nOutDims = result.getOutDimCount();
  for (auto i = nOutDims - nOutDims; i < nOutDims; i += 1) {
    result.setPwAff_inplace(i, meAligned.getPwAff(i));
  }
  return result;
}

#if 0
ISLPP_EXSITU_ATTRS Map MultiPwAff::createLtMap() ISLPP_EXSITU_FUNCTION{
  auto lt = BasicMap::createLt(getDomainSpace());
  return applyRange(lt);
}


ISLPP_EXSITU_ATTRS Map MultiPwAff::createGeMap() ISLPP_EXSITU_FUNCTION{
  auto ge = BasicMap::createLt(getDomainSpace());
  return applyRange(ge);
}
#endif

MultiPwAff isl::operator+(MultiPwAff lhs, MultiPwAff rhs) {
  assert(lhs.getSpace() == rhs.getSpace());
  auto nDims = lhs.getOutDimCount();
  assert(nDims == rhs.getOutDimCount());

  auto result = MultiPwAff::createZero(lhs.getSpace());
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto lhsAff = lhs[i];
    auto rhsAff = rhs[i];
    auto addAff = lhsAff + rhsAff;
    result[i] = addAff;
  }
  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t isl::MultiPwAff::getComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDims = getOutDimCount();
  uint64_t result = 0;
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    result += getPwAff(i).getComplexity();
  }
  // result -= (nDims - 1)*((uint64_t)1 << 32); // Every dim has at least one piece, therefore even the simplest MultiPwAff as complexity ot nDims*2^32
  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t isl::MultiPwAff::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDims = getOutDimCount();
  uint64_t result = 0;
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    result += getPwAff(i).getOpComplexity();
  }
  return result;
}


ISLPP_INPLACE_ATTRS void MultiPwAff::gistUndefined_inplace() ISLPP_INPLACE_FUNCTION{
  auto n = getOutDimCount();
  auto definedDomain = getDomainSpace().createUniverseSet();
  for (auto i = n - n; i < n; i += 1) {
    auto aff = getPwAff(i);
    definedDomain.intersect_inplace(aff.domain());
    if (aff.nPiece() == 1) {
      // There is just one piece; extend it to universe
      // isl_multi_pw_aff doesn't do this because it always works on the context's hull
      auto piece = aff.anyPiece();
      setPwAff_inplace(i, std::move(piece.second));
    }
  }
  gist_inplace(std::move(definedDomain));
}
