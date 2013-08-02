#include "islpp_impl_common.h"
#include "islpp/AstNode.h"

#include "islpp/Printer.h"


void AstNode::print(llvm::raw_ostream &out) const {
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}
