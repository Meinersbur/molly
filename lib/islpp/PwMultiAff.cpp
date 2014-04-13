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


PwMultiAff PwMultiAff::create(Set &&set, MultiAff &&maff) {
  return enwrap(isl_pw_multi_aff_alloc(set.take(), maff.take()));
}


PwMultiAff PwMultiAff::createIdentity(Space &&space) { return enwrap(isl_pw_multi_aff_identity(space.take())); }
PwMultiAff PwMultiAff::createFromMultiAff(MultiAff &&maff) { return enwrap(isl_pw_multi_aff_from_multi_aff(maff.take())); }

PwMultiAff PwMultiAff::createEmpty(Space &&space) { return enwrap(isl_pw_multi_aff_empty(space.take())); }
PwMultiAff PwMultiAff::createFromDomain(Set &&set) { return enwrap(isl_pw_multi_aff_from_domain(set.take())); }

PwMultiAff PwMultiAff::createFromSet(Set &&set) { return enwrap(isl_pw_multi_aff_from_set(set.take())); }
PwMultiAff PwMultiAff::createFromMap(Map &&map) { return enwrap(isl_pw_multi_aff_from_map(map.take())); }


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

#if 0
std::string PwMultiAff::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}


void PwMultiAff::dump() const{
  isl_pw_multi_aff_dump(keep());
}
#endif

void PwMultiAff::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  }
  else {
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
    result.unionAdd_inplace(maff.restrictDomain(domain));
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


ISLPP_EXSITU_ATTRS Map isl::Pw<MultiAff>::applyRange(Map map) ISLPP_EXSITU_FUNCTION {
  return toMap().applyRange(map);
}


void isl::Pw<MultiAff>::dumpExplicit(int maxElts, bool newlines, bool formatted) const {
  toMap().dumpExplicit(maxElts, newlines, formatted);
}


void isl::Pw<MultiAff>::dumpExplicit() const {
  toMap().dumpExplicit();
}


void isl::Pw<MultiAff>::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines, bool formatted) const {
  toMap().printExplicit(os, maxElts, newlines,formatted);
}


std::string isl::Pw<MultiAff>::toStringExplicit(int maxElts, bool newlines, bool formatted) const {
  return toMap().toStringExplicit(maxElts,newlines,formatted);
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


ISLPP_PROJECTION_ATTRS Set isl::Pw<MultiAff>::range() ISLPP_PROJECTION_FUNCTION {
  return toMap().range();
}


ISLPP_EXSITU_ATTRS Map isl::Pw<MultiAff>::applyDomain(Map map) ISLPP_EXSITU_FUNCTION {
  return toMap().applyDomain(map);
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::Pw<MultiAff>::embedIntoDomain(Space framedomainspace) ISLPP_EXSITU_FUNCTION {
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

  uint64_t result = nBsets;
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
