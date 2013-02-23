
#include <cstdlib>
#include <llvm/ADT/OwningPtr.h>

#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include "islpp/Map.h"
//#include <barvinok/barvinok.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;
using namespace isl;
using std::move;

void test() {
  OwningPtr<isl::Ctx> ctxown(isl::Ctx::create());
  {
    auto ctx = ctxown.get();
    //auto field = ctx->readBasicSet("[N] -> {[i] : 0 <= i and i < N}");
    //auto nodes = ctx->readBasicSet("[P] -> {[p] : 0 <= i and i < N}");

    // TODO: bugreport "[N,P] -> {[p,l] : exists (i,l : l=3*i)}" access violation

    // L = [N/p]+P-1
    // auto localFields = ctx->readBasicSet("[N,P] -> {[p,l] : exists (i,L : L=[N/P]+P-1 )}"); 
    //auto localFields = ctx->readBasicSet("[N,P] -> {[p,l] : exists (i,L : i=p*P)}"); 
   

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


    {
      auto map = Map::readFrom(ctx, "{ [1] -> [11]; [2] -> [12] }");
      auto set = Set::readFrom(ctx, "{ [1] }");
      bool empty = map.isEmpty();
      auto applied = isl::apply(move(set), move(map));
      applied.dump(); // { [11] }
    }

  }
  return;
}


int main(int argc_, const char **argv_) {
  test();
  return EXIT_SUCCESS;
}
