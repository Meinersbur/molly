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
#define L  2
#define LT L
#define LX 1
#define LY 1
#define LZ 1
 
#define B 1
#define BT B
#define BX B
#define BY B
#define BZ B

#define P 2
#define PT P
#define PX 1
#define PY 1
#define PZ 1
#endif

#define sBT STR(BT)
#define sBX STR(BX)
#define sBY STR(BY)
#define sBZ STR(BZ)

#define sPT STR(PT)
#define sPX STR(PX)
#define sPY STR(PY)
#define sPZ STR(PZ)




#pragma molly transform("{ [t,x,y,z,d] -> [node[pt,px,py,pz] -> local[t,x,y,z,d]] : 0<=pt<" sPT" and 0<=px<" sPX" and 0<=py<" sPY" and 0<=pz<" sPZ" and " sBT"pt<=t<=" sBT"*(pt+1) and " sBX"px<=x<=" sBX"*(px+1) and " sBY"py<=y<=" sBY"*(py+1) and " sBZ"pz<=z<=" sBZ"*(pz+1) }")
molly::array<su3matrix_t, LT + 1, LX + 1, LY + 1, LZ + 1, 4> gauge;
#define GAUGE_MOD(divident,divisor) divident


#pragma molly transform("{ [t,x,y,z] -> [node[floor(t/" sBT"),floor(x/" sBX"),floor(y/" sBY"),floor(z/" sBZ")] -> local[floor(t/2),x,y,z,t%2]] }")
molly::array<fullspinor_t, LT, LX, LY, LZ> source, sink;




complex_t ka[4] = {1};



extern "C" MOLLY_ATTR(process) void HoppingMatrix_noka() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1) {
          fullspinor_t result;
	  
          // T+
	  {
	    auto halfspinor = project_TUP(source[molly::mod(t + 1, LT)][x][y][z]);
	    halfspinor = gauge[GAUGE_MOD(t + 1, LT)][x][y][z][DIM_T] * halfspinor;
        result = expand_TUP(halfspinor);
	  }


          // T-
	  {
	    auto halfspinor = project_TDN(source[molly::mod(t - 1, LT)][x][y][z]);
	    halfspinor = gauge[t][x][y][z][DIM_T] * halfspinor;
            result += expand_TDN(halfspinor);
	  }
          

          // X+
	  {
	    auto halfspinor = project_XUP(source[t][molly::mod(x + 1, LX)][y][z]);
	    halfspinor = gauge[t][GAUGE_MOD(x + 1, LX)][y][z][DIM_X] * halfspinor;
            result += expand_XUP(halfspinor);
	  }
          
          // X-
	  {
	    auto halfspinor = project_XDN(source[t][molly::mod(x - 1, LX)][y][z]);
	    halfspinor = gauge[t][x][y][z][DIM_X] * halfspinor;
            result += expand_XDN(halfspinor);
	  }
	  

          // Y+
	  {
	    auto halfspinor = project_YUP(source[t][x][molly::mod(y + 1, LY)][z]);
	    halfspinor = gauge[t][x][GAUGE_MOD(y + 1, LY)][z][DIM_Y] * halfspinor;
            result += expand_YUP(halfspinor);
	  }
          
          // Y-
	  {
	    auto halfspinor = project_YDN(source[t][x][molly::mod(y - 1, LY)][z]);
	    halfspinor = gauge[t][x][y][z][DIM_Y] * halfspinor;
            result += expand_YDN(halfspinor);
	  }
          
     
          // Z+
	  {
	    auto halfspinor = project_ZUP(source[t][x][y][molly::mod(z + 1, LZ)]);
	    halfspinor = gauge[t][x][y][GAUGE_MOD(z + 1, LZ)][DIM_Z] * halfspinor;
      result += expand_ZUP(halfspinor);
	  }
          
          // Z-
	  {
	    auto halfspinor = project_ZDN(source[t][x][y][molly::mod(z - 1, LZ)]);
	    halfspinor = gauge[t][x][y][z][DIM_Z] * halfspinor;
      result += expand_ZDN(halfspinor);
	  }

          // Writeback
          sink[t][x][y][z] = result;
        }
} // void HoppingMatrix()


