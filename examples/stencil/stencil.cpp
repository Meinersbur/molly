
#include <molly.h>
 

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

molly::array<int, 128> MyField;
//molly::array<int, 44> MySecondField;


int main(int argc, char *argv[]) {
  MOLLY_DEBUG_FUNCTION_SCOPE
  //MyField.set(0,1);
  //MySecondField.set(0,2);
  //MyField.memberwithattr();

 // MyField[0] = 7;
  //My2DField[0][0] = 5;
  //My3DField[0][0][0] = 3;

  //(void)MyField.length();
  
  auto len = MyField.length();
  for (int i = 0; i < 2; i+=1) {
    //MyField.set(i,i+1);
    
    //*MyField.ptr(i) = i+1;
     MyField[i] = i+1;

    //*MySecondField.ptr(i) = i+2;
    //MySecondField[i] = i+2;
  }

  return 0;
}
