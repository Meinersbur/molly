#ifndef MOLLY_H
#define MOLLY_H

#include "molly_debug.h"



// TODO: Modes:
// - Single: There is just one rank
// - Broadcast: Every rank executes everything, but only the results of master are saved //TODO: for complete emulation of single-rank execution, we should also replace calls to output functions, such as printf, fwrite, etc...
// - Master: The master executes everything, only parallizable work is delegate to other ranks


#ifdef __clang__
typedef struct { long double x, y; } __float128;
#endif

#include <type_traits>
#include <cassert>
#include <cstdlib>
#include <tuple>
#include <limits>
#include <cstdint>
//#include <bits/nested_exception.h>
#ifdef __clang__
// Hack to compile without exceptions and libstdc++ 4.7 (nested_exception.h)
#define _GLIBCXX_NESTED_EXCEPTION_H 1
#endif

#if defined __clang__
//#pragma clang diagnostic ignored "-Wunknown-pragmas"
#elif defined __GNUC__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#if defined(__clang__)
#define CONCAT(X,...) X ## __VA_ARGS__
#define MOLLYATTR(...) __attribute__((CONCAT(molly_, __VA_ARGS__)))
#else
#define MOLLYATTR(...)
#endif



#if !defined(__clang__) && (defined(_MSC_VER) || __GNUC__< 4 || (__GNUC__==4 && __GNUC_MINOR__<8))
#define __attribute__(X)
#define CXX11ATTRIBUTE(...)
#else
//#define CXX11ATTRIBUTE(...) [[__VA_ARGS__]]
#define CXX11ATTRIBUTE(...)
#endif
 


#ifndef __has_feature
#define __has_feature(x) 0
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/// LLVM_OVERRIDE - Expands to 'override' if the compiler supports it.
/// Use to mark virtual methods as overriding a base class method.
#if __has_feature(cxx_override_control) \
    || (defined(_MSC_VER) && _MSC_VER >= 1700)
#define LLVM_OVERRIDE override
#else
#define LLVM_OVERRIDE
#endif



#if __has_attribute(used) || defined(__GNUC__)
#define LLVM_ATTRIBUTE_USED __attribute__((used))
#else
#define LLVM_ATTRIBUTE_USED
#endif


#pragma region Debugging
extern int _debugindention;

static inline const char *extractFilename(const char *filename) {
  const char *lastAfterSlash = filename;
  const char *pos = filename;
  while (true) {
    auto ch = *pos;
    pos+=1;

    switch(ch) {
    case '/':
      lastAfterSlash = pos;
      break;
    case '\0':
      return lastAfterSlash;
    }
  }
}



extern "C" bool __molly_isMaster();

#ifndef NDEBUG
#include <iomanip>
#include <iostream>
#include <sstream>

// Recursion end
static void dbgPrintVars_inner(const char *varnames) {}

template<typename First>
static void dbgPrintVars_inner(const char *varnames, const First &first) {
  //fprintf(stderr, " ");
  std::cerr << ' ';
  auto argsnames=varnames;

  // Skip leading space
  while (true) {
    auto ch = *argsnames;
    switch (ch) {
    case ' ':
      argsnames +=1;
      break;
    }
    break;
  }

  while (true) {
    auto ch = *argsnames;
    switch (ch) {
    case ',':
      argsnames +=1;
      break;
    case '\0':
      break;
    default:
      std::cerr<<ch;
      argsnames +=1;
      continue;
    }
    break;
  }
  //fprintf(stderr, "=");
  std::cerr << '=' << first;
}

template<typename First, typename... Args>
static void dbgPrintVars_inner(const char *varnames, const First &first, const Args&... args) {
  std::cerr << ' ';
  auto argsnames=varnames;

  // Skip leading space
  while (true) {
    auto ch = *argsnames;
    switch (ch) {
    case ' ':
      argsnames +=1;
      break;
    }
    break;
  }

  while (true) {
    auto ch = *argsnames;
    switch (ch) {
    case ',':
      argsnames +=1;
      break;
    case '\0':
      break;
    default:
      std::cerr<<ch;
      argsnames +=1;
      continue;
    }
    break;
  }
  std::cerr << '=' << first;

  dbgPrintVars_inner(argsnames, args...);
}

