#include "islpp/PwMultiAff.h"

#include <llvm/Support/raw_ostream.h>
#include "islpp/Printer.h"
#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include "islpp/Map.h"
#include "islpp/MultiPwAff.h"
#include "islpp/PwAffList.h"
#include "islpp/DimRange.h"

using namespace isl;
using namespace llvm;
using namespace std;



PwMultiAff PwMultiAff::createIdentity(Space &&space) { 
  return enwrap(isl_pw_multi_aff_identity(space.take())); 
}


PwMultiAff PwMultiAff::createFromMultiAff(MultiAff &&maff) { 
  return enwrap(isl_pw_multi_aff_from_multi_aff(maff.take()));
}


PwMultiAff PwMultiAff::createEmpty(Space &&space) { 
  return enwrap(isl_pw_multi_aff_empty(space.take())); 
}


PwMultiAff PwMultiAff::createFromDomain(Set &&set) { 
  return enwrap(isl_pw_multi_aff_from_domain(set.take())); 
}


PwMultiAff PwMultiAff::createFromSet(Set &&set) { 
  return enwrap(isl_pw_multi_aff_from_set(set.take()));
}


PwMultiAff PwMultiAff::createFromMap(Map &&map) { 
  return enwrap(isl_pw_multi_aff_from_map(map.take()));
}


Map PwMultiAff::toMap() const {
  return Map::enwrap(isl_map_from_pw_multi_aff(takeCopy()));
}


MultiPwAff PwMultiAff::toMultiPwAff() const {
  return MultiPwAff::enwrap(isl_multi_pw_aff_from_pw_multi_aff(takeCopy()));
}


void PwMultiAff::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


void PwMultiAff::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


Set PwMultiAff::getRange() const {
  return toMap().getRange();
}

#if 0
PwMultiAff PwMultiAff::pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER {
  auto resultSpace = Space::createMapFromDomainAndRange(mpa.getDomainSpace(), this->getRangeSpace());
  auto result = resultSpace.createEmptyPwMultiAff();

  foreachPiece([&result,&mpa] (const Set &set, const MultiAff &ma) -> bool {
    auto pullbacked = ma.pullback(mpa);
    result.unionAdd_inplace(pullbacked);
    return false;
  });

  return result;
}
#endif

static int foreachPieceCallback(__isl_take isl_set *set, __isl_take isl_multi_aff *maff, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Set &&, MultiAff &&)>*>(user);
  auto retval = func(Set::enwrap(set), MultiAff::enwrap(maff));
  return retval ? -1 : 0;
}
bool PwMultiAff::foreachPiece(const std::function<bool(Set &&, MultiAff &&)> &func) const {
  auto retval = isl_pw_multi_aff_foreach_piece(keep(), &foreachPieceCallback, const_cast<std::function<bool(Set &&, MultiAff &&)>*>(&func));
  return retval != 0;
}


Map PwMultiAff::reverse() const {
  return toMap().reverse();
}


PwMultiAff PwMultiAff::neg() const {
  auto space = getSpace();
  auto result = space.createEmptyPwMultiAff();

  foreachPiece([&result](Set &&domain, MultiAff &&maff) -> bool {
    maff.neg_inplace();
    result.unionAdd_inplace(maff.intersectDomain(domain));
    return false;
  });

  return result;
}


Set PwMultiAff::wrap() const {
  return toMap().wrap();
}


PwMultiAff PwMultiAff::projectOut(unsigned first, unsigned count) const {
  return removeDims(isl_dim_out, first, count);
}


PwMultiAff PwMultiAff::projectOut(const DimRange &range) const {
  assert(range.isValid());
  assert(range.getType() == isl_dim_out);
  return projectOut(range.getBeginPos(), range.getCount());
}


PwMultiAff PwMultiAff::projectOutSubspace(const Space &subspace) const {
  auto dimrange = getSpace().findSubspace(isl_dim_out, subspace);
  return projectOut(dimrange);
}


