
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif

#include <iostream>
#include <iomanip>
#include <cstdint>


#define LX 3
#define LY 6
#define LZ 3

#pragma molly transform("{ [x,y] -> [node[py] -> local[x,y]] : py=floor(y/3) }")
molly::array<double, LX, LY> A;
#pragma molly transform("{ [y,z] -> [node[py] -> local[y,z]] : py=floor(y/3) }")
molly::array<double, LY, LZ> B;
#pragma molly transform("{ [x,z] -> [node[0] -> local[x,z]] }")
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


MOLLY_ATTR(pure) void checkResult(double val, coord_t x, coord_t z) {
  double expected = 0;
  if (x==0 && (z==0 || z==1))
    expected = 1;

  if (val != expected) {
    if (__molly_isMaster())
      std::cerr << "ERROR C[" << x << ','<< z << "]=" << val << " (" << expected << " expected)\n";
  }
}


extern "C" MOLLY_ATTR(process) double reduce() {
  double sum = 0;
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t z = 0; z < LZ; z += 1) {
      auto val = C[x][z];
      checkResult(val,x,z);
      sum += val;
    }
  return sum;
}



int main(int argc, char *argv[]) {
  //waitToAttach();
  init();

  //waitToAttach();
  gemm();

  auto sum = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << sum << '\n';

  return 0;
}