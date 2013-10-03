
#include <cstdlib>
#include <llvm/ADT/OwningPtr.h>

#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include "islpp/Map.h"
//#include <barvinok/barvinok.h>
#include <llvm/Support/raw_os_ostream.h>

#include <isl/aff.h>
#include <isl/set.h>
#include <isl/map.h>

using namespace llvm;
using namespace isl;
using std::move;


// Missing in isl
static __isl_give isl_map* isl_map_from_multi_pw_aff(__isl_take isl_multi_pw_aff *mpwaff) {
  if (!mpwaff)
    return NULL;

  isl_space *space = isl_space_domain(isl_multi_pw_aff_get_space(mpwaff));
  isl_map *map = isl_map_universe(isl_space_from_domain(space));

  unsigned n = isl_multi_pw_aff_dim(mpwaff, isl_dim_out);
  for (int i = 0; i < n; ++i) {
    isl_pw_aff *pwaff = isl_multi_pw_aff_get_pw_aff(mpwaff, i); 
    isl_map *map_i = isl_map_from_pw_aff(pwaff);
    map = isl_map_flat_range_product(map, map_i);
  }

  isl_multi_pw_aff_free(mpwaff);
  return map;
}


void test() {
  OwningPtr<isl::Ctx> ctxown(isl::Ctx::create());
  {
    auto ctx = ctxown.get();
    //auto field = ctx->readBasicSet("[N] -> {[i] : 0 <= i and i < N}");
    //auto nodes = ctx->readBasicSet("[P] -> {[p] : 0 <= i and i < N}");

    // TODO: bugreport "[N,P] -> {[p,l] : exists (i,l : l=3*i)}" access violation

    // L = [N/p]+P-1
    //auto localFields = ctx->readBasicSet("[N,P] -> {[p,l] : exists (i,L : L=[N/P]+P-1)}"); 
    //auto localFields = ctx->readBasicSet("[N,P] -> {[p,l] : exists (i,L : i=p*P)}"); 


    {
      auto islctx = ctx->keep();

      auto space1to1 = isl_space_alloc(islctx, 0, 1, 1);
      auto space1to2 = isl_space_alloc(islctx, 0, 1, 2);
      auto space2to2 = isl_space_alloc(islctx, 0, 2, 2);  
      auto space2to1 = isl_space_alloc(islctx, 0, 2, 1);

      auto set1 = isl_set_read_from_str(islctx, "{ [k] : -10 <= k && k < 0 }");
      auto aff1i = isl_aff_read_from_str(islctx, "{[i] -> [2*i]}");
      auto aff1j = isl_aff_read_from_str(islctx, "{[j] -> [3*j]}");

      auto set2 = isl_set_read_from_str(islctx, "{ [k] : 0 < k && k <= 10 }");
      auto aff2i = isl_aff_read_from_str(islctx, "{[i] -> [i+2]}");
      auto aff2j = isl_aff_read_from_str(islctx, "{[j] -> [j+3]}");


      isl_aff *aff1[] = {aff1i, aff1j};
      auto afflist1 = isl_aff_list_alloc(islctx, 2);
      afflist1 = isl_aff_list_add(afflist1, aff1i);
      afflist1 = isl_aff_list_add(afflist1, aff1j);
      auto maff1 = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflist1)); 

      isl_aff *aff2[] = {aff2i, aff2j};
      auto afflist2 = isl_aff_list_alloc(islctx, 2);
      afflist2 = isl_aff_list_add(afflist2, aff2i);
      afflist2 = isl_aff_list_add(afflist2, aff2j);
      auto maff2 = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflist2)); 

      isl_aff *affi[] = {aff1i, aff2i};
      auto afflisti = isl_aff_list_alloc(islctx, 2);
      afflisti = isl_aff_list_add(afflisti, aff1i);
      afflisti = isl_aff_list_add(afflisti, aff2i);
      auto maffi = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflisti)); 

      isl_aff *affj[] = {aff1j, aff2j};
      auto afflistj = isl_aff_list_alloc(islctx, 2);
      afflistj = isl_aff_list_add(afflistj, aff1j);
      afflistj = isl_aff_list_add(afflistj, aff2j);
      auto maffj = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflistj)); 


      auto pwaff1i = isl_pw_aff_alloc(isl_set_copy(set1), isl_aff_copy(aff1i));
      auto pwaff1j = isl_pw_aff_alloc(isl_set_copy(set1), isl_aff_copy(aff1j));
      auto pwaff2i = isl_pw_aff_alloc(isl_set_copy(set2), isl_aff_copy(aff2i));
      auto pwaff2j = isl_pw_aff_alloc(isl_set_copy(set2), isl_aff_copy(aff2j));
      auto pwaffi = isl_pw_aff_union_max(isl_pw_aff_copy(pwaff1i), isl_pw_aff_copy(pwaff2i));
      auto pwaffj = isl_pw_aff_union_max(isl_pw_aff_copy(pwaff1j), isl_pw_aff_copy(pwaff2j));

      auto pwmaff1 = isl_pw_multi_aff_alloc(isl_set_copy(set1), isl_multi_aff_copy(maff1));
      auto pwmaff2 = isl_pw_multi_aff_alloc(isl_set_copy(set2), isl_multi_aff_copy(maff2));
      auto pwmaff = isl_pw_multi_aff_union_lexmax(isl_pw_multi_aff_copy(pwmaff1), isl_pw_multi_aff_copy(pwmaff2));
      isl_pw_multi_aff_dump(pwmaff);
      auto mapA = isl_map_from_pw_multi_aff(isl_pw_multi_aff_copy(pwmaff));
      isl_map_dump(mapA);

      auto pwafflist = isl_pw_aff_list_alloc(islctx, 2);
      pwafflist = isl_pw_aff_list_add(pwafflist, isl_pw_aff_copy(pwaffi));
      pwafflist = isl_pw_aff_list_add(pwafflist, isl_pw_aff_copy(pwaffj));
      auto mpwaff = isl_multi_pw_aff_from_pw_aff_list(isl_space_copy(space1to2), isl_pw_aff_list_copy(pwafflist));
      isl_multi_pw_aff_dump(mpwaff);
      auto mapB = isl_map_from_multi_pw_aff(isl_multi_pw_aff_copy(mpwaff));
      isl_map_dump(mapB);
    }

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


int main(int argc, const char *argv[]) {
  auto ctx = isl_ctx_alloc();
  bool r1,r2;

  {
    auto space1 = isl_space_wrap(isl_space_alloc(ctx, 1, 5, 5));
    auto space2 = isl_space_wrap(isl_space_alloc(ctx, 2, 5, 5));
    isl_space_tuple_match(space1, isl_dim_set, space2, isl_dim_set); // false

    auto set1 = isl_set_universe(space1);
    auto set2 = isl_set_universe(space2);
    r1 = isl_set_is_disjoint(set1, set2); // true
  }

  {
    auto space1 = isl_space_set_alloc(ctx, 1, 10);
    auto space2 = isl_space_set_alloc(ctx, 2, 10);
    isl_space_tuple_match(space1, isl_dim_set, space2, isl_dim_set); // true

    auto set1 = isl_set_universe(space1);
    auto set2 = isl_set_universe(space2);
    r2 = isl_set_is_disjoint(set1, set2); // false
  }

  assert(r1==r2 && "Help! Disjointness is different for nested and unnested spaces!!");

  //test();
  return EXIT_SUCCESS;
}
