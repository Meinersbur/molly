
#ifdef __clang__
typedef struct { long double x, y; } __float128;
#endif

#include <type_traits>
#include <cassert>
#include <cstdlib>
#include <tuple>

#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(X)
#define CXX11ATTRIBUTE(...)
#else
#define CXX11ATTRIBUTE(...) [[__VA_ARGS__]]
#endif

extern int _cart_lengths[1];
extern int _cart_local_coord[1];
extern int _rank_local;

typedef int rank_t;

template<typename T, int/*size_t*/ length1>
class Mesh1D {
public:
  int memberwithattr() __attribute__(( annotate("memberwithattr") )) __attribute__(( molly_lengthfunc ));

private:
  long me __attribute__(( molly_lengths(field, length1) ));
  long b __attribute__(( annotate("foo") ));
  T x;

public:
  Mesh1D() {}
  ~Mesh1D() {}

  int length() __attribute__((annotate("foo"))) __attribute__(( molly_lengthfunc )) { return length1; }

  T get(int i) __attribute__(( molly_getterfunc )) { return x; }
  void set(int i, const T &val) __attribute__(( molly_setterfunc )) {}
  T *ptr(int i) __attribute__(( molly_reffunc )) __attribute__(( molly_inline )) { return (T*)__builtin_molly_ptr(this, length1, i); }

  bool isLocal(int i) __attribute__(( molly_islocalfunc )) __attribute__(( molly_inline )) { return __builtin_molly_islocal(this, length1, i); }
  bool isRemote(int i) { return !isLocal(i); }
  rank_t getRankOf(int i) __attribute__(( molly_getrankoffunc )) __attribute__(( molly_inline )) { return __builtin_molly_rankof(this, length1, i); }

  T getLocal(int i) { assert(isLocal(i)); return x; }
  void setLocal(int i, const T &val) { assert(isLocal(i)); x = val; }

  T __get_broadcast(int i) const {
    auto remoteRank = getRankOf(i);
    if (remoteRank == _rank_local) {
      // The required value is here; broadcast it to all other nodes


    } else {
      // The required value is somewhere else; wait to receive it
    }
  }
  void __set_broadcast(int i, T value) {
    auto remoteRank = getRankOf(i);
    if (remoteRank == _rank_local) {
    } else {
    }
  }
} __attribute__(( molly_lengths(clazz, length1) )) /*__attribute__(( molly_dims(1) ))*/; // class Mesh1D


//TODO: Either introduce class TorusXD

template<typename T, int length1, int length2>
class Mesh2D {
  long c;
} __attribute__(( /*molly_lengths(dummy, length1, length2)*//*, molly_dims(2)*/ )); // class Mesh2D


template<typename T, int L1, int L2, int L3>
class Mesh3D {
}; // class Mesh3D





template<typename T, int L1, int L2, int L3, int L4>
class Mesh4D {
}; // class Mesh4D





namespace molly {

  template<typename T, int... L> class array;

  template <int>
  class _inttype {
  public:
    typedef int type;
  };

  template <int...>
  class _dimlengths; // Never instantiate, used to store a sequence of integers, namely the size of several dimensions


  template <int First, int...Rest>
  class _unqueue {
  public:
    typedef _dimlengths<Rest...> rest;
    typedef _dimlengths<First> first;
    static const int value = First;
  };




  // Based on http://stackoverflow.com/a/6454211
  //template<int...> struct index_tuple{}; 
  template<size_t...> 
  class _indices {}; 

  //template<std::size_t I, typename IndexTuple, typename... Types>
  //struct make_indices_impl;
  template<std::size_t I/*counter*/, typename IndexTuple/*the index list in construction*/, typename... Types>
  struct _index_sequence;

  //template<std::size_t I, std::size_t... Indices, typename T, typename... Types>
  //struct make_indices_impl<I, index_tuple<Indices...>, T, Types...>
  //{
  //   typedef typename make_indices_impl<I + 1, index_tuple<Indices...,
  //I>, Types...>::type type;
  //};
  template<std::size_t I, std::size_t... Indices, typename T/*unqueue*/, typename... Types/*remaining in queue*/>
  struct _index_sequence<I, _indices<Indices...>, T, Types...>
  {
    typedef typename /*recursion call*/_index_sequence<I + 1, _indices<Indices..., I>, Types...>::type type;
  };

  //template<std::size_t I, std::size_t... Indices>
  //struct make_indices_impl<I, index_tuple<Indices...> >
  //{
  //   typedef index_tuple<Indices...> type;
  //};
  template<std::size_t I, std::size_t... Indices>
  struct _index_sequence<I, _indices<Indices...> > 
  {
    typedef _indices<Indices...> type; // recursion terminator
  };

