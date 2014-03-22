
#include <molly.h>

#define L 8
#define LX L
#define LY L

#pragma molly transform("{ [x,y] -> [node[px,py] -> local[x,y]] : px=floor(x/4) and py=floor(y/4) }")
molly::array<double, LX, LY> source, sink;




typedef int64_t coord_t;

extern "C" void Jacobi() {
    for (coord_t x = 0; x < source.length(0); x += 1)
      for (coord_t y = 0; y < source.length(1); y += 1) {
        auto sum = source[x, y];

        if (x - 1 >= 0) 
          sum += source[x - 1, y];

        if (x + 1 < LX) 
          sum += source[x+1,y];
       
        if (y - 1 >= 0)
          sum += source[x , y-1];

        if (y + 1 < LY)
          sum += source[x , y+1];

        auto divisor = 1 + (x - 1 >= 0 ? 1 : 0) + (x + 1 < LX ? 1 : 0) + (y - 1 >= 0 ? 1 : 0) + (y + 1 < LY ? 1 : 0);
        auto avg = sum / divisor;

          // Writeback
          sink[t][x][y][z] = avg;
        }
} // void Jacobi()


int main(int argc, char *argv[]) {
  // TODO: Initialize source
  Jacobi();
  return 0;
}
