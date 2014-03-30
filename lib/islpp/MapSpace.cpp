#include "islpp_impl_common.h"
#include "Ctx.h"
#include "Space.h"
#include "islpp/MapSpace.h"

using namespace isl;


void MapSpace::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void MapSpace::dump() const {
  isl_space_dump(keep());
}


ISLPP_PROJECTION_ATTRS Ctx *MapSpace::getCtx() ISLPP_PROJECTION_FUNCTION {
  return Ctx::enwrap(isl_space_get_ctx(keep()));
}


MapSpace::operator Space() const { 
  return Space::enwrap(takeCopy());
}
