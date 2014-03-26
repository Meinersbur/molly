
#include <iostream>
#include <iomanip>
#include <cstdint>


#define LX 6
#define LY 3

#ifdef WITH_MOLLY
#include <molly.h>

//#pragma molly transform("{ [x,y] -> [node[px,py] -> local[x,y]] : px=floor(x/4) and py=floor(y/4) }")
#pragma molly transform("{ [x,y] -> [node[px] -> local[x,y]] : px=floor(x/3) }")
molly::array<double, LX, LY> source, sink;

#define MOLLY_ATTR(X) [[molly::X]]
#else
double source[LX][LY];
double sink[LX][LY];
 
#define MOLLY_ATTR(X) [[molly::X]]
#endif


typedef int64_t coord_t;

MOLLY_ATTR(pure) int getNumStencilPoints(coord_t x, coord_t y) {
  return 1 + (x - 1 >= 0 ? 1 : 0) + (x + 1 < LX ? 1 : 0) + (y - 1 >= 0 ? 1 : 0) + (y + 1 < LY ? 1 : 0);
}


MOLLY_ATTR(pure) double initVal(coord_t x, coord_t y) {
  if (x == 0 && y == 0)
    return 1;
  return 0;
}


extern "C" MOLLY_ATTR(process) void Jacobi() {
  // Init
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      source[x][y] = initVal(x, y);
    }

  // Compute
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
} // void Jacobi()


extern "C" MOLLY_ATTR(process) double reduce() {
  double sum = 0;
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      sum += sink[x][y];
    }
  return sum;
} // double Jacobi_reduce()


int main(int argc, char *argv[]) {
  Jacobi();
  auto sum = reduce();
#ifdef WITH_MOLLY
  if (__molly_isMaster())
    std::cout << "Molly result = " << sum << '\n';
#else
  std::cout << "Ref   result = " << std::setprecision(6) << sum << '\n';
#endif

  return 0;
}
