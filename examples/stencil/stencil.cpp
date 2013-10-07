#include <molly.h>
#include <iostream>

//molly::array<int> My0DField;
//molly::array<int, 4> My1DField;
//molly::array<int, 4, 4> My2DField;
//molly::array<int, 4, 4, 4> My3DField;
//molly::array<int, 4, 4, 4, 4> My4DField;
//std::vector<int> vec;

//#pragma layout interchange(0,1)

//#pragma molly transform("{ [i] -> [node, j] : node = [i/32], i = node*32 + j }", 1)
molly::array<int, 128> FieldSrc;
molly::array<int, 128> FieldDst;

//molly::array<int, 44> MySecondField;


extern "C" {
  static void fill() {
    auto len = FieldSrc.length();
    for (int i = 0; i < len; i+=1) {
      FieldSrc[i] = i+3;
    }
  }
}

#if 0
extern "C" void test() {
  MOLLY_DEBUG_FUNCTION_SCOPE

  auto len = FieldDst.length();
  for (int i = 0; i < len-1; i+=1) {
    FieldDst[i] = 2*FieldSrc[i+1];
  }

  auto val = FieldDst[len/2];

  //std::cerr << "cerr: Rank " << molly::world_self() << " got value " << val << std::endl;
  std::cout << "cout: Rank " << molly::world_self() << " got value " << val << std::endl;
}
#endif


extern "C" void test() {
  MOLLY_DEBUG_FUNCTION_SCOPE

  auto len = FieldDst.length();
  for (int i = 0; i < len-1; i+=1) {
    FieldSrc[i] = i+3;
    FieldDst[i+1] = 2*FieldSrc[i];
  }
}


#if 0
extern "C" void sink() {
  auto len = FieldSrc.length();
  for (int i = 0; i < len; i+=1) {
    auto val = FieldSrc[i];
    std::cout << "FieldSrc["<<i<<"] = " << val;
  }
}
#endif

int main(int argc, char *argv[]) {
  MOLLY_DEBUG_FUNCTION_SCOPE

  test();

  //fill();
  //test();
  //sink();

  return 0;
}
