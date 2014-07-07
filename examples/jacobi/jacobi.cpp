
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cmath>
#include <array>
#include <bench.h>


#define ITERATIONS 3
#ifndef LX
#define L  2
#define LX 1
#define LY 1

 
#define B 1
#define BX B
#define BY B

#define P 2
#define PX 1
#define PY 1
#endif

#define sBX STR(BX)
#define sBY STR(BY)

#define sPX STR(PX)
#define sPY STR(PY)





#pragma molly transform("{ [x,y] -> [node[floor(x/" sBX "),floor(y/" sBY ")] -> local[x,y]] }")
molly::array<double, LX, LY> source, sink;

typedef int64_t coord_t;


MOLLY_ATTR(pure) double initVal(coord_t x, coord_t y) {
  if (x == 0 && y == 0)
    return 1;
  return 0;
}


MOLLY_ATTR(pure) int getNumStencilPoints(coord_t x, coord_t y) {
  return 1 + (x - 1 >= 0 ? 1 : 0) + (x + 1 < LX ? 1 : 0) + (y - 1 >= 0 ? 1 : 0) + (y + 1 < LY ? 1 : 0);
}


extern "C" MOLLY_ATTR(process) void Jacobi() {
  // Init
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      source[x][y] = initVal(x, y);
    }

  // Compute
  for (auto i = 0; i < ITERATIONS; i += 1) {
    for (coord_t x = 0; x < LX; x += 1)
      for (coord_t y = 0; y < LY; y += 1) {
        auto sum = source[x][y];

        if (x - 1 >= 0)
          sum += source[x - 1][y];

        if (x + 1 < LX)
          sum += source[x + 1][y];

        if (y - 1 >= 0)
          sum += source[x][y - 1];

        if (y + 1 < LY)
          sum += source[x][y + 1];

        auto avg = sum / (double)getNumStencilPoints(x, y);

        // Writeback
        sink[x][y] = avg;
      }

    if (i + 1 < ITERATIONS)
      for (coord_t x = 0; x < LX; x += 1)
        for (coord_t y = 0; y < LY; y += 1)
          source[x][y] = sink[x][y];
  }
} // void Jacobi()


MOLLY_ATTR(pure) void checkResult(double val, coord_t x, coord_t y) {
  if (ITERATIONS != 1)
    return;

  auto sum = initVal(x, y);
  int points = 1;

  if (x - 1 >= 0) {
    sum += initVal(x - 1, y);
    points += 1;
  }

  if (x + 1 < LX) {
    sum += initVal(x + 1, y);
    points += 1;
  }

  if (y - 1 >= 0) {
    sum += initVal(x, y - 1);
    points += 1;
  }

  if (y + 1 < LY) {
    sum += initVal(x, y + 1);
    points += 1;
  }

  auto expected = sum / points;

  if (std::abs(val - expected) >= 0.001) {
    if (__molly_isMaster())
      std::cerr << "ERROR sink[" << x << ',' << y << "]=" << val << " (" << expected << " expected)\n";
  }
}


extern "C" MOLLY_ATTR(process) double reduce() {
  double sum = 0;
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      auto val = sink[x][y];
      checkResult(val, x, y);
      sum += val;
    }
  return sum;
}


void bench() {
   int nTests = 10;
   int nRounds = 10;
   
  molly::exec_bench([nRounds] (int k, molly::bgq_hmflags flags) {
    for (auto i=0; i<nRounds;i+=1) {
      Jacobi();
    }
  }, nTests, LX*LY, 1/*TODO: Make exec_bench more flexible*/);
}


int main(int argc, char *argv[]) {
  //waitToAttach();
  Jacobi();
  auto sum = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << sum << '\n';

  bench();
  
  return 0;
}
