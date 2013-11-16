#include <molly.h>


molly::array<bool,6,6> habitat1;
molly::array<bool,6,6> habitat2;


[[molly::pure]] static bool hasLife(bool prevHasLife, int neighbors) { MOLLY_DEBUG_FUNCTION_ARGS(prevHasLife, neighbors)
  if (prevHasLife)
    return 2 <= neighbors && neighbors <= 3;
  return neighbors == 3;
}

#if 0
extern "C" void test() { MOLLY_DEBUG_FUNCTION_SCOPE
  for (auto i = 0; i < 100; i+=1) {
    for (int x = 1, width = habitat1.length(0); x < width-1; x+=1) {
      for (int y = 1, height = habitat1.length(1); y < height-1; y+=1) {
        //auto neighbors = habitat1[x-1][y-1] + habitat1[x-1][y] + habitat1[x-1][y+1] + habitat1[x][y+1] + habitat1[x+1][y+1] + habitat1[x+1][y] + habitat1[x+1][y-1] + habitat1[x][y-1];
        auto neighbors = habitat1[x-1][y] + habitat1[x][y+1] + habitat1[x+1][y] + habitat1[x][y-1];
        habitat2[x][y] = hasLife(habitat1[x][y], neighbors);
      }
    }
     for (int x = 1, width = habitat1.length(0); x < width-1; x+=1)
       for (int y = 1, height = habitat1.length(1); y < height-1; y+=1)
         habitat1[x][y] = habitat2[x][y];
  }
}
#endif

extern "C" void test() { MOLLY_DEBUG_FUNCTION_SCOPE 
  for (auto i = 0; i < 3; i+=1) {
    for (int x = 0, width = habitat1.length(0); x < width-2; x+=1) {
      for (int y = 0, height = habitat1.length(1); y < height-2; y+=1) {

        auto n1 = *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x), (uint64_t)(y+1));

        auto n2 = *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x+1), (uint64_t)(y+2));

        auto n3 = *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x+2), (uint64_t)(y+1));

        auto n4 = *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x+1), (uint64_t)(y));

        auto n = *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x+1), (uint64_t)(y+1));

        auto r = hasLife(n, n1+n2+n3+n4);

        *(bool*)__builtin_molly_ptr(&habitat2, (uint64_t)(x+1), (uint64_t)(y+1)) = r;
      }
    }
    for (int x = 0, width = habitat1.length(0); x < width-2; x+=1) {
      for (int y = 0, height = habitat1.length(1); y < height-2; y+=1) {

        auto r = *(bool*)__builtin_molly_ptr(&habitat2, (uint64_t)(x+1), (uint64_t)(y+1));

        *(bool*)__builtin_molly_ptr(&habitat1, (uint64_t)(x+1), (uint64_t)(y+1)) = r;
      }
    }
  }
}


int main(int argc, char *argv[], char *envp[]) { MOLLY_DEBUG_FUNCTION_SCOPE

  test();

  return EXIT_SUCCESS;
}
