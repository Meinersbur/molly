
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(X)
#endif

extern int _cart_lengths[1];
extern int _cart_local_coord[1];
extern int _rank_local;

typedef int rank_t;

template<typename T, int/*size_t*/ length1>
class Array1D {
public:
  int memberwithattr() __attribute__(( annotate("memberwithattr") )) __attribute__(( molly_lengthfunc ));

private:
  long me __attribute__(( molly_lengths(field, length1) ));
  long b __attribute__(( annotate("foo") ));
  T x;

public:
  Array1D() {}
  ~Array1D() {}
  
  int length() __attribute__((annotate("foo"))) __attribute__(( molly_lengthfunc )) { return length1; }

  T get(int i) __attribute__(( molly_getterfunc )) { return x; }
  void set(int i, const T &val) __attribute__(( molly_setterfunc )) {}
  T *ptr(int i) __attribute__(( molly_reffunc )) __attribute__(( molly_inline )) { return (T*)__builtin_molly_ptr(this, length1, i); }
  
  bool isLocal(int i) __attribute__(( molly_islocalfunc )) __attribute__(( molly_inline )) { return __builtin_molly_islocal(this, length1, i); }
  bool isRemote(int i) { return !isLocal(i); }
  rank_t getRankOf(int i) __attribute__(( molly_getrankoffunc )) __attribute__(( molly_inline )) { return __builtin_molly_rankof(this, length1, i); }

  T getLocal(int i) { assert(isLocal(i)); return x; }
  void setLocal(int i, const T &val) { assert(isLocal(i)); x = val; }
} __attribute__(( molly_lengths(clazz, length1) )) /*__attribute__(( molly_dims(1) ))*/;

 

 

template<typename T, int length1, int length2>
class Array2D {
  long c;
} __attribute__(( /*molly_lengths(dummy, length1, length2)*//*, molly_dims(2)*/ ));





static class MollyInit {
public:
  MollyInit() {
    // This will be called multiple times
    int x = _cart_lengths[0]; // To avoid that early optimizers throw it away 
    int y = _cart_local_coord[0];
  }
  ~MollyInit() {
  }
} molly_global;