PwMultiAff PwMultiAff::sublist(pos_t first, count_t count) ISLPP_EXSITU_FUNCTION{
  auto domainSpace = getDomainSpace();
  auto resultSpace = domainSpace.mapsTo(count);
  auto result = resultSpace.createEmptyPwMultiAff();

  foreachPiece([first, count, &result](Set &&set, MultiAff &&aff) -> bool {
    auto subaff = aff.subMultiAff(first, count);
    auto subpwaff = PwMultiAff::create(set, subaff);
    result.unionAdd_inplace(subpwaff); //FIXME: disjointAdd
    return false;
  });

  return result;
}


ISLPP_EXSITU_ATTRS PwMultiAff PwMultiAff::sublist(Space subspace) ISLPP_EXSITU_FUNCTION{
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());

  auto result = sublist(range.getFirst(), range.getCount());
  result.cast_inplace(getDomainSpace().mapsTo(subspace));
  return result;
}


PwMultiAff PwMultiAff::cast(Space space) ISLPP_EXSITU_FUNCTION{
  assert(getOutDimCount() == space.getOutDimCount());
  assert(getInDimCount() == space.getInDimCount());

  space.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(space);

  auto result = space.move().createEmptyPwMultiAff();
  meAligned.foreachPiece([&result, &space](Set&&set, MultiAff&&maff) -> bool {
    result.unionAdd_inplace(PwMultiAff::create(set.cast(space.getDomainSpace()), maff.cast(space))); // TODO: disjointUnion/addPiece 
    return false;
  });

  return result;
}


ISLPP_EXSITU_ATTRS PwMultiAff PwMultiAff::castDomain(Space domainSpace) ISLPP_EXSITU_FUNCTION{
  assert(getInDimCount() == domainSpace.getSetDimCount());

  domainSpace.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(domainSpace);

  auto result = Space::createMapFromDomainAndRange(domainSpace, getRangeSpace()).createEmptyPwMultiAff();
  meAligned.foreachPiece([&result, &domainSpace](Set&&set, MultiAff&&maff) -> bool {
    auto resultPiece = PwMultiAff::create(set.cast(domainSpace), maff.castDomain(domainSpace));
    result.unionAdd_inplace(resultPiece.move()); // TODO: disjointUnion/addPiece 
    return false;
  });

  return result;
}


ISLPP_INPLACE_ATTRS void PwMultiAff::castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION{
  obj_give(castDomain(domainSpace));
}


ISLPP_EXSITU_ATTRS PwMultiAff PwMultiAff::castRange(Space rangeSpace) ISLPP_EXSITU_FUNCTION{
  assert(getOutDimCount() == rangeSpace.getSetDimCount());

  rangeSpace.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(rangeSpace);

  auto result = Space::createMapFromDomainAndRange(getDomainSpace(), rangeSpace).createEmptyPwMultiAff();
  meAligned.foreachPiece([&result, &rangeSpace](Set &&set, MultiAff &&maff) -> bool {
    auto resultPiece = PwMultiAff::create(set.move(), maff.castRange(rangeSpace));
    result.unionAdd_inplace(resultPiece.move()); // TODO: disjointUnion/addPiece 
    return false;
  });

  return result;
}


ISLPP_EXSITU_ATTRS Map isl::Pw<MultiAff>::applyRange(Map map) ISLPP_EXSITU_FUNCTION{
  return toMap().applyRange(map);
}


void isl::Pw<MultiAff>::dumpExplicit(int maxElts, bool newlines, bool formatted,bool sorted) const {
  toMap().dumpExplicit(maxElts, newlines, formatted,sorted);
}


void isl::Pw<MultiAff>::dumpExplicit() const {
  toMap().dumpExplicit();
}


void isl::Pw<MultiAff>::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines, bool formatted, bool sorted) const {
  toMap().printExplicit(os, maxElts, newlines, formatted,sorted);
}


std::string isl::Pw<MultiAff>::toStringExplicit(int maxElts, bool newlines, bool formatted, bool sorted) const {
  return toMap().toStringExplicit(maxElts, newlines, formatted, sorted);
}


std::string isl::Pw<MultiAff>::toStringExplicit() const {
  return toMap().toStringExplicit();
}