template<typename... Args>
static void dbgPrintVars(const char *file, int line, const char *varnames, const Args&... args) {
  if (!__molly_isMaster())
    return;

  for (int i = _debugindention; i > 0; i-=1) {
    std::cerr << "  ";
  }
  std::cerr << extractFilename(file) << ":" << std::setw(3) << std::setiosflags(std::ios::left) << line << std::resetiosflags(std::ios::left);

  dbgPrintVars_inner(varnames, args...);
  std::cerr << std::endl;
}

//TODO: improve:
// - Put 2 char tag in front to mark severity
// - Refactor part that writes prologue (severity, indention, __FILE__:__LINE__) to allow at other places
// - make configurable which rank to print
// - account for newlines
// - allow arbitrary code
// - Put into its own file
// - A version MOLLY_DEBUG_FUNCTION_SCOPE to which you can submit the arguments to; and maybe also the return value
// - redirect to a file for each rank
#define MOLLY_DEBUG(...) \
  do { \
  if (__molly_isMaster()) { \
    for (int i = _debugindention; i > 0; i-=1) { \
      std::cerr << "  "; \
    } \
    std::cerr << extractFilename(__FILE__) << ":" << std::setw(3) << std::setiosflags(std::ios::left) << __LINE__ << " " << std::resetiosflags(std::ios::left) << __VA_ARGS__ << std::endl; \
  } \
  } while (0)

#define MOLLY_VAR(...) \
    dbgPrintVars(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)

//TODO: molly attribute that allows function calls to this in SCoPs
#define MOLLY_DEBUG_FUNCTION_SCOPE DebugFunctionScope _debugfunctionscopeguard(__PRETTY_FUNCTION__, __FILE__, __LINE__);
#else
#define MOLLY_DEBUG(...) ((void)0)
#define MOLLY_VAR(...) ((void)0)
#define MOLLY_DEBUG_FUNCTION_SCOPE
#endif

#ifndef NDEBUG

class DebugFunctionScope {
  const char *funcname;
public:
  DebugFunctionScope(const char *funcname, const char *file, int line);
  ~DebugFunctionScope();
};
#endif


#pragma endregion



//#define MOLLY_COMMUNICATOR_MEMCPY

namespace molly {
class Communicator;
} // namespace molly

extern int _cart_lengths[1];
extern int _cart_local_coord[1];
extern int _rank_local;

namespace molly {
  class SendCommunicationBuffer;
 class RecvCommunicationBuffer;

typedef int rank_t;
typedef SendCommunicationBuffer combufsend_t;
typedef RecvCommunicationBuffer combufrecv_t;


void init(int &argc, char **&argv, int clusterDims, int *dimLengths, bool *dimPeriodical);
void finalize();

void broadcast_send(const void *sendbuf, size_t size);
void broadcast_recv(void *recvbuf, size_t size, rank_t sender);

int cart_dims();
int cart_self_coord(int d);
rank_t world_self();


} // namespace molly
#if 0
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

#endif



namespace molly {

  template<typename T, uint64_t... L> class array;

  template <uint64_t>
  class _inttype {
  public:
    typedef uint64_t type;
  };

  template <uint64_t...>
  class _dimlengths; // Never instantiate, used to store a sequence of integers, namely the size of several dimensions


  template <uint64_t First, uint64_t...Rest>
  class _unqueue {
  public:
    typedef _dimlengths<Rest...> rest;
    typedef _dimlengths<First> first;
    static const uint64_t value = First;
  };



#pragma region _make_index_sequence
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
#pragma endregion

  //template<int... List>
  //int select(int i, typename _inttype<List>::type... list) {
  //  return 
  //}

#pragma region _array_partial_subscript
  template<typename T/*Elt type*/, typename Stored/*coordinates already known*/, typename Togo/*coordinates to go*/ >
  class _array_partial_subscript; // Never instantiate, use specializations only 

