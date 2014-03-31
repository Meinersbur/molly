#include "islpp_impl_common.h"
#include "islpp/SetSpace.h"

#include "islpp/Space.h"
#include "islpp/Printer.h"

using namespace isl;

#if 0
void SetSpace::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void SetSpace::dump() const {
  isl_space_dump(keep());
}


ISLPP_PROJECTION_ATTRS Ctx *SetSpace::getCtx() ISLPP_PROJECTION_FUNCTION{
  return Ctx::enwrap(isl_space_get_ctx(keep()));
}
#endif

//SetSpace::operator Space() const {
//  return Space::enwrap(takeCopy());
//}
