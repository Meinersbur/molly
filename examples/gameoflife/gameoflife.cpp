
#ifdef WITH_MOLLY
#include <molly.h>
#else
#include <molly_emulation.h>
#endif
#include <bench.h>


MOLLY_ATTR(pure) static bool hasLife(bool prevHasLife, int neighbors) { MOLLY_DEBUG_FUNCTION_SCOPE
  if (prevHasLife)
    return 2 <= neighbors && neighbors <= 3;
  return neighbors == 3;
}


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
molly::array<bool,LX,LY> source,sink;




MOLLY_ATTR(process) void GameOfLife() { 
    for (int i = 0; i < ITERATIONS; i+=1) {
    for (int x = 1, width = source.length(0); x < width-2; x+=1) {
      for (int y = 1, height = source.length(1); y < height-2; y+=1) {
	auto n = source[x][y];
	
	auto n1 = source[x-1][y-1];
	auto n2 = source[x][y-1];
	auto n3 = source[x+1][y-1];
	auto n4 = source[x+1][y];
	auto n5 = source[x+1][y+1];
	auto n6 = source[x][y+1];
	auto n7 = source[x-1][y+1];
	auto n8 = source[x-1][y];
	
	auto r = hasLife(n, n1+n2+n3+n4+n5+n6+n7+n8);
	
	sink[x][y] = r;
      }
    }
    
    if (i < ITERATIONS-1)
        for (int x = 1, width = source.length(0); x < width-2; x+=1) {
      for (int y = 1, height = source.length(1); y < height-2; y+=1) {
	source[x][y] = sink[x][y];
      }
	}
    }
}


void bench() {
  printArgs(argc, argv);
  
   int nTests = 10;
   int nRounds = 10;
   
  molly::exec_bench([nRounds] (int k, molly::bgq_hmflags flags) {
    for (auto i=0; i<nRounds;i+=1) {
      GameOfLife();
    }
  }, nTests, LX*LY, 1/*TODO: Make exec_bench more flexible*/);
}


int main(int argc, char *argv[], char *envp[]) { MOLLY_DEBUG_FUNCTION_SCOPE
  GameOfLife();
  
  bench();
  
  return EXIT_SUCCESS;
}