  //template<typename... Types>
  //struct make_indices : make_indices_impl<0, index_tuple<>, Types...>
  //{};
  template<typename... Types>
  //struct _make_index_sequence : _index_sequence<0, _indices<>, Types...> {}; // Using inheritancle becasue there are no templated typedefs (outside of classes)
  using _make_index_sequence = _index_sequence<0, _indices<>, Types...>;
  // TODO: Also a version that converts constants parameter packs (instead of type parameter pack)


  //template<int... List>
  //int select(int i, typename _inttype<List>::type... list) {
  //  return 
  //}

  template<typename T/*Elt type*/, typename Stored/*coordinates already known*/, typename Togo/*coordinates to go*/ >
  class _array_partial_subscript; // Never instantiate, use specializations only 



  template<typename T, int... Stored, int... Togo> 
  class _array_partial_subscript<T, _dimlengths<Stored...>, _dimlengths<Togo...>> {
    static_assert(sizeof...(Togo) > 1, "Specialization for more-coordinates-to-come");

    static const int nStored = sizeof...(Stored);
    static const int nTogo = sizeof...(Togo);
    static const int nDim = nStored + nTogo;

    typedef array<T, Stored..., Togo...> fieldty;
    typedef   _array_partial_subscript<T, _dimlengths<Stored..., _unqueue<Togo...>::value>, typename _unqueue<Togo...>::rest > subty;

    fieldty *owner;
    int coords[nStored]; 
  public:
    _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords) __attribute__((molly_inline)) 
      : owner(owner), coords({coords...})   {
        assert(owner);
        //TODO: assertion that all stored are in range
    }

  private:
    template<size_t... Indices>
    subty buildSubtyHelper(_indices<Indices...>/*unused*/, int coords[sizeof...(Indices)], int appendCoord)  __attribute__((molly_inline)) {
      return subty(owner, coords[Indices]..., appendCoord);
    }

  public:
    subty operator[](int i) /*TODO: const*/  __attribute__((molly_inline)) {
      assert(0 <= i);
      assert(i < _unqueue<Togo...>::value);
      return buildSubtyHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), coords, i);
    }
  }; // class _array_partial_subscript


  template<typename T, int Togo, int... Stored> 
  class _array_partial_subscript<T, _dimlengths<Stored...>, _dimlengths<Togo>> {
    static const int nStored = sizeof...(Stored);
    static const int nTogo = 1;
    static const int nDim = nStored + nTogo;

    typedef array<T, Stored..., Togo> fieldty;

    fieldty *owner;
    int coords[nStored]; 
  public:
    _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords) __attribute__((molly_inline))
      : owner(owner), coords({coords...}) {
        assert(owner);
    }

  private:
    template<size_t... Indices>
    T &getPtrHelper(_indices<Indices...>/*unused*/, int coords[sizeof...(Indices)], int last) __attribute__((molly_inline)) {
      return *owner->ptr(coords[Indices]..., last);
    }

  public:
    T &operator[](int i)  __attribute__((molly_inline)) /*TODO: const*/{
      assert(0 <= i); // Check lower bound of coordinate
      assert(i < Togo); // Check upper bound of coordinate
      return getPtrHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), coords, i);
    }
  }; // class _array_partial_subscript



  template<int> struct AsInt {
    typedef int type;
  };

#ifndef __clang__
  template<typename... Args>
  int __builtin_molly_ptr(Args... coords) {
    return sizeof...(coords);
  }
