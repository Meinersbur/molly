
#include <cstdlib>
#include <llvm/ADT/OwningPtr.h>

#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Point.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include "islpp/Map.h"
//#include <barvinok/barvinok.h>
#include <llvm/Support/raw_os_ostream.h>

#include <isl/aff.h>
#include <isl/set.h>
#include <isl/map.h>

#include <set>
#include <unordered_set>

using namespace llvm;
using namespace isl;
using std::move;

#if 0
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
#endif

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


isl_aff *aff1[] = { aff1i, aff1j };
auto afflist1 = isl_aff_list_alloc(islctx, 2);
afflist1 = isl_aff_list_add(afflist1, aff1i);
afflist1 = isl_aff_list_add(afflist1, aff1j);
auto maff1 = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflist1));

isl_aff *aff2[] = { aff2i, aff2j };
auto afflist2 = isl_aff_list_alloc(islctx, 2);
afflist2 = isl_aff_list_add(afflist2, aff2i);
afflist2 = isl_aff_list_add(afflist2, aff2j);
auto maff2 = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflist2));

isl_aff *affi[] = { aff1i, aff2i };
auto afflisti = isl_aff_list_alloc(islctx, 2);
afflisti = isl_aff_list_add(afflisti, aff1i);
afflisti = isl_aff_list_add(afflisti, aff2i);
auto maffi = isl_multi_aff_from_aff_list(isl_space_copy(space1to2), isl_aff_list_copy(afflisti));

isl_aff *affj[] = { aff1j, aff2j };
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
      llvm::raw_fd_ostream f("set.pov", err, llvm::sys::fs::F_Text);
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

    auto one = PwAff::readFromStr(ctx, "{ [i] -> [1] }");
    auto mone = PwAff::readFromStr(ctx, "{ [i] -> [-1] }");
    auto valmone = isl::Val::intFrom(ctx, 1l); // -1 and 0 won't work
    auto div = one.mod(valmone);
 

    auto rule = PwAff::readFromStr(ctx, "{ [i] -> [floor(4i/11)] : 0 <= i and i < 11 }");
    rule.dump();
    rule.dumpExplicit(12);

    auto revRule = rule.reverse();
    revRule.dump();
    revRule.dumpExplicit(12);

    auto lexminRevRule = revRule.lexminPwMultiAff();
    lexminRevRule.dump();
    //lexminRevRule.dumpExplicit(12);

    auto lexmaxRevRule = revRule.lexmaxPwMultiAff();
    lexmaxRevRule.dump();
    //lexmaxRevRule.dumpExplicit(12);

  }


  return;
}