extern "C" MOLLY_ATTR(process) void HoppingMatrix_kamul() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1) {
          fullspinor_t result;
      
          // T+
      {
        auto halfspinor = project_TUP(source[molly::mod(t + 1, LT)][x][y][z]);
        halfspinor = gauge[GAUGE_MOD(t + 1, LT)][x][y][z][DIM_T] * halfspinor;
        halfspinor *= ka[0];
            result = expand_TUP(halfspinor);
      }


          // T-
      {
        auto halfspinor = project_TDN(source[molly::mod(t - 1, LT)][x][y][z]);
        halfspinor = gauge[t][x][y][z][DIM_T] * halfspinor;
        halfspinor *= conj(ka[0]);
            result += expand_TDN(halfspinor);
      }
          

          // X+
      {
        auto halfspinor = project_XUP(source[t][molly::mod(x + 1, LX)][y][z]);
        halfspinor = gauge[t][GAUGE_MOD(x + 1, LX)][y][z][DIM_X] * halfspinor;
        halfspinor *= ka[1];
            result += expand_XUP(halfspinor);
      }
          
          // X-
      {
        auto halfspinor = project_XDN(source[t][molly::mod(x - 1, LX)][y][z]);
        halfspinor = gauge[t][x][y][z][DIM_X] * halfspinor;
        halfspinor *= conj(ka[1]);
            result += expand_XDN(halfspinor);
      }
      

          // Y+
      {
        auto halfspinor = project_YUP(source[t][x][molly::mod(y + 1, LY)][z]);
        halfspinor = gauge[t][x][GAUGE_MOD(y + 1, LY)][z][DIM_Y] * halfspinor;
        halfspinor *= ka[2];
            result += expand_YUP(halfspinor);
      }
          
          // Y-
      {
        auto halfspinor = project_YDN(source[t][x][molly::mod(y - 1, LY)][z]);
        halfspinor = gauge[t][x][y][z][DIM_Y] * halfspinor;
        halfspinor *= conj(ka[2]);
        result += expand_YDN(halfspinor);
      }
          
     
          // Z+
      {
        auto halfspinor = project_ZUP(source[t][x][y][molly::mod(z + 1, LZ)]);
        halfspinor = gauge[t][x][y][GAUGE_MOD(z + 1, LZ)][DIM_Z] * halfspinor;
        halfspinor *= ka[3];
        result += expand_ZUP(halfspinor);
      }
          
          // Z-
      {
        auto halfspinor = project_ZDN(source[t][x][y][molly::mod(z - 1, LZ)]);
        halfspinor = gauge[t][x][y][z][DIM_Z] * halfspinor;
        halfspinor *= conj(ka[3]);
        result += expand_ZDN(halfspinor);
      }



          // Writeback
          sink[t][x][y][z] = result;
        }
} // void HoppingMatrix()