  template<typename T, int... Stored, int... Togo> 
  class _array_partial_subscript<T, _dimlengths<Stored...>, _dimlengths<Togo...>> {
    static_assert(sizeof...(Togo) > 1, "Specialization for more-coordinates-to-come");

    static const int nStored = sizeof...(Stored);
    static const int nTogo = sizeof...(Togo);
    static const int nDim = nStored + nTogo;

    typedef array<T, Stored..., Togo...> fieldty;
    typedef _array_partial_subscript<T, _dimlengths<Stored..., _unqueue<Togo...>::value>, typename _unqueue<Togo...>::rest > subty;

    fieldty *owner;
    int coords[nStored]; 
  public:
    _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords) MOLLYATTR(inline)  
      : owner(owner), coords({coords...})   {
        assert(owner);
        //TODO: assertion that all stored are in range
    }

  private:
    template<size_t... Indices>
    MOLLYATTR(inline)
    subty buildSubtyHelper(_indices<Indices...>/*unused*/, int coords[sizeof...(Indices)], int appendCoord)   {
      return subty(owner, coords[Indices]..., appendCoord);
    }

  public:
    MOLLYATTR(inline)
    subty operator[](int i) /*TODO: const*/ {
      //assert(0 <= i);
      //assert(i < _unqueue<Togo...>::value);
      return buildSubtyHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), coords, i);
    }
  }; // class _array_partial_subscript


  template<typename T, int Togo, uint64_t... Stored> 
  class _array_partial_subscript<T, _dimlengths<Stored...>, _dimlengths<Togo>> {
    static const int nStored = sizeof...(Stored);
    static const int nTogo = 1;
    static const int nDim = nStored + nTogo;

    typedef array<T, Stored..., Togo> fieldty;

    fieldty *owner;
    int coords[nStored]; 
  public:
    MOLLYATTR(inline)
    _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords) 
      : owner(owner), coords({coords...}) {
        assert(owner);
    }

  private:
    template<size_t... Indices>
    MOLLYATTR(inline)
    T &getPtrHelper(_indices<Indices...>/*unused*/, int coords[sizeof...(Indices)], int last)  {
      return *owner->ptr(coords[Indices]..., last);
    }

  public:
    MOLLYATTR(inline)
    T &operator[](int i)  /*TODO: const*/{
      assert(0 <= i); // Check lower bound of coordinate
      assert(i < Togo); // Check upper bound of coordinate
      return getPtrHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), coords, i);
    }
  }; // class _array_partial_subscript
#pragma endregion


  template<int> struct AsInt {
    typedef int type;
  };


#pragma region Implementation of out_parampack
#ifndef NDEBUG
  template<typename... Args>
  struct out_parampack_impl;
  
 template<> 
 struct out_parampack_impl<> {
 public:
   out_parampack_impl(const char *sep) { }
   void print(std::ostream &os) const { }
 };

   template<typename Arg> 
 struct out_parampack_impl<Arg> : out_parampack_impl<>  {
   const char *sep;
   const Arg &arg;
 public:
   out_parampack_impl(const char *sep, const Arg &arg) : out_parampack_impl<>(sep), sep(sep), arg(arg) {}
    void print(std::ostream &os) const {
      os << arg;
      out_parampack_impl<>::print(os);
    }
 };

  template<typename Arg, typename... Args> 
 struct out_parampack_impl<Arg, Args...> : out_parampack_impl<Args...>  {
   const char *sep;
   const Arg &arg;
 public:
   out_parampack_impl(const char *sep, const Arg &arg, const Args&... args) : out_parampack_impl<Args...>(sep, args...), sep(sep),  arg(arg) {}
    void print(std::ostream &os) const {
      os << ", " << arg;
      out_parampack_impl<Args...>::print(os);
    }
 };

template<typename... Args>
std::ostream &operator<<(std::ostream &stream, const out_parampack_impl<Args...> &ob) {
  ob.print(stream);
  return stream;
}

