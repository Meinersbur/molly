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

#if 0
void PwMultiAff::release() { isl_pw_multi_aff_free(takeOrNull()); }
#endif
PwMultiAff PwMultiAff::create(Set &&set, MultiAff &&maff) { return enwrap(isl_pw_multi_aff_alloc(set.take(), maff.take())); }
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
  auto space = getDomainSpace();
  auto nPieces = this->nPieces();
   auto result = space.createZeroMultiPwAff();
   auto nOut = result.getOutDimCount();
  auto list = PwAffList::alloc(getCtx(), nOut);
 
 for (auto i=nOut-nOut; i < nOut; i+=1) {
   auto resultPwAff = space.createEmptyPwAff();

  foreachPiece([&](Set &&set, MultiAff &&maff) -> bool {
    auto aff = maff.getAff(i);  
    auto paff = PwAff::create(set, aff);
    assert(isDisjoint( resultPwAff.domain(), set)  );
      resultPwAff.unionMin_inplace(paff);
      return true;
  });

  list.setPwAff_inplace(i, resultPwAff);
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

