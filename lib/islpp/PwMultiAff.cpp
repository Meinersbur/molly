#include "islpp/PwMultiAff.h"

#include <llvm/Support/raw_ostream.h>
#include "islpp/Printer.h"
#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include "islpp/Map.h"

using namespace isl;
using namespace llvm;


void PwMultiAff::release() { isl_pw_multi_aff_free(take()); }

PwMultiAff PwMultiAff::create(Set &&set, MultiAff &&maff) { return enwrap(isl_pw_multi_aff_alloc(set.take(), maff.take())); }
PwMultiAff PwMultiAff::createIdentity(Space &&space) { return enwrap(isl_pw_multi_aff_identity(space.take())); }
PwMultiAff PwMultiAff::createFromMultiAff(MultiAff &&maff) { return enwrap(isl_pw_multi_aff_from_multi_aff(maff.take())); }

PwMultiAff PwMultiAff::createEmpty(Space &&space) { return enwrap(isl_pw_multi_aff_empty(space.take())); }
PwMultiAff PwMultiAff::createFromDomain(Set &&set) { return enwrap(isl_pw_multi_aff_from_domain(set.take())); }

PwMultiAff PwMultiAff::createFromSet(Set &&set) { return enwrap(isl_pw_multi_aff_from_set(set.take())); }
PwMultiAff PwMultiAff::createFromMap(Map &&map) { return enwrap(isl_pw_multi_aff_from_map(map.take())); }


Map PwMultiAff::toMap() const {
  return Map::wrap(isl_map_from_pw_multi_aff(takeCopy()));
}


void PwMultiAff::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


std::string PwMultiAff::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}


void PwMultiAff::dump() const{
  isl_pw_multi_aff_dump(keep());
}


void PwMultiAff::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


static int foreachPieceCallback(__isl_take isl_set *set, __isl_take isl_multi_aff *maff, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Set &&,MultiAff &&)>*>(user);
  auto retval = func( Set::wrap(set), MultiAff::wrap(maff) );
  return retval ? -1 : 0;
}
bool PwMultiAff::foreachPiece(const std::function<bool(Set &&,MultiAff &&)> &func) const {
  auto retval = isl_pw_multi_aff_foreach_piece(keep(), &foreachPieceCallback, const_cast<std::function<bool(Set &&,MultiAff &&)>*>(&func));
  return retval!=0;
}
