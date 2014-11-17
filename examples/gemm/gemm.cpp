
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <bench.h>

#ifndef LY
#define L  16
#define LY L

#define BY B

#define PY P
#endif


#define sBY STR(BY)

#define sPY STR(PY)


#define LX LY
#define LZ LY





#pragma molly transform("{ [x,y] -> [rank[floor(y/" sBY ")] -> local[x,y]] }")
molly::array<double, LX, LY> A;
#pragma molly transform("{ [y,z] -> [rank[floor(y/" sBY ")] -> local[y,z]] }")
molly::array<double, LY, LZ> B;
#pragma molly transform("{ [x,z] -> [rank[0] -> local[x,z]] }")
molly::array<double, LX, LZ> C;

typedef int64_t coord_t;

MOLLY_ATTR(pure) double initValA(coord_t x, coord_t y) {
  if (x == 0 && y == 0)
    return 1;
  return 0;
}

MOLLY_ATTR(pure) double initValB(coord_t y, coord_t z) {
  if (y == 0 && z == 0)
    return 1;
  if (y == 0 && z == 1)
    return 2;
  return 0;
}

extern "C" MOLLY_ATTR(process) void init() {
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      A[x][y] = initValA(x, y);
    }

  for (coord_t y = 0; y < LY; y += 1)
    for (coord_t z = 0; z < LZ; z += 1) {
      B[y][z] = initValB(y, z);
    }
}


extern "C" MOLLY_ATTR(process) void gemm() {
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t z = 0; z < LZ; z += 1) {
      //C[x][z] = 0;
      double sum = 0;
      for (coord_t y = 0; y < LY; y += 1) {
        //C[x][z] += A[x][y] * B[y][z];
        sum += A[x][y] * B[y][z];
      }
      C[x][z] = sum;
    }
}


#if 1
#pragma molly transform("{ [x,y] -> [rank[floor(y/" sBY ")] -> local[x,y]] }")
molly::array<double, LX, LY> A_float;
#pragma molly transform("{ [y,z] -> [rank[floor(y/" sBY ")] -> local[y,z]] }")
molly::array<double, LY, LZ> B_float;
#pragma molly transform("{ [x,z] -> [rank[0] -> local[x,z]] }")
molly::array<double, LX, LZ> C_float;

extern "C" MOLLY_ATTR(process) void gemm_float() {
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t z = 0; z < LZ; z += 1) {
      //C[x][z] = 0;
      float sum = 0;
      for (coord_t y = 0; y < LY; y += 1) {
        //C[x][z] += A[x][y] * B[y][z];
        sum += A[x][y] * B[y][z];
      }
      C[x][z] = sum;
    }
}
#else
void gemm_float() {
}
#endif




MOLLY_ATTR(pure) void checkResult(double val, coord_t x, coord_t z) {
  double expected = 0;
  if (x==0 && z==0)
    expected = 1;
  if (x==0 && z==1)
    expected = 2;

  if (val != expected) {
    if (__molly_isMaster())
      std::cerr << "ERROR C[" << x << ',' << z << "]=" << val << " (" << expected << " expected)\n";
  }
}


extern "C" MOLLY_ATTR(process) double reduce() {
  double sum = 0;
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t z = 0; z < LZ; z += 1) {
      auto val = C[x][z];
      checkResult(val, x, z);
      sum += val;
    }
  return sum;
}


static void addConfig(std::vector<bench_exec_info_cxx_t> &configs, const char *desc, bool nocom, bool sloppyprec) {
    const size_t eltsize = sloppyprec ? sizeof(float) : sizeof(double);  
    const auto rounds = 1;
    const auto nSitesA = LX*LY;
    const auto nSitesB = LY*LZ;
    const auto mulsPerCall = rounds* LX*LZ*LY;
    const auto addsPerCall = rounds* LX*LZ*(LY-1);
    
    configs.emplace_back();
    auto &benchinfo = configs.back();
    
    benchinfo.desc = desc;
    benchinfo.func = [nocom,sloppyprec](size_t tid, size_t nThreads) {
      molly_com_enabled = !nocom;
      for (auto i = 0; i < rounds; i += 1) {
        if (sloppyprec)
          gemm_float();
        else
          gemm();
      }
      molly_com_enabled = true;
    };
    benchinfo.nStencilsPerCall = mulsPerCall;
    benchinfo.nFlopsPerCall =mulsPerCall+addsPerCall;
    benchinfo.nStoredBytesPerCall = rounds*LX*LZ * eltsize;
    benchinfo.nLoadedBytesPerCall = rounds* (nSitesA + nSitesB) * eltsize;
    benchinfo.nWorkingSet = (nSitesA + nSitesB) * eltsize;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
}

 
static void bench() {  
  std::vector<bench_exec_info_cxx_t> configs;
  
  addConfig(configs, "gemm dbl", false, false);
  addConfig(configs, "gemm sgl", false, true);
 
  addConfig(configs, "nocom dbl", true, false);
  addConfig(configs, "nocom sgl", true, true);
  
  bench_exec_cxx(1, configs);
}


int main(int argc, char *argv[]) {
  printArgs(argc, argv);
  
  //waitToAttach();
  init();

  //waitToAttach();
  gemm();

  auto sum = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << sum << '\n';

  bench();
  
  return 0;
}
