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
  auto nPieces = this->nPieces();
  auto result = getSpace().createZeroMultiPwAff();
  auto nOut = result.getOutDimCount();
  auto list = PwAffList::alloc(getCtx(), nOut);

  for (auto i=nOut-nOut; i < nOut; i+=1) {
    auto resultPwAff = getDomainSpace().mapsTo(1).createEmptyPwAff();

    foreachPiece([&resultPwAff,i](Set &&set, MultiAff &&maff) -> bool {
      auto aff = maff.getAff(i);
      auto paff = PwAff::create(set, aff);
      assert(isDisjoint(resultPwAff.domain(), set));
      resultPwAff.unionMin_inplace(paff); // No add_piece or disjount_union publicly available, pieces are disjount
      return false;
    });

    list.add_inplace(resultPwAff);
    result.setPwAff_inplace(i, resultPwAff);
  }
  return result;
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
  auto &func = *static_cast<std::function<bool(Set &&,MultiAff &&)>*>(user);
  auto retval = func( Set::enwrap(set), MultiAff::enwrap(maff) );
  return retval ? -1 : 0;
}
bool PwMultiAff::foreachPiece(const std::function<bool(Set &&,MultiAff &&)> &func) const {
  auto retval = isl_pw_multi_aff_foreach_piece(keep(), &foreachPieceCallback, const_cast<std::function<bool(Set &&,MultiAff &&)>*>(&func));
  return retval!=0;
}


Map PwMultiAff::reverse() const {
  return toMap().reverse();
}


PwMultiAff PwMultiAff::neg() const {
  auto space = getSpace();
  auto result = space.createEmptyPwMultiAff();

  foreachPiece([&result] (Set &&domain, MultiAff &&maff) -> bool {
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


PwMultiAff PwMultiAff:: projectOut(const DimRange &range) const {
  assert(range.isValid());
  assert(range.getType() == isl_dim_out);
  return projectOut(range.getBeginPos(), range.getCount());
}


PwMultiAff PwMultiAff::projectOutSubspace(const Space &subspace) const {
  auto dimrange = getSpace().findSubspace(isl_dim_out, subspace);
  return projectOut(dimrange);
}


PwMultiAff PwMultiAff::sublist(pos_t first, count_t count) ISLPP_EXSITU_QUALIFIER {
  auto domainSpace = getDomainSpace();
  auto resultSpace = domainSpace.mapsTo(count);
  auto result = resultSpace.createEmptyPwMultiAff();

  foreachPiece([first,count,&result](Set &&set, MultiAff &&aff) -> bool {
    auto subaff = aff.subMultiAff(first, count);
    auto subpwaff = PwMultiAff::create(set, subaff);
    result.unionAdd_inplace(subpwaff); //FIXME: disjointAdd
    return false;
  });

  return result;
}


ISLPP_EXSITU_ATTRS PwMultiAff PwMultiAff::sublist(Space subspace) ISLPP_EXSITU_QUALIFIER {
  auto range = getSpace().findSubspace(isl_dim_out, subspace);
  assert(range.isValid());

  auto result = sublist(range.getFirst(), range.getCount());
  result.cast_inplace(getDomainSpace().mapsTo(subspace));
  return result;
}


PwMultiAff PwMultiAff::cast(Space space) ISLPP_EXSITU_QUALIFIER {
  assert(getOutDimCount() == space.getOutDimCount());
  assert(getInDimCount() == space.getInDimCount());

  space.alignParams_inplace(this->getSpace());
  auto meAligned = this->alignParams(space);

  auto result = space.move().createEmptyPwMultiAff();
  meAligned.foreachPiece([&result,&space](Set&&set,MultiAff&&maff) -> bool {
    result.unionAdd_inplace(PwMultiAff::create(set.cast(space.getDomainSpace()), maff.cast(space))); // TODO: disjointUnion/addPiece 
    return false;
  });

  return result;
}
