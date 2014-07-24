
#ifdef WITH_MOLLY
#include <molly.h>
#else /* WITH_MOLLY */
#include <molly_emulation.h>
#endif /* WITH_MOLLY */

#include <bench.h>

#include <iostream>
#include <cassert>

#include <lqcd.h>





#ifndef LT
#define L  16
#define LT L
#define LX L
  
#define B 8
#define BT B
#define BX B

#define P 2
#define PT P
#define PX P
#endif

#define sBT STR(BT)
#define sBX STR(BX)

#define sPT STR(PT)
#define sPX STR(PX)

#pragma molly transform("{ [t,x,d] -> [node[pt,px] -> local[t,x,d]] : 0<=pt<" sPT" and 0<=px<" sPX" and " sBT"pt<=t<=" sBT"*(pt+1) and " sBX"px<=x<=" sBX"*(px+1) }")
molly::array<su3matrix_t, LT + 1, LX + 1, 4> gauge;
#define GAUGE_MOD(divident,divisor) divident

#pragma molly transform("{ [t,x] -> [node[floor(t/" sBT"),floor(x/" sBX")] -> local[floor(t/2),x,t%2]] }")
molly::array<spinor_t, LT, LX> source, sink;


complex_t ka[2] = {1};


extern "C" MOLLY_ATTR(process) void HoppingMatrix_noka() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1) {
          fullspinor_t result;
	  
          // T+
	  {
	    auto halfspinor = project_TUP(source[molly::mod(t + 1, LT)][x]);
	    halfspinor = gauge[GAUGE_MOD(t + 1, LT)][x][DIM_T] * halfspinor;
            result += expand_TUP(halfspinor);
	  }

          // T-
	  {
	    auto halfspinor = project_TDN(source[molly::mod(t - 1, LT)][x]);
	    halfspinor = gauge[t][x][DIM_T] * halfspinor;
            result += expand_TDN(halfspinor);
	  }
          

          // X+
	  {
	    auto halfspinor = project_XUP(source[t][molly::mod(x + 1, LX)]);
	    halfspinor = gauge[t][GAUGE_MOD(x + 1, LX)][DIM_X] * halfspinor;
            result += expand_XUP(halfspinor);
	  }
          
          // X-
	  {
	    auto halfspinor = project_XDN(source[t][molly::mod(x - 1, LX)]);
	    halfspinor = gauge[t][x][DIM_X] * halfspinor;
            result += expand_XDN(halfspinor);
	  }
	  

      // Writeback
      sink[t][x] = result;
    }
} // void HoppingMatrix_noka()


extern "C" MOLLY_ATTR(process) void HoppingMatrix_kamul() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1) {
          fullspinor_t result;
      
          // T+
      {
        auto halfspinor = project_TUP(source[molly::mod(t + 1, LT)][x]);
        halfspinor = gauge[GAUGE_MOD(t + 1, LT)][x][DIM_T] * halfspinor;
        halfspinor *= ka[0];
            result += expand_TUP(halfspinor);
      }

          // T-
      {
        auto halfspinor = project_TDN(source[molly::mod(t - 1, LT)][x]);
        halfspinor = gauge[t][x][DIM_T] * halfspinor;
        halfspinor *= conj(ka[0]);
            result += expand_TDN(halfspinor);
      }
          

          // X+
      {
        auto halfspinor = project_XUP(source[t][molly::mod(x + 1, LX)]);
        halfspinor = gauge[t][GAUGE_MOD(x + 1, LX)][DIM_X] * halfspinor;
        halfspinor *= ka[1];
            result += expand_XUP(halfspinor);
      }
          
          // X-
      {
        auto halfspinor = project_XDN(source[t][molly::mod(x - 1, LX)]);
        halfspinor = gauge[t][x][DIM_X] * halfspinor;
        halfspinor *= conj(ka[1]);
            result += expand_XDN(halfspinor);
      }
      

      // Writeback
      sink[t][x] = result;
    }
} // void HoppingMatrix_kamul()



