
#include <cstdlib>
#include <llvm/ADT/OwningPtr.h>

#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include <barvinok/barvinok.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;
using namespace isl;

void test() {
  OwningPtr<isl::Ctx> ctx(isl::Ctx::create());
  {
    auto set = ctx->readBasicSet("{[i] : exists (a : i = 3a and i >= 10 and i <= 42)}");
    std::string str; 

    {
      auto printer = ctx->createPrinterToStr();
      printer.setOutputFormat(isl::Format::Isl);
      printer.print(set);
       str = printer.getString();
    }

    {
      Set gset(set);
      std::string err;
      llvm::raw_fd_ostream f("set.pov", err);
      gset.printPovray(f);
    }

    {
      auto printer = ctx->createPrinterToStr();
      printer.setOutputFormat(isl::Format::Polylib);
      printer.print(set);
      auto str = printer.getString();
    }

    {
      auto printer = ctx->createPrinterToFile("test.polylib");
      printer.setOutputFormat(isl::Format::Polylib);
      printer.print(set);
      //printer.flush();
    }

  }
  return;
}


int main(int argc_, const char **argv_) {
  test();
  return EXIT_SUCCESS;
}
