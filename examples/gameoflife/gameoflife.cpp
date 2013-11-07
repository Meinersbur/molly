#include <molly.h>

molly::array<bool,6,6> habitat1;
molly::array<bool,6,6> habitat2;


MOLLYATTR(pure) static bool hasLife(bool prevHasLife, int neighbors) { MOLLY_DEBUG_FUNCTION_SCOPE
  if (prevHasLife)
    return 2 <= neighbors && neighbors <= 3;
return neighbors == 3;
}


extern "C" void test() { MOLLY_DEBUG_FUNCTION_SCOPE
  for (int x = 1, width = habitat1.length(0); x < width-1; x+=1) {
    for (int y = 1, height = habitat1.length(1); y < height-1; y+=1) {
      auto neighbors = habitat1[x-1][y-1] + habitat1[x-1][y] + habitat1[x-1][y+1] + habitat1[x][y+1] + habitat1[x+1][y+1] + habitat1[x+1][y] + habitat1[x+1][y-1] + habitat1[x][y-1];
      habitat2[x][y] = hasLife(habitat1[x][y], neighbors);
    }
  }
}

int main(int argc, char *argv[], char *envp[]) { MOLLY_DEBUG_FUNCTION_SCOPE

  test();

  return EXIT_SUCCESS;
}
