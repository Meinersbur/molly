
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cmath>


#define LX 6
#define LY 3
#define ITERATIONS 10

//#pragma molly transform("{ [x,y] -> [node[px,py] -> local[x,y]] : px=floor(x/4) and py=floor(y/4) }")
#pragma molly transform("{ [x,y] -> [node[px] -> local[x,y]] : px=floor(x/3) }")
molly::array<double, LX, LY> data;

typedef int64_t coord_t;



MOLLY_ATTR(pure) double source(coord_t x, coord_t y) {
  if (x == 0 && y == 0)
    return 1;
  return 0;
}


MOLLY_ATTR(pure) void sink(coord_t x, coord_t y, double val) {
  std::cerr << "SINK data[" << x << "," << y << "] = "<<val<<"\n";
  double expected = 0;
  if (x == 0 && y == 0)
    expected = 1;
  if (expected != val)
    std::cerr << "ERROR: Found " << val << " at (" << x << "," << y << ") expected " << expected << "\n";
}


extern "C" MOLLY_ATTR(process) void flow() {
  // Init
  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      data[x][y] = source(x, y);
    }


  for (coord_t x = 0; x < LX; x += 1)
    for (coord_t y = 0; y < LY; y += 1) {
      sink(x, y, data[x][y]);
    }
} // void flow()




int main(int argc, char *argv[]) {
  flow();

  return 0;
}