std::string isl::Pw<MultiAff>::toString() const {
  return ObjBaseTy::toString();
}


isl::Pw<MultiAff>::Pw(MultiPwAff that) : Obj(that.isValid() ? that.toPwMultiAff() : PwMultiAff()) {
}


const PwMultiAff & isl::Pw<MultiAff>::operator=(MultiPwAff that) {
  obj_give(that.isValid() ? that.toPwMultiAff() : PwMultiAff()); return *this;
}


void isl::Pw<MultiAff>::dump() const {
  isl_pw_multi_aff_dump(keep());
}


ISLPP_PROJECTION_ATTRS Set isl::Pw<MultiAff>::range() ISLPP_PROJECTION_FUNCTION{
  return toMap().range();
}


ISLPP_EXSITU_ATTRS Map isl::Pw<MultiAff>::applyDomain(Map map) ISLPP_EXSITU_FUNCTION{
  return toMap().applyDomain(map);
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::Pw<MultiAff>::embedIntoDomain(Space framedomainspace) ISLPP_EXSITU_FUNCTION{
  auto subspace = getDomainSpace();
  auto myspace = getSpace();
  auto range = myspace.findSubspace(isl_dim_in, subspace);
  auto islctx = getCtx();

  auto before = islctx->createSetSpace(range.getCountBefore()).createIdentityMultiAff();
  auto after = islctx->createSetSpace(range.getCountAfter()).createIdentityMultiAff();
  return isl::product(before, *this, after).cast(framedomainspace, framedomainspace.replaceSubspace(getDomainSpace(), getRangeSpace()));
}


static int countPiecesCallback(__isl_take isl_set *set, __isl_take isl_multi_aff *aff, void *user) {
  auto count = static_cast<unsigned *>(user);
  *count += 1;
  return 0;
}
unsigned isl::Pw<MultiAff>::nPieces() const {
  unsigned result = 0;
  auto retval = isl_pw_multi_aff_foreach_piece(keep(), countPiecesCallback, &result);
  assert(retval == 0);
  return result;
}


static int enumPiecesCallback(__isl_take isl_set *set, __isl_take isl_multi_aff *aff, void *user) {
  auto list = static_cast<std::vector<std::pair<Set, MultiAff>> *>(user);
  list->push_back(std::make_pair(Set::enwrap(set), MultiAff::enwrap(aff)));
  return 0;
}
std::vector<std::pair<Set, MultiAff>> PwMultiAff::getPieces() const {
  std::vector<std::pair<Set, MultiAff>> result;
  result.reserve(this->nPieces());
  auto retval = isl_pw_multi_aff_foreach_piece(keep(), enumPiecesCallback, &result);
  assert(retval == 0);
  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t PwMultiAff::getComplexity() ISLPP_PROJECTION_FUNCTION{
  uint32_t nBsets = 0;
  uint32_t affComplexity = 0;
  uint32_t bsetComplexity = 0;

  for (const auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    // The impact of the expression itself is actually very minor and only one really needs to be evaluated. We assume the most complex one
    affComplexity = std::max(affComplexity, aff.getComplexity());

    for (auto const &bset : set.getBasicSets()) {// for an input vector need to determine in which bset it is; which aff it resolves to is of minor importance
      nBsets += 1;
      bsetComplexity += bset.getComplexity();
    }
  }

  if (nBsets == 0)
    return 0;
  uint64_t result = (nBsets - 1);
  result <<= 32;
  result |= bsetComplexity + affComplexity;
  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t PwMultiAff::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  uint64_t complexity = 0;
  int nPieces = 0;

  for (const auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    complexity += set.getOpComplexity();
    complexity += aff.getOpComplexity();
    nPieces += 1;
  }

  if (nPieces == 0)
    return 0;
  complexity += nPieces - 1; // select for matching piece; just last one doesn't need a select
  return complexity;
}


/// Given that all coefficients of maff and baff are equal, but not the constants; find a common expression that safisfies maff on mset and baff on bset
/// True is returned if succeeded and maff contains the common aff
/// If failing, returns false. maff is undefined in this case
template<typename BasicSet>
static bool tryCombineConstant(Set mset, Aff &maff, BasicSet bset, Aff baff, isl_dim_type type, bool tryDivs) {
  Int mval, bval; // For reuse
  auto nDims = maff.dim(type);

  auto affspace = maff.getSpace();
  auto mcst = maff.getConstant();
  auto bcst = baff.getConstant();
  if (mcst == bcst)
    return true;

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto mcoeff = maff.getCoefficient(type, i);
    auto bcoeff = baff.getCoefficient(type, i);

    if (!bset.isFixed(type == isl_dim_param ? isl_dim_param : isl_dim_set, i, bval)) // TODO: Can also work with fixed range bounds that div to different constants
      continue;
    if (!mset.isFixed(type == isl_dim_param ? isl_dim_param : isl_dim_set, i, mval))
      continue;
    if (bval == mval)
      continue; // No chance to make a difference

    // goal: find coeff s.t.
    // new_mcst + coeff*mval = mcst 
    // new_bcst + coeff*bval = bcst 
    // new_mcst = new_bcst <=> mcst - coeff*mval = bcst - coeff*bval
    // <=> mcst - bcst = coeff*mval - coeff*bval <=> mcst - bcst = coeff*(mval - bval)
    // <=> coeff = (mcst - bcst)/(mval - bval) 
    auto dcst = mcst - bcst;
    auto dval = mval - bval;
    if (mcoeff.isZero() && bcoeff.isZero() && isDivisibleBy(dcst, dval)) {
      auto coeff = divexact(dcst, dval);
      maff.setCoefficient_inplace(type, i, coeff);
      maff.addConstant_inplace(-coeff*mval);
#ifndef NDEBUG
      baff.setCoefficient_inplace(type, i, coeff);
      baff.addConstant_inplace(-coeff*bval);
      assert(maff == baff);
#endif /* NDEBUG */
      return true;
    }
    //TODO: Non-div solution might be possible using multiple such dimensions

    assert(!"Untested code!");
    if (!tryDivs)
      continue;

    if (mval > 0 && bval >= 0 && mval > bval) {
      // floord(mval,mval)==1 but floord(bval,mval)==0
      auto divisoraff = affspace.createVarAff(type, i);
      auto floordiv = divisoraff.divBy(mval);
      auto dfloordiv = dcst*floordiv;
      maff -= dfloordiv;
      maff += dcst;
#ifndef NDEBUG
      baff -= floordiv;
      baff += dcst;
      assert(maff == baff); // Might fail
#endif
      return true;
    }

    if (bval > 0 && mval >= 0 && bval > mval) {
      // floord(bval,bval)==1 but floord(mval,bval)==0
      auto divisoraff = affspace.createVarAff(type, i);
      auto floordiv = divisoraff.divBy(bval);
      auto dfloordiv = dcst*floordiv;
      maff += dfloordiv;
      maff -= dcst;
#ifndef NDEBUG
      baff += floordiv;
      baff -= dcst;
      assert(maff == baff); // Might fail
#endif
      return true;
    }

    // TODO: Add cases for negative
  }

  return false;
}


/// Merge as many affine pieces as possible
/// Greedy algorithm
template<typename BasicSet>
static void combineMultiAffs(std::vector<std::pair<Set, MultiAff>> &merged, std::vector<std::pair<BasicSet, MultiAff>> &affs, bool tryCoeffs, bool tryDivs) {
  auto n = affs.size();
  merged.reserve(n);
  while (n) {
    assert(n == affs.size());
    Set set = affs[n - 1].first;
    auto aff = affs[n - 1].second;
    affs.pop_back();
    n -= 1; assert(n == affs.size());

    auto ip = n;
    while (ip) {
      auto i = ip - 1;
      const auto &bset = affs[i].first;
      const auto &baff = affs[i].second;

      if (isl::tryCombineMultiAff(set, aff, bset, baff, tryCoeffs, tryDivs, set, aff)) {
        affs.erase(affs.begin() + i);
        n -= 1;
      } else {
        int a = 0;
      }
      ip -= 1;
    }

    merged.emplace_back(std::move(set), std::move(aff));
  }
}


template<typename BasicSet>
static PwMultiAff fromList(const std::vector<pair<BasicSet, MultiAff>> &list) {
  const auto &any = list[0];
  auto result = any.second.getSpace().createEmptyPwMultiAff();
  for (const auto &pair : list) {
   const auto &set = pair.first;
   const auto &aff = pair.second;
    result.unionAdd_inplace(PwMultiAff::create(set, aff));
  }
  return result;
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::PwMultiAff::simplify() ISLPP_EXSITU_FUNCTION{
  if (nPieces()==0)
  return *this;

  auto nParamDims = getParamDimCount();
  auto nDomainDims = getInDimCount();

  // Normalize affs
  // TODO: Because this is done per BasicSet -- not per piece -- the same aff can gist differently, leading to that those cannot be merged again
  // one of those affs can be simpler than the other since its domain is more restrictive
  // Still have to consider whether this is a bad thing since at least one of the expressions is simpler than before; 
  // the total number of BasicSets from all pieces does not worsen which is what matters (does it?)
  std::vector<std::pair<Set, MultiAff>> affs;
  for (auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    // Normalize the divs to the way they are added by isl_aff_floor
    aff.normalizeDivs_inplace();

    aff.gist_inplace(set);
    affs.emplace_back(set, std::move(aff));

#if 0
    auto nPieces = set.getBasicSetCount();
    set.makeDisjoint_inplace();
    auto nPieces2 = set.getBasicSetCount();
    if (nPieces < nPieces2) {
      int b = 0;
    }

    for (const auto &bset : set.getBasicSets()){
      //auto normalizedAff = aff;
      //bset.detectEqualities_inplace(); // make isl_basic_set_is_fixed work
      auto normalizedAff = aff.gist(bset);

      affs.emplace_back(bset, std::move(normalizedAff));
    }
#endif
  }
#if 0
  if (fromList(affs) != *this) {
    auto ref = fromList(affs);
    auto diff1 = ref - *this;
    auto diffDom1 = ref.domain() - domain();
    diff1.dump();
    auto diff2 = *this - ref;
    auto diffDom2 = domain() - ref.domain();
    diff2.dump();
    int a = 0;
    simplify();
  }
#endif

  std::vector<pair<Set, MultiAff>> merged;
  combineMultiAffs(merged, affs, false, false);
  if (fromList(merged) != *this) {
    int a = 0;
  }

  std::vector<pair<Set, MultiAff>> coeffmerged;
  combineMultiAffs(coeffmerged, merged, true, false);
  if (fromList(coeffmerged) != *this) {
    int a = 0;
  }

  std::vector<pair<Set, MultiAff>> divmerged;
  combineMultiAffs(divmerged, coeffmerged, true, true);
  if (fromList(divmerged) != *this) {
    auto x = simplify();
    int a = 0;
  }

  auto result = getSpace().createEmptyPwMultiAff();
  for (auto &pair : divmerged) {
    auto &set = pair.first;
    set.coalesce_inplace();
    auto &aff = pair.second; 
    result.unionAdd_inplace(PwMultiAff::create(std::move(set), std::move(aff)));
  }
  assert(*this == result);

  auto beforeComplexity = getComplexity();
  auto afterComplexity = result.getComplexity();
#ifndef NDEBUG
  auto beforeOpComplexity = getOpComplexity();
  auto afterOpComplexity = result.getOpComplexity();

  if (beforeComplexity < afterComplexity) {
    int a = 0;
  }
  if (beforeOpComplexity < afterOpComplexity) {
    int b = 0;

  }
#endif /* NDEBUG */

  if (afterComplexity > beforeComplexity) {
    result = copy();
  }

  //if (*this != result) {
  //  auto x = simplify();
  //}
  return result; // NRVO
}


ISLPP_INPLACE_ATTRS void PwMultiAff::gistUndefined_inplace() ISLPP_INPLACE_FUNCTION{
  auto definedDomain = domain();
  gist_inplace(std::move(definedDomain));
}
