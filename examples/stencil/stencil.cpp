
//template<int... L>
//class [[molly::lengths(L...))]] testclass {};


#include <molly.h>

#include <iostream>

#if 0
//================ 
template<class T, class U> 
struct Foo { }; 

template<class... Args> 
void Bar(Foo<Args...> f) { } 

int main2() { Foo<int, float> f; Bar(f); }
#endif

//molly::array<int> My0DField;
//molly::array<int, 4> My1DField;
//molly::array<int, 4, 4> My2DField;
//molly::array<int, 4, 4, 4> My3DField;
//molly::array<int, 4, 4, 4, 4> My4DField;
//std::vector<int> vec;

//#pragma layout interchange(0,1)
molly::array<int, 128> MyField;
//molly::array<int, 44> MySecondField;


extern "C" void test() {
  MOLLY_DEBUG_FUNCTION_SCOPE

  //MyField.set(0,1);
  //MySecondField.set(0,2);
  //MyField.memberwithattr();

  // MyField[0] = 7;
  //My2DField[0][0] = 5;
  //My3DField[0][0][0] = 3;

  //(void)MyField.length();

  auto len = MyField.length();
  for (int i = 0; i < len; i+=1) {
    //MyField.set(i,i+1);

    //*MyField.ptr(i) = i+1;
    MyField[i] = i+2;

    int *a = &MyField[i];
    int *b = a + 1;

    //auto ptr = __builtin_molly_ptr(&MyField, i);
    //*ptr = i+2;

    //auto x = *ptr;

    //*MySecondField.ptr(i) = i+2;
    //MySecondField[i] = i+2;
  }

#if 0
  for (int i = 0; i < LowLoval; i++) {
    // MPI
  }
  for (int i = LowLocal; i < HighLocal; i++) {
    MyField[i] = MyField[i-1] + MyField[i+1];

    // MyField[i-1]
    // Local
    // MyField[i+1]
  }
  for (int i = HighLocal; i < len; i++) {
    // MPI
  }

  for (int i = 0; i < 2; i+=1) {
    MyField[i+len/2] = i+len/2+1;
  }
#endif

  auto val = MyField[len/2];

  //std::cerr << "cerr: Rank " << molly::world_self() << " got value " << val << std::endl;
  std::cout << "cout: Rank " << molly::world_self() << " got value " << val << std::endl;
}


int main(int argc, char *argv[]) {
  MOLLY_DEBUG_FUNCTION_SCOPE

    test();

  return 0;
}