template<typename... Args>
static inline out_parampack_impl<Args...> out_parampack(const char *sep, const Args&... args) {
  return out_parampack_impl<Args...>(sep, args...); //FIXME: perfect forwarding, rvalue-refs
}
#endif
#pragma endregion






extern "C" uint64_t __molly_cluster_myrank();


#pragma region Dummy builtins for other compilers
#ifndef __mollycc__
  template<typename F, typename... Args>
  void *__builtin_molly_ptr(F field, Args... coords) { MOLLY_DEBUG_FUNCTION_SCOPE
    return field->__ptr_local(coords...);
  }

#if 0
  template<typename F>
  int __builtin_molly_locallength(F field, int d) { MOLLY_DEBUG_FUNCTION_SCOPE
    return field->length(0);
  }
#endif
  template<typename F, typename... Args>
  uint64_t __builtin_molly_local_indexof(F *field, Args... coords) { MOLLY_DEBUG_FUNCTION_SCOPE
    return 0;
  }

  template<typename F, typename... Args>
  int __builtin_molly_localoffset(F, Args...) { MOLLY_DEBUG_FUNCTION_SCOPE
    return 0;
  }

    template<typename F, typename... Args>
  bool __builtin_molly_islocal(F field, Args... coords) { MOLLY_DEBUG_FUNCTION_SCOPE
    return true;
  }

     template<typename F, typename... Args>
  rank_t __builtin_molly_rankof(F, Args...) { MOLLY_DEBUG_FUNCTION_SCOPE
    return 0;
  }

  void __builtin_molly_global_init() { MOLLY_DEBUG_FUNCTION_SCOPE
  }

  void __builtin_molly_global_free() { MOLLY_DEBUG_FUNCTION_SCOPE
  }
#endif
#pragma endregion

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

#pragma region _select
  MOLLYATTR(inline)
  static inline int _select(int i); // Forward declaration

  template<typename FirstType, typename... Types>
  MOLLYATTR(inline)
  static inline int _select(int i, FirstType first, Types... list)  {
    assert(i < (int)(1/*first*/+sizeof...(list)));
    if (i == 0)
      return first;
    return _select(i-1, list...); // This is no recursion, every _select has a different signature
  }

  MOLLYATTR(inline)
  static inline int _select(int i) { // overload for compiler-tim termination, should never be called
#ifdef _MSC_VER
    __assume(false);
#else
    __builtin_unreachable();
#endif
  }
#pragma endregion


  extern int dummyint;

  template<typename T, uint64_t Dims>
  class CXX11ATTRIBUTE(molly::field, molly::dimensions(Dims)) field {
  public:
  }; // class field


#pragma region LocalStore
  class LocalStore {
  public:
    LocalStore() { MOLLY_DEBUG_FUNCTION_SCOPE }
    virtual ~LocalStore() { MOLLY_DEBUG_FUNCTION_SCOPE }

    virtual void init(uint64_t countElts) = 0;
    virtual void release() = 0;

    virtual void *getDataPtr() const = 0; //TODO: Unvirtualize for speed
    virtual size_t getElementSize() const = 0;
    virtual uint64_t getCountElements() const = 0;
  }; // class LocalStore
#pragma endregion