static void test_simplify() {
  auto ctx = Ctx::create();

  auto affi = PwMultiAff::readFromStr(ctx, 
  "{ body21_inter0[i0, i1, i2]->rank[(floor((i1) / 4)), (floor((i2) / 4))] : (exists(e0 = floor((i1) / 4), e1 = floor((i2) / 4) : i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and i2 <= 7 "
  "and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1 and 4e1 >= -3 + i2 and 4e1 <= i2 and 4e1 >= -3i0 + i2)) or(i2 = 7 and i0 <= 2 and i0 >= 0 and i1 <="
  "7 and i1 >= 1) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i0 >= 1 and i2 <= 6 and i2 >= 0 and 4e0 >= -3 + i2 and 4e0 <= i2)) or(exists(e0 = floor((i2) / 4) : i0 = 0 and "
  "i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1 and 4e0 >= -3 + i2 and 4e0 <= -1 + i2)) or(exists(e0 = floor((i1) / 4) : i2 = 7 and i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and 4e0"
  ">= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1)) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i2 <= 6 and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i2 and 4e0 <= i2 and 4e0 >"
  "= -3i0 + i2)) or(exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and i1 <= 6 and i1 >= 1 and 4e0 >= -3 + i1 and 4e0 <= -1 + i1)); body21_inter0[i0, i1, i2]->rank[(-1 + floor((i1) /"
  "4)), (0)] : i0 = 0 and i1 = 4 and i2 = 0; body21_inter0[i0, i1, i2]->rank[(floor((i1) / 4)), (floor((-1 + i2) / 4))] : exists(e0 = floor((i2) / 4) : i0 = 0 and 4e0 = i2 and i1 <= 7 and "
  "i1 >= 1 and i2 <= 6 and i2 >= 1) }"
    );
  auto ri = affi.simplify();
  assert(affi == ri);


  auto affk = PwAff::readFromStr(ctx, "{ rank[rankdim0, rankdim1] -> [(4rankdim1)] : rankdim1 = 1 and rankdim0 <= 1 and rankdim0 >= 0; rank[rankdim0, rankdim1] -> [(0)] : rankdim1 = 0 and rankdim0 <= 1 and rankdim0 >= 0 }");
  auto rk = affk.simplify();
  assert(rk == affk);

  auto afft = PwAff::readFromStr(ctx,
    "{ "
    "body21_inter0[i0, i1, i2]->[(floor((i1) / 4))] : (exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i0 >= 1 and i2 <= 6 and i2 >= 0 and 4e0 >= -3 + i2 and 4e0 <= i2)) or(exists(e0 = floor((i2) / 4) : i0 = 0 and "
    "i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1 and 4e0 >= -3 + i2 and 4e0 <= -1 + i2)) or (exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i2 <= 6 and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i2 and 4e0 <= i2 and 4e0 >"
    "= -3i0 + i2)) }");
  auto rt = afft.simplify();
  assert(afft == rt);

  auto aff1 = PwAff::readFromStr(ctx, 
    "{ body21_inter0[i0, i1, i2]->[(floor((i1) / 4))] : (exists(e0 = floor((i1) / 4), e1 = floor((i2) / 4) : i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and i2 <= 7 "
    "and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1 and 4e1 >= -3 + i2 and 4e1 <= i2 and 4e1 >= -3i0 + i2)) or(i2 = 7 and i0 <= 2 and i0 >= 0 and i1 <="
    "7 and i1 >= 1) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i0 >= 1 and i2 <= 6 and i2 >= 0 and 4e0 >= -3 + i2 and 4e0 <= i2)) or(exists(e0 = floor((i2) / 4) : i0 = 0 and "
    "i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1 and 4e0 >= -3 + i2 and 4e0 <= -1 + i2)) or (exists(e0 = floor((i1) / 4) : i2 = 7 and i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and 4e0 "
    ">= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1)) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i2 <= 6 and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i2 and 4e0 <= i2 and 4e0 >"
    "= -3i0 + i2)) or(exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and i1 <= 6 and i1 >= 1 and 4e0 >= -3 + i1 and 4e0 <= -1 + i1)); body21_inter0[i0, i1, i2]->[(-1 + floor((i1) /"
    "4))] : exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and 4e0 = i1 and i1 <= 6 and i1 >= 1); body21_inter0[i0, i1, i2]->[(floor((i1) / 4))] : exists("
    "e0 = floor((i2) / 4) : i0 = 0 and 4e0 = i2 and i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1) }");
    auto r1 = aff1.simplify();
  assert(aff1==r1);

  auto aff2 = PwAff::readFromStr(ctx, 
    "{ body21_inter0[i0, i1, i2]->[(floor((i2) / 4))] : (exists(e0 = floor((i1) / 4), e1 = floor((i2) / 4) : i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and i2 <= 7 "
    "and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1 and 4e1 >= -3 + i2 and 4e1 <= i2 and 4e1 >= -3i0 + i2)) or(i2 = 7 and i0 <= 2 and i0 >= 0 and i1 <="
    "7 and i1 >= 1) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i0 >= 1 and i2 <= 6 and i2 >= 0 and 4e0 >= -3 + i2 and 4e0 <= i2)) or(exists(e0 = floor((i2) / 4) : i0 = 0 and "
    "i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1 and 4e0 >= -3 + i2 and 4e0 <= -1 + i2)) or (exists(e0 = floor((i1) / 4) : i2 = 7 and i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and 4e0 "
    ">= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1)) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i2 <= 6 and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i2 and 4e0 <= i2 and 4e0 >"
    "= -3i0 + i2)) or(exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and i1 <= 6 and i1 >= 1 and 4e0 >= -3 + i1 and 4e0 <= -1 + i1)); body21_inter0[i0, i1, i2]->["
    "(0)] : exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and 4e0 = i1 and i1 <= 6 and i1 >= 1); body21_inter0[i0, i1, i2]->[(floor((-1 + i2) / 4))] : exists("
    "e0 = floor((i2) / 4) : i0 = 0 and 4e0 = i2 and i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1) }");
    auto r2 = aff2.simplify();
  assert(aff2 == r2);

  auto maff = PwMultiAff::readFromStr(ctx, 
    "{ body21_inter0[i0, i1, i2]->rank[(floor((i1) / 4)), (floor((i2) / 4))] : (exists(e0 = floor((i1) / 4), e1 = floor((i2) / 4) : i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and i2 <= 7 "
    "and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1 and 4e1 >= -3 + i2 and 4e1 <= i2 and 4e1 >= -3i0 + i2)) or(i2 = 7 and i0 <= 2 and i0 >= 0 and i1 <="
    "7 and i1 >= 1) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i0 >= 1 and i2 <= 6 and i2 >= 0 and 4e0 >= -3 + i2 and 4e0 <= i2)) or(exists(e0 = floor((i2) / 4) : i0 = 0 and "
    "i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1 and 4e0 >= -3 + i2 and 4e0 <= -1 + i2)) or (exists(e0 = floor((i1) / 4) : i2 = 7 and i0 <= 2 and i1 <= 7 and i1 >= 1 and i1 <= 7i0 and 4e0 "
    ">= -3 + i1 and 4e0 <= i1 and 4e0 >= -3i0 + i1)) or(exists(e0 = floor((i2) / 4) : i1 = 7 and i0 <= 2 and i2 <= 6 and i2 >= 0 and i2 <= 7i0 and 4e0 >= -3 + i2 and 4e0 <= i2 and 4e0 >"
    "= -3i0 + i2)) or(exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and i1 <= 6 and i1 >= 1 and 4e0 >= -3 + i1 and 4e0 <= -1 + i1)); body21_inter0[i0, i1, i2]->rank[(-1 + floor((i1) /"
    "4)), (0)] : exists(e0 = floor((i1) / 4) : i0 = 0 and i2 = 0 and 4e0 = i1 and i1 <= 6 and i1 >= 1); body21_inter0[i0, i1, i2]->rank[(floor((i1) / 4)), (floor((-1 + i2) / 4))] : exists("
    "e0 = floor((i2) / 4) : i0 = 0 and 4e0 = i2 and i1 <= 7 and i1 >= 1 and i2 <= 6 and i2 >= 1) }");
 auto r = maff.simplify();
 assert(r==maff);
}


