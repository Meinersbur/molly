#include "islpp/AffExpr.h"

#include "islpp/Printer.h"

using namespace isl;
using namespace llvm;


void AffExpr::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(this->asAff());
  out << printer.getString();
}