#endif


  template<typename ... Args> 
  class _consume_parameterpack {
  public:
    typedef std::true_type type;
    static const bool value = true;
  };

  template<typename First, typename ... Args> 
  class _first_parampack {
  public:
    typedef First type;
  };

  template<bool First, typename ... Args> 
  class _condition {
  public:
    typedef std::integral_constant<bool, First> type;
    static const bool value = First;
  };


  //template<size_t i, int.. Values>
  //class _getval;

  template<size_t i, int Front, int... Values>
  class _getval {
  public:
    static const int value = _getval<i-1,Values...>::value;
  };

  template<int Front, int...Values>
  class _getval<0, Front, Values...> {
  public:
    static const int value = Front;
  };

  int _select(int i) __attribute__(( molly_inline )) ; // Forward declaration

  template<typename FirstType, typename... Types> 
  int _select(int i, FirstType first, Types... list) __attribute__(( molly_inline )) {
    assert(i < (1/*first*/+sizeof...(list)));
    if (i == 0)
      return first;
    return _select(i-1, list...); // This is no recursion, every _select has a different signature
  }

  int _select(int i) __attribute__(( molly_inline )) { // overload for compiler-tim termination, should never be called
#ifdef _MSC_VER
    __assume(false);
#else
    __builtin_unreachable();
#endif
  }

  template<typename T, int Dims>
  class CXX11ATTRIBUTE(molly::field, molly::dimensions(Dims)) field {
  } __attribute__((molly_field)) ; // class field


  /// A multi-dimensional array; the dimensions must be given at compile-time
  /// T = underlaying type (must be POD)
  /// L = sizes of dimensions (each >= 1)
  // TODO: Support sizeof...(L)==0
  template<typename T, int... L>
  class CXX11ATTRIBUTE(molly::field/*, molly::lengths(L...)*/) array: public field<T, sizeof...(L)> {
    T dummy;

  public:
    typedef T ElementType;

    array() {
      if (std::getenv("bla")==(char*)-1) {
        // Dead code, but do not optimize away so the template functions get instantiated
        T dummy;
        (void)ptr(static_cast<int>(L)...);
        (void)__get_broadcast(dummy, static_cast<int>(L)...);
        (void)__set_broadcast(dummy, static_cast<int>(L)...);
        (void)__get_master(dummy, static_cast<int>(L)...);
        (void)__set_master(dummy, static_cast<int>(L)...);
      }
    }

    int length(int d) __attribute__(( molly_inline ))/*So the loop ranges are not hidden from Molly*/ {
      assert(0 <= d && d < sizeof...(L));
      return _select(d, L...);
    }

    /// Overload for length(int) for !D
    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(L)==1), typename std::conditional<true, int, Dummy>::type >::type
      length() __attribute__(( molly_inline )) {
        return length(0);
    }


    // TODO: Only allow sizeof...(L) int parameters, at least match the number of dimensions
    //template<typename... Coords>
    // T* ptr(Coords... coords) __attribute__((molly_fieldmember)) __attribute__((molly_ptrfunc)) __attribute__((molly_inline)) {
    /// Synopsis: 
    ///   T *ptr(int, int, ...)
    /// Returns a pointer to the element with the given coordinates; Molly will track loads and stores to this memory location and insert communication code
    T *ptr(typename _inttype<L>::type... coords) __attribute__((molly_fieldmember)) __attribute__((molly_ptrfunc)) __attribute__((molly_inline)) {
      return (T*)__builtin_molly_ptr(this, (static_cast<int>(coords))...);
    }



#if 0
    template<typename... Argss>
    //class XY {
    //public:
    typename std::enable_if< _condition<(sizeof...(Argss) == sizeof...(L))>::value, T*>::type
      ptr(Argss... coords) __attribute__(( molly_memberfunc )) __attribute__(( molly_ptrfunc )) __attribute__(( molly_inline )) {
        return (T*)__builtin_molly_ptr(this, L..., (static_cast<int>(coords))...);
    }
    //};
#endif


    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(L)==1), T&>::type
      operator[](int i) __attribute__((molly_inline)) {
        assert(0 <= i);
        assert(i < _unqueue<L...>::value);
        return *ptr(i);
    }

    typedef _array_partial_subscript<T, typename _unqueue<L...>::first, typename _unqueue<L...>::rest> subty;

    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(L)>1), subty>::type
      operator[](int i) __attribute__((molly_inline)) {
        assert(0 <= i);
        assert(i < _unqueue<L...>::value);
        return subty(this, i);
    }

#if 0
    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(L)==1), typename std::conditional<true, T&, Dummy>::type >::type
      operator[](int i) {
        return dummy;
    }

    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(L)>1), typename std::conditional<true, _array_partial_subscript<T, typename d, L...>, Dummy>::type >::type
      operator[](int i) {
        return _array_partial_subscript<T, sizeof...(L)-1, L...>();
    }
#endif

    void __get_local(T &val, typename _inttype<L>::type... coords) const  __attribute__((molly_fieldmember)) __attribute__((molly_get_local)) {
    }
      void __set_local(const T &val, typename _inttype<L>::type... coords) const  __attribute__((molly_fieldmember)) __attribute__((molly_set_local)) {
    }

    //template<typename... Coords>
    //void __get_broadcast(T &val, Coords... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_get_broadcast)) 
    void __get_broadcast(T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_get_broadcast)) {
    }
    void __set_broadcast(const T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_set_broadcast)) {
    }

       void __get_master(T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_get_master)) {
    }
    void __set_master(const T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_set_master)) {
    }

  } __attribute__(( molly_lengths(clazz, L) )); // class array


  /// A multi-dimensional array, but its dimensions are not known ar compile-time
  /// T = underlaying type
  /// D = number of dimensions
  /// This type is not (yet) supported by Molly
  template<typename T, int D>
  class mesh {
    int lengths[D];
    bool wraparound[D];
    T *data;
  }; // class mesh



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

} // namespace molly
