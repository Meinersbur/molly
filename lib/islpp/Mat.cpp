#include "islpp_impl_common.h"
#include "islpp/Mat.h"
#include "islpp/Printer.h"

#include <iostream>

using namespace isl;


void Mat::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void Mat::dump() const {
  if (isNull()) {
    std::cout << "NULL"; 
  return;
  }
  isl_mat_dump(keep());
}