//#ifndef __MOLLYRT
#if 1
  /// A multi-dimensional array; the dimensions must be given at compile-time
  /// T = underlaying type (must be POD)
  /// L = sizes of dimensions (each >= 1)
  // TODO: Support sizeof...(L)==0
  template<typename T, uint64_t... L>
  class CXX11ATTRIBUTE(molly::field) array: public LocalStore, public field<T, sizeof...(L)> {


#pragma region LocalStore
  private:
    size_t localelts;
    T *localdata;

    void init(uint64_t countElts) LLVM_OVERRIDE { MOLLY_DEBUG_FUNCTION_SCOPE
      assert(!localdata && "No double-initialization");
      localdata = new T[countElts];
      localelts = countElts;
    }

    void release() LLVM_OVERRIDE { MOLLY_DEBUG_FUNCTION_SCOPE
      delete localdata;
      localdata = nullptr;
    }

    void *getDataPtr() const LLVM_OVERRIDE { MOLLY_DEBUG_FUNCTION_SCOPE
      return localdata;
    }

    size_t getElementSize() const LLVM_OVERRIDE { MOLLY_DEBUG_FUNCTION_SCOPE
      return sizeof(T);
    }

    uint64_t getCountElements() const LLVM_OVERRIDE { MOLLY_DEBUG_FUNCTION_SCOPE
      return localelts;
    }
#pragma endregion


#ifndef NDEBUG
// Allow access to assert, etc.
  public:
#else
  private:
#endif

      MOLLYATTR(inline) uint64_t coords2idx(typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(local_indexof);
      //{
      // return __builtin_molly_local_indexof(this, coords...);
      //}
#if 0
    size_t coords2idx(typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords2idx(" << out_parampack(", ", coords...) << ")");

    assert(__builtin_molly_islocal(this, coords...));
    return __builtin_molly_local_indexof(this, coords...);

#if 0
      size_t idx = 0;
      size_t lastlocallen = 0;
      size_t localelts = 1;
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, L...);
        auto locallen = __builtin_molly_locallength(this, (uint64_t)d);
        auto coord = _select(d, coords...);
        auto clustercoord = cart_self_coord(d);
        auto localcoord = coord - clustercoord*locallen;
        MOLLY_DEBUG("d="<<d << " len="<<len << " locallen="<<locallen << " coord="<<coord << " clustercoord="<<clustercoord << " localcoord="<<localcoord);
        assert(0 <= localcoord && localcoord < locallen);
        idx = idx*lastlocallen + localcoord;
        lastlocallen = locallen;
        localelts *= locallen;
      }
      MOLLY_DEBUG("this->localelts="<<this->localelts << " localelts="<<localelts);
      assert(this->localelts == localelts);
      assert(0 <= idx && idx < localelts);
      MOLLY_DEBUG("RETURN coords2idx(" << out_parampack(", ", coords...) << ") = " << idx);
      return idx;
#endif
    }
#endif


     MOLLYATTR(inline) uint64_t coords2rank(typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(field_rankof);
     //{
     //  return __builtin_molly_rankof(this, coords...);
     //}
#if 0
    /// Compute the rank which stores a specific value
    rank_t coords2rank(typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords2rank(" << out_parampack(", ", coords...) << ")");

    return __builtin_molly_rankof(this, coords...);
#if 0
      rank_t rank = 0;
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, L...);
        auto locallen = __builtin_molly_locallength(this, (uint64_t)d);
        auto coord = _select(d, coords...);
        auto clustercoord = coord / locallen;
        auto clusterlen = (len / locallen) + 1;
        assert(clustercoord < clusterlen);
        rank = (rank * clusterlen) + clustercoord;
      }
      return rank;
#endif
    }
#endif


    LLVM_ATTRIBUTE_USED bool isLocal(typename _inttype<L>::type... coords) const MOLLYATTR(islocalfunc) MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
    MOLLY_DEBUG("isLocal(" << out_parampack(", ", coords...) << ")");
   
    auto expectedRank = coords2rank(coords...);
    auto myrank = __molly_cluster_myrank();
    //world_self(); // TODO: This is the MPI rank, probably not the same as what molly thinks the rank is
    return expectedRank==myrank;

#if 0
      auto expRank = coords2rank(coords...);
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, L...);
        auto locallen = __builtin_molly_locallength(this, (uint64_t)d);
        auto coord = _select(d, coords...);
        auto clustercoord = cart_self_coord(d); //TODO: Make sure this is inlined
        //MOLLY_DEBUG("d="<<d << " len="<<len << " locallen="<<locallen << " coord="<<coord << " clustercoord="<<clustercoord);

        auto localbegin = clustercoord*locallen;
        auto localend = localbegin+locallen;
        MOLLY_VAR(d,len,locallen,coord,clustercoord,localbegin,localend);
        if (localbegin <= coord && coord < localend)
          continue;

        MOLLY_DEBUG("rtn=false coords2rank(coords...)="<<expRank<< " self="<<world_self());
        assert(expRank != world_self());
        return false;

      }
    MOLLY_DEBUG("rtn=true coords2rank(coords...)="<<expRank<< " self="<<world_self());
      assert(expRank == world_self());
      return true;
