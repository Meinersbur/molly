#include "islpp_impl_common.h"
#include "islpp/ValList.h"

#include "islpp/Printer.h"


void isl::List<Val>::print( llvm::raw_ostream &out) const {
  if (isNull())
    return;

  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void ValList::dump() const {
  isl_val_list_dump(keep());
}
