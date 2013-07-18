#include "islpp_impl_common.h"
#include "islpp/AstExpr.h"

#include "islpp/Printer.h"


void AstExpr::print(llvm::raw_ostream &out) const {
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}