#endif
    }

  public:
    typedef T ElementType;
    static const auto Dims = sizeof...(L);


    ~array() MOLLYATTR(inline) { MOLLY_DEBUG_FUNCTION_SCOPE
      __builtin_molly_field_free(this);
      delete[] localdata;
    }


    array() MOLLYATTR(inline) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("array dimension is (" << out_parampack(", ", L...) << ")");

    //TODO: Do not call here, Molly should generate a call to __molly_field_init for every field it found
    __builtin_molly_field_init(this); // inlining is crucial since we need the original reference to the field in the first argument

#if 0
      localelts = 1;
      for (auto d = Dims-Dims; d < Dims; d+=1) {
      auto locallen = __builtin_molly_locallength(this, (uint64_t)d);
        MOLLY_DEBUG("__builtin_molly_locallength(this, "<<d<<")=" << locallen);
        localelts *= locallen;
      }
      MOLLY_DEBUG("localelts=" << localelts);
      localdata = new T[localelts];
      assert(localdata);

      if (std::getenv("bla")==(char*)-1) {
        MOLLY_DEBUG("This should never execute");
        // Dead code, but do not optimize away so the template functions get instantiated
        //TODO: Modify clang::CodeGen to generate the unconditionally
        T dummy;
        (void)ptr(static_cast<int>(L)...);
        (void)__get_local(dummy, static_cast<int>(L)...);
        (void)__set_local(dummy, static_cast<int>(L)...);
        (void)__ptr_local(static_cast<int>(L)...);
        (void)__get_broadcast(dummy, static_cast<int>(L)...);
        (void)__set_broadcast(dummy, static_cast<int>(L)...);
        (void)__get_master(dummy, static_cast<int>(L)...);
        (void)__set_master(dummy, static_cast<int>(L)...);
        (void)isLocal(L...);
      }
#endif

      if (1==0) {
        MOLLY_DEBUG("This should never execute");
        // Dead code, but do not optimize away so the template functions get instantiated
        //TODO: Modify clang::CodeGen to generate the unconditionally
        T dummy;
        (void)ptr(static_cast<int>(L)...);
        (void)__get_local(dummy, static_cast<int>(L)...);
        (void)__set_local(dummy, static_cast<int>(L)...);
        (void)__ptr_local(static_cast<int>(L)...);
        (void)__get_broadcast(dummy, static_cast<int>(L)...);
        (void)__set_broadcast(dummy, static_cast<int>(L)...);
        (void)__get_master(dummy, static_cast<int>(L)...);
        (void)__set_master(dummy, static_cast<int>(L)...);
        (void)isLocal(L...);
      }
    }


    int length(int d) const MOLLYATTR(inline)/*So the loop ranges are not hidden from Molly*/ { //MOLLY_DEBUG_FUNCTION_SCOPE
      //assert(0 <= d && d < (int)sizeof...(L));
      return _select(d, L...);
    }

    /// Overload for length(int) for !D
    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(L)==1), typename std::conditional<true, int, Dummy>::type >::type
      length() MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
        return length(0);
    }


    /// Returns a pointer to the element with the given coordinates; Molly will track loads and stores to this memory location and insert communication code
    T *ptr(typename _inttype<L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(ptrfunc) MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
      return (T*)__builtin_molly_ptr(this, coords...);
    }


    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(L)==1), T&>::type
      operator[](int i) MOLLYATTR(fieldmember) MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
        //assert(0 <= i);
        //assert(i < _unqueue<L...>::value);
        return *ptr(i);
    }

    typedef _array_partial_subscript<T, typename _unqueue<L...>::first, typename _unqueue<L...>::rest> subty;

    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(L)>1), subty>::type
      operator[](int i) MOLLYATTR(fieldmember) MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
        //assert(0 <= i);
        //assert(i < _unqueue<L...>::value);
        return subty(this, i);
    }