MOLLY_ATTR(pure) spinor_t initSpinorVal(coord_t t, coord_t x) {
  if (t == 0 && x == 0)
    return spinor_t(su3vector_t::one_b(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());

  return spinor_t::zero();
}


MOLLY_ATTR(pure) su3matrix_t initGaugeVal(coord_t t, coord_t x, direction_t dir) {
  t = molly::mod(t,LT);
  x = molly::mod(x,LX);

  if (t == 0 && x == 0)
    return su3matrix_t::mone();

  return su3matrix_t::zero();
}


MOLLY_ATTR(pure) void checkSpinorVal(coord_t t, coord_t x, spinor_t val) {
}


extern "C" MOLLY_ATTR(process) void init() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      source[t][x] = initSpinorVal(t, x);


  for (coord_t t = 0; t < gauge.length(0); t += 1)
    for (coord_t x = 0; x < gauge.length(1); x += 1)
      for (coord_t d = 0; d < gauge.length(2); d += 1)
        gauge[t][x][d] = initGaugeVal(t, x, static_cast<direction_t>(2 * d));
}




extern "C" MOLLY_ATTR(process) double reduce() {
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      checkSpinorVal(t, x, source[t][x]);

  double result = 0;
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      result += norm(sink[t][x]);
  return result;
}


void bench() {
  const uint64_t nSites = LT*LX;
  const int spinorsize = 4 * 3 * 2 * 8;
  const int su3size = 3 * 3 * 2 * 8;
  std::vector<bench_exec_info_cxx_t> configs;
  const int rounds = 1;
  const uint64_t nStencilsPerCall = nSites * rounds;
  
  {
    configs.emplace_back();
    auto &benchinfo = configs.back();
    benchinfo.desc = "Dslash2d dbl noka";
    benchinfo.func = [](size_t tid, size_t nThreads) {
      assert(tid==0);
      assert(nThreads == 1);
      for (auto i = 0; i < rounds; i += 1) {
        HoppingMatrix_noka();
      }
    };
    benchinfo.nStencilsPerCall = nStencilsPerCall;
    benchinfo.nFlopsPerCall =  nStencilsPerCall * (/*operator+=*/3 * (4 * 3 * 2) + 8 * (/*project*/2 * 3 * 2 + /*su3mm*/2 * (9 * (2 + 4) + 6 * 2)));
    benchinfo.nStoredBytesPerCall = nStencilsPerCall * spinorsize;
    benchinfo.nLoadedBytesPerCall = nStencilsPerCall * (4 * spinorsize + 4 * su3size);
    benchinfo.nWorkingSet = nSites*spinorsize + (LT + 1)*(LX + 1) *su3size;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
  }
  
  {
    configs.emplace_back();
    auto &benchinfo = configs.back();
    benchinfo.desc = "Dslash2d dbl kamul";
    benchinfo.func = [](size_t tid, size_t nThreads) {
      assert(tid==0);
      assert(nThreads == 1);
      for (auto i = 0; i < rounds; i += 1) {
        HoppingMatrix_kamul();
      }
    };
    benchinfo.nStencilsPerCall = nStencilsPerCall;
    benchinfo.nFlopsPerCall =  nStencilsPerCall * (/*operator+=*/3 * (4 * 3 * 2) + 8 * (/*project*/2 * 3 * 2 +  2*/*kamul*/3*(2/*add*/ + 4/*mul*/) + /*su3mm*/2 * (9 * (2 + 4) + 6 * 2)));
    benchinfo.nStoredBytesPerCall = nStencilsPerCall * spinorsize;
    benchinfo.nLoadedBytesPerCall = nStencilsPerCall * (4 * spinorsize + 4 * su3size);
    benchinfo.nWorkingSet = nSites*spinorsize + (LT + 1)*(LX + 1) *su3size;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
  }

  bench_exec_cxx(1, configs);
}


int main(int argc, char *argv[]) {

  {
      init();
  HoppingMatrix_noka();
  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result (noka) = " << result << '\n';
  }
  
    {
        init();
  HoppingMatrix_kamul();
  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result (kamul) = " << result << '\n';
  }
  
  bench();
  
  return 0;
}