int main(int argc, const char *argv[]) {
  test_simplify();

 //test();

  auto ctx = isl_ctx_alloc();
  bool r1,r2;
#if 0
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

  assert(r1 == r2 && "Help! Disjointness is different for nested and unnested spaces!!");
#endif

#if 0
  {
    auto affstr = "{ [i0, i1, i2, i3,i4, i5, i6, i7,rankdim3,i12] -> [(floor((i7)/4))] }";
    auto aff = isl_aff_read_from_str(ctx, affstr);
    auto contextstr = "{ [i0, i1, i2, i3, i4, i5, i6, i7,rankdim3,i12] :  i7 = 1 + i3 and 4rankdim3 = -3 + i3 and 4i12 = 1 + i3 }";
    auto context = isl_set_read_from_str(ctx, contextstr);
    auto gisted = isl_aff_gist(isl_aff_copy(aff), isl_set_copy(context));
    isl_aff_dump(gisted);
  }
#endif

  auto islctx = Ctx::enwrap(ctx);
#if 0
 auto pwaff = PwAff::readFromStr(islctx, "{ srcNode[i0, i1, i2, i3] ->[0] :"
    "(i0 = 0 and i1 = 0 and i2 <= 3 and i2 >= 0 and i3 <= 3 and i3 >= 0) or "
    "(i0 = 1 and i1 = 1 and i2 <= 3 and i2 >= 0 and i3 <= 3 and i3 >= 0) or "
    "(i0 = 2 and i1 = 2 and i2 <= 3 and i2 >= 0 and i3 <= 3 and i3 >= 0) or "
    "(i0 = 3 and i1 = 3 and i2 <= 3 and i2 >= 0 and i3 <= 3 and i3 >= 0) }");
 pwaff.coalesce_inplace();
 pwaff.dump();
#endif

 isl_options_set_coalesce_bounded_wrapping(islctx->keep(), 0);

 auto triage = Map::readFrom(islctx, 
//   "{ [x, y] -> [x, 1 + y] : x >= 1 and y >= 1 and x <= 3 and y <= 3;"
//   "  [x, y] -> [1 + x, y] : x >= 1 and x <= 5 and y >= 2 and y <= 4 }");
 "{ [x, y] ->[x, 1 + y] : x >= 1 and y >= 1 and"
 "					    x <= 10 and y <= 10;"
 "[x, y] ->[1 + x, y] : x >= 1 and x <= 20 and"
   "					    y >= 5 and y <= 15 }");
 triage.dump();
 triage.dumpExplicit(2*20*21);

 std::set<std::tuple<int,int,int,int>> elts;
 triage.wrap().foreachPoint([&elts](isl::Point p) -> bool {
   elts.emplace(p.getCoordinate(isl_dim_set, 0).asSi(), p.getCoordinate(isl_dim_set, 1).asSi(), p.getCoordinate(isl_dim_set, 2).asSi(), p.getCoordinate(isl_dim_set, 3).asSi());
 return false;
 });

 triage.coalesce_inplace();
 triage.dump();
 triage.dumpExplicit(2 * 20 * 21);

 std::set<std::tuple<int, int, int, int>> elts2;
 triage.wrap().foreachPoint([&elts2](isl::Point p) -> bool {
   elts2.emplace(p.getCoordinate(isl_dim_set, 0).asSi(), p.getCoordinate(isl_dim_set, 1).asSi(), p.getCoordinate(isl_dim_set, 2).asSi(), p.getCoordinate(isl_dim_set, 3).asSi());
   return false;
 });

 assert(elts==elts2);

 
 //auto set7 = Set::readFrom(islctx, "{ [x,y,z] : x=42 and 5<=y and y<=6 and 3<=z and z<=4 ; [x,y,z] : x=41 and 3<=y and y<=4  and 3<=z and z<=4 }");
 auto set7 = Set::readFrom(islctx, "{ [x,y,z] : x=41 and 141<=y<=142 and 300<=z<=301; [x,y,z] : x=42 and 143<=y<=144 and 302<=z<=303 }");
 set7.dump();
 set7.coalesce_inplace(); 
 set7.dump();


 auto set6 = Set::readFrom(islctx, "{ [i0, i1] : i1 >= 40 and i1 >= i0 and i1 <= i0 and i1 <= 41 and i1 >= i0 and i1 <= i0; [42, 42] }");
 set6.coalesce_inplace();
 set6.dump();
 set6.dumpExplicit();


 auto set5 = Set::readFrom(islctx, "{ [i0, i1]: "
   "(i0 = 40 and i1 = 40) or "
   "(i0 = 41 and i1 = 41) or "
   "(i0 = 42 and i1 = 42) }");
 set5.coalesce_inplace();
 set5.dump();
 set5.dumpExplicit();

 auto set = Set::readFrom(islctx, "{ [i0, i1]: "
   "(i0 = 39 and i1 = 39) or "
   "(i0 = 40 and i1 = 40) or "
   "(i0 = 41 and i1 = 41) or "
   "(i0 = 42 and i1 = 42) }");
 set.coalesce_inplace();
 set.dump();
 set.dumpExplicit();



  

 auto set3 = Set::readFrom(islctx, "{ [i0, i1]: "
   //   "(i0 = 0 and i1 = i0) or "
   //   "(i0 = 1 and i1 = i0) or "
   "(i0 = 41 and i1 = i0) or "
   "(i0 = 42 and i1 = i0) }");
 set3.coalesce_inplace();
 set3.dump();
 set3.dumpExplicit();


 auto set2 = Set::readFrom(islctx, "{ [i0, i1]: "
//   "(i0 = 0 and i0 = 0) or "
//   "(i0 = 1 and i0 = 1) or "
   "(i0 = 41 and i0 = 41) or "
   "(i0 = 42 and i0 = 42) }");
 set2.coalesce_inplace();
 set2.dump();
 //set2.dumpExplicit();


 auto set4 = Set::readFrom(islctx, "{ [i0, i1]: "
   "(i0 = 39 and i1 = 139) or "
   "(i0 = 40 and i1 = 140) or "
   "(i0 = 41 and i1 = 141) or "
   "(i0 = 42 and i1 = 142) }");
 set4.coalesce_inplace();
 set4.dump();
 set4.dumpExplicit();




  return EXIT_SUCCESS;
}