#pragma region Local access
    LLVM_ATTRIBUTE_USED void __get_local(T &val, typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(get_local) { MOLLY_DEBUG_FUNCTION_SCOPE
       //assert(__builtin_molly_islocal(this, coords...));
       auto idx = coords2idx(coords...);
       assert(0 <= idx && idx < localelts);
       assert(localdata);
       val = localdata[idx];
    }
    LLVM_ATTRIBUTE_USED void __set_local(const T &val, typename _inttype<L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(set_local) { MOLLY_DEBUG_FUNCTION_SCOPE
      //assert(__builtin_molly_islocal(this, coords...));
      auto idx = coords2idx(coords...);
      assert(0 <= idx && idx < localelts);
      assert(localdata);
      localdata[idx] = val;
    }
    LLVM_ATTRIBUTE_USED T *__ptr_local(typename _inttype<L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(ptr_local) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("Coords are (" << out_parampack(", ", coords...) << ")");
      //assert(__builtin_molly_islocal(this, coords...));
      auto idx = coords2idx(coords...);
      assert(0 <= idx && idx < localelts);
      assert(localdata);
      return &localdata[idx];
    }
    const T *__ptr_local(typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) {
      return const_cast<T*>(__ptr_local(coords...));
    }
#pragma endregion


   LLVM_ATTRIBUTE_USED void __get_broadcast(T &val, typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(get_broadcast);
    LLVM_ATTRIBUTE_USED void __set_broadcast(const T &val, typename _inttype<L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(set_broadcast) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords=("<<out_parampack(", ", coords...) << ") L=("<<out_parampack(", ", L...) <<")");
      if (isLocal(coords...)) {
        __set_local(val, coords...);
      } else {
        // Nonlocal value, forget it!
        // In debug mode, we could check whether it is really equal on all ranks
      }
    }

    LLVM_ATTRIBUTE_USED void __get_master(T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_get_master)) { MOLLY_DEBUG_FUNCTION_SCOPE
    }
    LLVM_ATTRIBUTE_USED void __set_master(const T &val, typename _inttype<L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_set_master)) { MOLLY_DEBUG_FUNCTION_SCOPE
    }

    private:
      uint64_t localoffset(uint64_t d) { return __builtin_molly_localoffset(this, d); }
      uint64_t locallength(uint64_t d) { return __builtin_molly_locallength(this, d); }
  } MOLLYATTR(lengths(L))/* NOTE: clang doesn't parse the whatever in [[molly::length(whatever)]] at all*/; // class array
#endif

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


  //void init();
  //void finalize();

    rank_t getMyRank();
    bool isMaster();

    int getClusterDims();
    int getClusterLength(int d);
} // namespace molly


namespace molly {
 class SendCommunicationBuffer;
 class RecvCommunicationBuffer;

} // namespace molly;
extern "C" void __molly_sendcombuf_create(molly::SendCommunicationBuffer *combuf, molly::rank_t dst, int size);
extern "C" void __molly_recvcombuf_create(molly::RecvCommunicationBuffer *combuf, molly::rank_t src, int size);
//extern "C" LLVM_ATTRIBUTE_USED void __molly_combuf_send(void *combuf, uint64_t dstRank);
//extern "C" LLVM_ATTRIBUTE_USED void __molly_combuf_recv(void *combuf, uint64_t srcRank);
extern "C" int __molly_local_coord(int i);

extern "C" LLVM_ATTRIBUTE_USED uint64_t __molly_cluster_current_coordinate(uint64_t d);


#ifndef __MOLLYRT
template<typename T, uint64_t... L>
LLVM_ATTRIBUTE_USED void molly::array<T, L...>::__get_broadcast(T &val, typename _inttype<L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(get_broadcast) { MOLLY_DEBUG_FUNCTION_SCOPE
  if (isLocal(coords...)) {
    __get_local(val, coords...);
    broadcast_send(&val, sizeof(T)); // Send to other ranks so they can return the same result
  } else {
    broadcast_recv(&val, sizeof(T), coords2rank(coords...));
  }
}
#endif

#endif /* MOLLY_H */
