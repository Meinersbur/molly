#include "islpp/printer.h"

#include <llvm/Support/raw_ostream.h>

#include <isl/printer.h>

using namespace isl;
using namespace llvm;
using namespace std;


std::string Printer::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}


void Printer::dump() const { 
  print(llvm::errs());
}
