#include "islpp/MultiVal.h"

#include "islpp/Printer.h"
#include <llvm/Support/raw_ostream.h>

using namespace isl;


 void MultiVal::print(llvm::raw_ostream &out) const {
     if (isNull())
    return;

  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
 }
