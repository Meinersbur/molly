
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cmath>


#if !defined(LX)
#define LX 8
#define LY 8

#define BX 4
#define BY 8

#define PX 2
#define PY 1
#endif

#define _STR(X) #X
#define STR(X) _STR(X)

#define sBX STR(BX)
#define sBY STR(BY)

#define sPX STR(PX)
#define sPY STR(PY)



#pragma molly transform("{ [x,y] -> [node[floor(x/" sBX"),floor(y/" sBY")] -> local[x,y]] }")
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
