#include "islpp/Val.h"

#include <llvm/Support/raw_ostream.h>
#include "islpp/Printer.h"

using namespace isl;


void Val::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


std::string Val::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}


void Val::dump() const{
  isl_val_dump(keep());
}


void Val::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}