MOLLY_ATTR(pure) spinor_t initSpinorVal(coord_t t, coord_t x, coord_t y, coord_t z) {
  //return spinor_t(su3vector_t(t+2, 1, t+2), su3vector_t(x+2, x+2, 1), su3vector_t(y+2, 1, y+2), su3vector_t(z+2, z+2, 1));
  
  ka[0] = 1;
  ka[1] = 1;
  ka[2] = 1;
  ka[3] = 1;
  
  if (t==0&&x==0&&y==0&&z==0)
    return spinor_t(su3vector_t::one_b(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());

  return spinor_t::zero();
}


MOLLY_ATTR(pure) su3matrix_t initGaugeVal(coord_t t, coord_t x, coord_t y, coord_t z, direction_t dir) {
  t = molly::mod(t, LT);
  x = molly::mod(x, LX);
  y = molly::mod(y, LY);
  z = molly::mod(z, LZ);

  //return su3matrix_t::allone();

  if (t == 0 && x == 0 && y == 0 && z == 0)
    return su3matrix_t::mone();

  return su3matrix_t::zero();
}


MOLLY_ATTR(pure) void checkSpinorVal(coord_t t, coord_t x, coord_t y, coord_t z, spinor_t val) {
  return;

  auto c = val[0][0];
  if (c != complex_t(t)) {
    std::cerr << "Wrong t (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[1][0];
  if (c != complex_t(x)) {
    std::cerr << "Wrong x (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[2][0];
  if (c != complex_t(y)) {
    std::cerr << "Wrong y (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[3][0];
  if (c != complex_t(z)) {
    std::cerr << "Wrong z (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
}


#if 1
extern "C" MOLLY_ATTR(process) void init() {
//extern "C" void init() {
#if 1
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1)
          source[t][x][y][z] = initSpinorVal(t, x, y, z);

  for (coord_t t = 0; t < gauge.length(0); t += 1)
    for (coord_t x = 0; x < gauge.length(1); x += 1)
      for (coord_t y = 0; y < gauge.length(2); y += 1)
        for (coord_t z = 0; z < gauge.length(3); z += 1)
          for (coord_t d = 0; d < gauge.length(4); d += 1)
            gauge[t][x][y][z][d] = initGaugeVal(t, x, y, z, static_cast<direction_t>(2 * d));
#endif
}
#endif


extern "C" MOLLY_ATTR(process) double reduce() {
//extern "C" double reduce() {
#if 0
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      for (coord_t y = 0; y < sink.length(2); y += 1)
        for (coord_t z = 0; z < sink.length(3); z += 1)
          checkSpinorVal(t, x, y, z, source[t][x][y][z]);
#endif

#if 0
  double result = 0; 
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      for (coord_t y = 0; y < sink.length(2); y += 1)
        for (coord_t z = 0; z < sink.length(3); z += 1)
          result += norm(sink[t][x][y][z]);
  return result;
#endif
  return 0;
}



#ifndef WITH_MOLLY
// Only to satisfy MollyRT
extern "C" void __molly_generated_init() {
}
extern "C" int __molly_orig_main(int argc, char *argv[]) {
  return EXIT_SUCCESS;
}
extern "C" void __molly_generated_release() {
}
#endif

 

static void addConfig(std::vector<bench_exec_info_cxx_t> &configs, const char *desc, bool kamul, bool nocom) {
  const uint64_t nSites = LT*LX*LY*LZ;
  const int spinorsize = 4 * 3 * 2 * 8;
  const int su3size = 3 * 3 * 2 * 8;
  const int rounds = 1;
  const uint64_t nStencilsPerCall = nSites * rounds;
  
      configs.emplace_back();
    auto &benchinfo = configs.back();
    benchinfo.desc = desc;
    benchinfo.func = [kamul,nocom](size_t tid, size_t nThreads) {
      molly_com_enabled = !nocom;
      assert(tid==0);
      assert(nThreads == 1);
      for (auto i = 0; i < rounds; i += 1) {
        if (kamul) {
          HoppingMatrix_kamul();
        } else {
          HoppingMatrix_noka();
        }
      }
      molly_com_enabled = true;
    };
    benchinfo.nStencilsPerCall = nStencilsPerCall;
    benchinfo.nFlopsPerCall = nStencilsPerCall * (/*operator+=*/7 * (4 * 3 * 2) + 8 * (/*project*/2 * 3 * 2 + (kamul ? 2*/*kamul*/3*(2/*add*/ + 4/*mul*/) : 0) + /*su3mm*/2 * (9 * (2 + 4) + 6 * 2)));
    benchinfo.nStoredBytesPerCall = nStencilsPerCall * spinorsize;
    benchinfo.nLoadedBytesPerCall = nStencilsPerCall * (8 * spinorsize + 8 * su3size);
    benchinfo.nWorkingSet = nSites*spinorsize + (LT + 1)*(LX + 1)*(LY + 1)*(LZ + 1) *su3size;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
}

 
void bench() {
  const uint64_t nSites = LT*LX*LY*LZ;
  const int spinorsize = 4 * 3 * 2 * 8;
  const int su3size = 3 * 3 * 2 * 8;
  std::vector<bench_exec_info_cxx_t> configs;
  const int rounds = 1;
  const uint64_t nStencilsPerCall = nSites * rounds;
  
 addConfig(configs, "dslash dbl ka", true, false);
 addConfig(configs, "dslash dbl noka", false, false);
  
  addConfig(configs, "nocom dbl ka", true, true);
  addConfig(configs, "nocom dbl noka", false, true);
  
  bench_exec_cxx(1, configs);
}


int main(int argc, char *argv[]) {

  {
  init();
  HoppingMatrix_kamul();
  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result (kamul) = " << result << '\n';
  }
  
  { 
  init();
    HoppingMatrix_noka();
  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result (noka) = " << result << '\n';
  }

  bench();
  
  return 0;
}
