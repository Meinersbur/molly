#ifndef MOLLY_H
#define MOLLY_H

#include "molly_debug.h"
#ifdef BGQ
//#include "mypapi.h"
#endif

#include <functional>

#define PRINTRANK -1


// TODO: Modes:
// - Single: There is just one rank
// - Broadcast: Every rank executes everything, but only the results of master are saved //TODO: for complete emulation of single-rank execution, we should also replace calls to output functions, such as printf, fwrite, etc...
// - Master: The master executes everything, only parallelizable work is delegate to other ranks


#ifdef __clang__
//typedef struct { long double x, y; } __float128;
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

#if defined(__clang__)
//#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wgnu-array-member-paren-init"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif

#if defined(__clang__)
#define CONCAT(X,...) X ## __VA_ARGS__
#define MOLLYATTR(...) __attribute__((CONCAT(molly_, __VA_ARGS__)))
#else
#define MOLLYATTR(...)
#endif

// TODO: Rename to make clear what the difference to MOLLYATTR is
#define MOLLY_ATTR(X) [[molly::X]]


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


#define _STR(X) #X
#define STR(X) _STR(X)



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
extern "C" int64_t __molly_cluster_myrank();
extern "C" int __molly_cluster_mympirank();
extern "C" int __molly_cluster_rank_count();

#ifndef NTRACE
#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeinfo>

// Recursion end
static void dbgPrintVars_inner(const char *varnames) {}

template<typename First>
static void dbgPrintVars_inner(const char *varnames, const First &first) {   if (PRINTRANK >= 0 && __molly_cluster_mympirank() != PRINTRANK)  return;
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
static void dbgPrintVars_inner(const char *varnames, const First &first, const Args&... args) {  if (PRINTRANK >= 0 && __molly_cluster_mympirank() != PRINTRANK)  return;
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
  if (PRINTRANK >= 0 && __molly_cluster_mympirank() != PRINTRANK)
    return;
  std::cerr << __molly_cluster_mympirank() << ")";

  for (int i = _debugindention; i > 0; i-=1) {
    std::cerr << ' ' << ' ';
  }
  std::cerr << extractFilename(file) << ":" << std::setw(3) << std::setiosflags(std::ios::left) << line << std::resetiosflags(std::ios::left);

  dbgPrintVars_inner(varnames, args...);
  std::cerr << std::endl;
}




#pragma region Implementation of out_parampack
#ifndef NTRACE
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
   //const char *sep;
   const Arg &arg;
 public:
   out_parampack_impl(const char *sep, const Arg &arg)
     : out_parampack_impl<>(sep), arg(arg) {}

    void print(std::ostream &os) const {
      os << arg;
      //out_parampack_impl<>::print(os);
    }
 };

template<typename Arg, typename... Args>
struct out_parampack_impl<Arg, Args...> : out_parampack_impl<Args...>  {
   const char *sep;
   const Arg &arg;
 public:
   out_parampack_impl(const char *sep, const Arg &arg, const Args&... args)
     : out_parampack_impl<Args...>(sep, args...), sep(sep),  arg(arg) {}

    void print(std::ostream &os) const {
      os << arg;
      os << sep;
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





//TODO: improve:
// - Put 2 char tag in front to mark severity
// - Refactor part that writes prologue (severity, indention, __FILE__:__LINE__) to allow at other places
// - make configurable which rank to print
// - account for newlines
// - allow arbitrary code
// - Put into its own file
// - A version MOLLY_DEBUG_FUNCTION_SCOPE to which you can submit the arguments to; and maybe also the return value
// - redirect to a file for each rank

#if 0
#define MOLLY_DEBUG(...) \
  do { \
  if (__molly_isMaster()) { \
    for (int i = _debugindention; i > 0; i-=1) { \
      std::cerr << ' ' << ' '; \
    } \
    std::cerr << extractFilename(__FILE__) << ":" << std::setw(3) << std::setiosflags(std::ios::left) << __LINE__ << " " << std::resetiosflags(std::ios::left) << __VA_ARGS__ << std::endl; \
  } \
  } while (0)
#else
#define MOLLY_DEBUG(...)                             \
  do {                                               \
    if (PRINTRANK>=0 && __molly_cluster_mympirank()!=PRINTRANK)      \
      break;                                         \
    std::cerr << __molly_cluster_mympirank() << ")"; \
    for (int i = _debugindention; i > 0; i-=1) {     \
      std::cerr << ' ' << ' ';                       \
    }                                                \
    std::cerr << extractFilename(__FILE__) << ":" << std::setw(3) << std::setiosflags(std::ios::left) << __LINE__ << " " << std::resetiosflags(std::ios::left) << __VA_ARGS__ << std::endl; \
  } while (0)
#endif
  
#define MOLLY_VAR(...) \
    dbgPrintVars(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)





#if defined(__MOLLY_FUNCNAME__) || defined(__GNUC__)
#define __MOLLY_FUNCNAME__ __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__) || defined(_MSC_VER)
#define __MOLLY_FUNCNAME__ __FUNCSIG__
#elif defined(__FUNCTION__)
#define __MOLLY_FUNCNAME__ __FUNCTION__
#elif defined(__func__)
#define __MOLLY_FUNCNAME__ __func__
#else
#define __MOLLY_FUNCNAME__ ""
#endif


//TODO: molly attribute that allows function calls to this in SCoPs
#define MOLLY_DEBUG_FUNCTION_SCOPE DebugFunctionScope _debugfunctionscopeguard(__MOLLY_FUNCNAME__, __FILE__, __LINE__);
#define MOLLY_DEBUG_FUNCTION_ARGS(...) \
  DebugFunctionScope _debugfunctionscopeguard(__MOLLY_FUNCNAME__, __FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__);
#define MOLLY_DEBUG_METHOD_ARGS(...) \
  DebugFunctionScope _debugfunctionscopeguard(this, __MOLLY_FUNCNAME__, __FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__);
#else
#define MOLLY_DEBUG(...) ((void)0)
#define MOLLY_VAR(...) ((void)0)
#define MOLLY_DEBUG_FUNCTION_SCOPE
#define MOLLY_DEBUG_FUNCTION_ARGS(...)
#define MOLLY_DEBUG_METHOD_ARGS(...)
#endif

#ifndef NTRACE



static std::string extractFuncname(const char *prettyfunc) {
  std::string prettyFunction = prettyfunc;
  auto lparen = prettyFunction.find("(");
  if (lparen == std::string::npos)
    return prettyfunc;

  auto space = prettyFunction.substr(0, lparen).rfind(" ");
  auto firstColon = prettyFunction.substr(0, lparen).rfind("::");
  size_t secondColon = std::string::npos;
  if (firstColon != std::string::npos) {
    secondColon = prettyFunction.substr(0, firstColon).rfind("::");
  }

  size_t start = 0;
  if (space != std::string::npos)
    start = space + 1;
  if (secondColon != std::string::npos && secondColon >= start)
    start = secondColon + 2;

  return prettyFunction.substr(start, lparen - start);
}



#pragma region Implementation of out_printargs
#ifndef NTRACE
template<typename... Args>
struct out_printargs_impl;

template<>
struct out_printargs_impl<> {
protected:
  const char *varnames;
public:
  out_printargs_impl(const char *varnames) : varnames(varnames) {}

  void print(std::ostream &os) const { }
  void print(std::ostream &os, const char *remainingnames) const { }
};

template<typename Arg>
struct out_printargs_impl<Arg> : protected out_printargs_impl<> {
private:
  const Arg &arg;
public:
  out_printargs_impl(const char *varnames, const Arg &arg)
    : out_printargs_impl<>(varnames), arg(arg) {}

  void print(std::ostream &os) const {
    print(os, this->varnames);
  }

  void print(std::ostream &os, const char *remainingnames) const {
    auto lastvar = remainingnames;
    while (true) {
      if (lastvar[0] == '\0')
        break;
      if (lastvar[0] == ' ' || lastvar[0] == ',') {
        lastvar+=1;
        continue;
      }

      break;
    }
    os << lastvar << "=" << arg;
  }
};

template<typename Arg, typename... Args>
struct out_printargs_impl<Arg, Args...> : protected out_printargs_impl<Args...>{
  //const char *varnames;
private:
  const Arg &arg;
public:
  out_printargs_impl(const char *varnames, const Arg &arg, const Args&... args)
    : out_printargs_impl<Args...>(varnames, args...), arg(arg) {}

  void print(std::ostream &os) const {
    print(os, this->varnames);
  }

  void print(std::ostream &os, const char *remainingnames) const {
    auto varname = remainingnames;
    while (true) {
      if (varname[0] == '\0')
        break;
      if (varname[0] == ' ' || varname[0] == ',') {
        varname+=1;
        continue;
      }
      break;
    }

    auto endvarname = varname;
    while (true) {
      if (endvarname[0] == '\0')
        break;
      if (endvarname[0] == ' ' || endvarname[0] == ',')
        break;
      endvarname += 1;
    }

    os << std::string(varname, endvarname) << "=" << arg << ", ";
    out_printargs_impl<Args...>::print(os, endvarname);
  }
};

template<typename... Args>
std::ostream &operator<<(std::ostream &stream, const out_printargs_impl<Args...> &ob) {
  ob.print(stream);
  return stream;
}

template<typename... Args>
static inline out_printargs_impl<Args...> out_printargs(const char *varnames, const Args&... args) {
  return out_printargs_impl<Args...>(varnames, args...); //FIXME: perfect forwarding, rvalue-refs
}
#endif
#pragma endregion



class DebugFunctionScope {
  const char *funcname;
public:
  DebugFunctionScope(const char *funcname, const char *file, int line);

  template<typename... T>
  DebugFunctionScope(const char *funcname, const char *file, int line, const char *varnames, const T&... vars)
    : funcname(funcname) {
    //std::cerr << "varnames=" <<varnames << "\n";
    auto myrank = __molly_cluster_mympirank();
    if (PRINTRANK >= 0 && myrank != PRINTRANK)
      return;
    std::cerr << myrank << ")";
    for (int i = _debugindention; i > 0; i -= 1) {
      std::cerr << "  ";
    }
    std::cerr << "ENTER " << extractFuncname(funcname) << '(' << out_printargs(varnames, vars...) << ')';
    std::cerr << " (" << extractFilename(file) << ':' << line << ')' << std::endl;
    _debugindention += 1;
  }

  template<typename... T>
  DebugFunctionScope(void *self, const char *funcname, const char *file, int line, const char *varnames, const T&... vars)
    : funcname(funcname) {
    auto myrank = __molly_cluster_mympirank();
    if (PRINTRANK >= 0 && myrank != PRINTRANK)
      return;
    std::cerr << myrank << ")";
    for (int i = _debugindention; i > 0; i -= 1) {
      //fprintf(stderr,"  ");
      std::cerr << "  ";
    }
    std::cerr << "ENTER " << self << "->" << extractFuncname(funcname) << '(' << out_printargs(varnames, vars...) << ')';
    std::cerr << " (" << extractFilename(file) << ':' << line << ')' << std::endl;
    _debugindention += 1;
  }

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



namespace molly {

  template<typename T, int... __L> class array;

  template <int64_t>
  class _inttype {
  public:
    typedef int64_t type;
  };

  template <int64_t...>
  class _dimlengths; // Never instantiate, used to store a sequence of integers, namely the size of several dimensions


  template <int64_t First, int64_t...Rest>
  class _unqueue {
  public:
    typedef _dimlengths<Rest...> rest;
    typedef _dimlengths<First> first;
    static const int64_t value = First;
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
  //struct _make_index_sequence : _index_sequence<0, _indices<>, Types...> {}; // Using inheritance because there are no templated typedefs (outside of classes)
  using _make_index_sequence = _index_sequence<0, _indices<>, Types...>;
  // TODO: Also a version that converts constants parameter packs (instead of type parameter pack)
#pragma endregion


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
    int64_t coords[nStored];
  public:
    MOLLYATTR(inline) _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords)  
      : owner(owner), coords({coords...}) {
        //assert(owner);
        //TODO: assertion that all stored are in range
    }

  private:
    template<size_t... Indices>
    MOLLYATTR(inline) subty buildSubtyHelper(_indices<Indices...>/*unused*/, int64_t coords[sizeof...(Indices)], int64_t appendCoord)   {
      return subty(owner, coords[Indices]..., appendCoord);
    }

  public:
    MOLLYATTR(inline) subty operator[](int64_t i) /*TODO: const*/ {
      //assert(0 <= i);
      //assert(i < _unqueue<Togo...>::value);
      return buildSubtyHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), coords, i);
    }
  }; // class _array_partial_subscript


  template<typename T, int Togo, int64_t... Stored>
  class _array_partial_subscript<T, _dimlengths<Stored...>, _dimlengths<Togo>> {
    static const int nStored = sizeof...(Stored);
    static const int nTogo = 1;
    static const int nDim = nStored + nTogo;

    typedef array<T, Stored..., Togo> fieldty;

    fieldty *owner;
    int64_t coords[nStored];
  public:
    MOLLYATTR(inline) _array_partial_subscript(fieldty *owner, typename _inttype<Stored>::type... coords) 
      : owner(owner), coords({coords...}) {
        //uint64_t tmp[] = {coords...};
        //std::copy(&tmp[0], &tmp[sizeof...(coords)], this->coords);
        //assert(owner);
    }

  private:
    template<size_t... Indices>
    MOLLYATTR(inline) T &getPtrHelper(_indices<Indices...>/*unused*/, int64_t coords[sizeof...(Indices)], int64_t last)  {
      return *owner->ptr(coords[Indices]..., last);
    }

  public:
    MOLLYATTR(inline) T &operator[](int64_t i)  /*TODO: const*/{
      //assert(0 <= i); // Check lower bound of coordinate
      //assert(i < Togo); // Check upper bound of coordinate
      return getPtrHelper(typename _make_index_sequence<typename _inttype<Stored>::type...>::type(), this->coords, i);
      //return *owner->ptr(coords[_indices<Stored>]..., i);
    }
  }; // class _array_partial_subscript
#pragma endregion


  template<int> struct AsInt {
    typedef int type;
  };










#pragma region Dummy builtins for other compilers
#if !defined(__mollycc__) && !defined(__MOLLYRT)
  template<typename F, typename... Args>
  void *__builtin_molly_ptr(F field, Args... coords) { MOLLY_DEBUG_FUNCTION_SCOPE
    return NULL;
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

  static void __builtin_molly_global_init() { MOLLY_DEBUG_FUNCTION_SCOPE
  }

  static void __builtin_molly_global_free() { MOLLY_DEBUG_FUNCTION_SCOPE
  }

  static int64_t __builtin_molly_mod(int64_t divident, int64_t divisor) {
    MOLLY_DEBUG_FUNCTION_SCOPE
     assert(divisor > 0);
     auto result = (divident + divident%divisor)%divisor; // always positive
     auto quotient = (divident - result) / divisor;
     assert(result >= 0);
     assert(result < abs(divisor));
     assert(divisor*quotient + result == divident);
     return result;
  }
#endif // !defined(__mollycc__) && !defined(__MOLLYRT) 
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
    //assert(i < (int)(1/*first*/+sizeof...(list))); // Interferes with natural loop detection
    if (i == 0)
      return first;
    return _select(i-1, list...); // This is no recursion, every _select has a different signature
  }

  MOLLYATTR(inline)
  static inline int _select(int i) { // overload for compiler-time termination, should never be called
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
  /// __L = sizes of dimensions (each >= 1)
  // TODO: Support sizeof...(__L)==0
  template<typename T, int... __L>
  class CXX11ATTRIBUTE(molly::field) array: public LocalStore, public field<T, sizeof...(__L)> {


#pragma region LocalStore
  private:
    size_t localelts;
    T *localdata;

    void init(uint64_t countElts) LLVM_OVERRIDE{ MOLLY_DEBUG_METHOD_ARGS(countElts, sizeof(T))
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

      MOLLYATTR(inline) uint64_t coords2idx(typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(local_indexof);
      //{
      // return __builtin_molly_local_indexof(this, coords...);
      //}
#if 0
    size_t coords2idx(typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords2idx(" << out_parampack(", ", coords...) << ")");

    assert(__builtin_molly_islocal(this, coords...));
    return __builtin_molly_local_indexof(this, coords...);

#if 0
      size_t idx = 0;
      size_t lastlocallen = 0;
      size_t localelts = 1;
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, __L...);
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


     MOLLYATTR(inline) uint64_t coords2rank(typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(field_rankof);
     //{
     //  return __builtin_molly_rankof(this, coords...);
     //}
#if 0
    /// Compute the rank which stores a specific value
    rank_t coords2rank(typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords2rank(" << out_parampack(", ", coords...) << ")");

    return __builtin_molly_rankof(this, coords...);
#if 0
      rank_t rank = 0;
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, __L...);
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


    LLVM_ATTRIBUTE_USED bool isLocal(typename _inttype<__L>::type... coords) const MOLLYATTR(islocalfunc) MOLLYATTR(fieldmember) { MOLLY_DEBUG_FUNCTION_SCOPE
    MOLLY_DEBUG("isLocal(" << out_parampack(", ", coords...) << ")");
   
    auto expectedRank = coords2rank(coords...);
    auto myrank = __molly_cluster_myrank();
    //world_self(); // TODO: This is the MPI rank, probably not the same as what molly thinks the rank is
    return expectedRank==myrank;

#if 0
      auto expRank = coords2rank(coords...);
      for (auto d = Dims-Dims; d<Dims; d+=1) {
        auto len = _select(d, __L...);
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
    static const auto Dims = sizeof...(__L);


    ~array() MOLLYATTR(inline) { MOLLY_DEBUG_FUNCTION_SCOPE
      __builtin_molly_field_free(this);
      delete[] localdata;
    }


    array() MOLLYATTR(inline) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("array dimension is (" << out_parampack(", ", __L...) << ")");

    //TODO: Do not call here, Molly should generate a call to __molly_field_init for every field it found
    //__builtin_molly_field_init(this); // inlining is crucial since we need the original reference to the field in the first argument
    //EDIT: Now inserted by compiler magic
    //FIXME: Relooking at the source, only to those that have a #pragma transform????

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
        (void)ptr(static_cast<int>(__L)...);
        (void)__get_local(dummy, static_cast<int>(__L)...);
        (void)__set_local(dummy, static_cast<int>(__L)...);
        (void)__ptr_local(static_cast<int>(__L)...);
        (void)__get_broadcast(dummy, static_cast<int>(__L)...);
        (void)__set_broadcast(dummy, static_cast<int>(__L)...);
        (void)__get_master(dummy, static_cast<int>(__L)...);
        (void)__set_master(dummy, static_cast<int>(__L)...);
        (void)isLocal(__L...);
      }
#endif

      if (1==0) {
        MOLLY_DEBUG("This should never execute");
        // Dead code, but do not optimize away so the template functions get instantiated
        //TODO: Modify clang::CodeGen to generate the unconditionally
        T dummy;
        (void)ptr(static_cast<int>(__L)...);
        (void)__get_local(dummy, static_cast<int>(__L)...);
        (void)__set_local(dummy, static_cast<int>(__L)...);
        (void)__ptr_local(static_cast<int>(__L)...);
        (void)__get_broadcast(dummy, static_cast<int>(__L)...);
        (void)__set_broadcast(dummy, static_cast<int>(__L)...);
        (void)__get_master(dummy, static_cast<int>(__L)...);
        (void)__set_master(dummy, static_cast<int>(__L)...);
        (void)isLocal(__L...);
      }
    }


   /* constexpr */ int length(uint64_t d) const MOLLYATTR(inline)/*So the loop ranges are not hidden from Molly*/ { //MOLLY_DEBUG_FUNCTION_SCOPE
      //assert(0 <= d && d < (int)sizeof...(__L));
      return _select(d, __L...);
    }

    /// Overload for length(int) for !D
    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(__L)==1), typename std::conditional<true, int, Dummy>::type >::type
      /* constexpr */  length() MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
        return length(0);
    }


    /// Returns a pointer to the element with the given coordinates; Molly will track loads and stores to this memory location and insert communication code
    T *ptr(typename _inttype<__L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(ptrfunc) MOLLYATTR(inline) { //MOLLY_DEBUG_FUNCTION_SCOPE
      return (T*)__builtin_molly_ptr(this, coords...);
    }


    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(__L)==1), T&>::type
      MOLLYATTR(fieldmember) MOLLYATTR(inline) operator[](int64_t i)  { //MOLLY_DEBUG_FUNCTION_SCOPE
        //assert(0 <= i);
        //assert(i < _unqueue<__L...>::value);
        return *ptr(i);
    }

    typedef _array_partial_subscript<T, typename _unqueue<__L...>::first, typename _unqueue<__L...>::rest> subty;

    template<typename Dummy = void>
    typename std::enable_if<std::is_same<Dummy, void>::value && (sizeof...(__L)>1), subty>::type
      MOLLYATTR(fieldmember) MOLLYATTR(inline) operator[](int64_t i) { //MOLLY_DEBUG_FUNCTION_SCOPE
        //assert(0 <= i);
        //assert(i < _unqueue<__L...>::value);
        return subty(this, i);
    }

#pragma region Local access
    LLVM_ATTRIBUTE_USED void __get_local(T &val, typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(get_local) { MOLLY_DEBUG_FUNCTION_SCOPE
       //assert(__builtin_molly_islocal(this, coords...));
       auto idx = coords2idx(coords...);
       assert(0 <= idx && idx < localelts);
       assert(localdata);
       val = localdata[idx];
    }
    LLVM_ATTRIBUTE_USED void __set_local(const T &val, typename _inttype<__L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(set_local) { MOLLY_DEBUG_FUNCTION_SCOPE
      //assert(__builtin_molly_islocal(this, coords...));
      auto idx = coords2idx(coords...);
      assert(0 <= idx && idx < localelts);
      assert(localdata);
      localdata[idx] = val;
    }
    LLVM_ATTRIBUTE_USED T *__ptr_local(typename _inttype<__L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(ptr_local) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("Coords are (" << out_parampack(", ", coords...) << ")");
      //assert(__builtin_molly_islocal(this, coords...));
      auto idx = coords2idx(coords...);
      assert(0 <= idx && idx < localelts);
      assert(localdata);
      return &localdata[idx];
    }
    const T *__ptr_local(typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) {
      return const_cast<T*>(__ptr_local(coords...));
    }
#pragma endregion


   LLVM_ATTRIBUTE_USED void __get_broadcast(T &val, typename _inttype<__L>::type... coords) const MOLLYATTR(fieldmember) MOLLYATTR(get_broadcast);
    LLVM_ATTRIBUTE_USED void __set_broadcast(const T &val, typename _inttype<__L>::type... coords) MOLLYATTR(fieldmember) MOLLYATTR(set_broadcast) { MOLLY_DEBUG_FUNCTION_SCOPE
      MOLLY_DEBUG("coords=("<<out_parampack(", ", coords...) << ") __L=("<<out_parampack(", ", __L...) <<")");
      if (isLocal(coords...)) {
        __set_local(val, coords...);
      } else {
        // Nonlocal value, forget it!
        // In debug mode, we could check whether it is really equal on all ranks
      }
    }

    LLVM_ATTRIBUTE_USED void __get_master(T &val, typename _inttype<__L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_get_master)) { MOLLY_DEBUG_FUNCTION_SCOPE
    }
    LLVM_ATTRIBUTE_USED void __set_master(const T &val, typename _inttype<__L>::type... coords) const __attribute__((molly_fieldmember)) __attribute__((molly_set_master)) { MOLLY_DEBUG_FUNCTION_SCOPE
    }

    private:
      uint64_t localoffset(uint64_t d) { return __builtin_molly_localoffset(this, d); }
      uint64_t locallength(uint64_t d) { return __builtin_molly_locallength(this, d); }
  } MOLLYATTR(lengths(__L))/* NOTE: clang doesn't parse the whatever in [[molly::length(whatever)]] at all*/; // class array
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

#if !defined(__MOLLYRT)
    // TODO: Support multiple bit lengths
    MOLLYATTR(inline) int64_t mod(int64_t divident, int64_t divisor) {
      return __builtin_molly_mod(divident, divisor);
    }
#endif

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
template<typename T, int... __L>
LLVM_ATTRIBUTE_USED MOLLYATTR(fieldmember) MOLLYATTR(get_broadcast) void molly::array<T, __L...>::__get_broadcast(T &val, typename molly::_inttype<__L>::type... coords) const { MOLLY_DEBUG_FUNCTION_SCOPE
  if (isLocal(coords...)) {
    __get_local(val, coords...);
    broadcast_send(&val, sizeof(T)); // Send to other ranks so they can return the same result
  } else {
    broadcast_recv(&val, sizeof(T), coords2rank(coords...));
  }
}
#endif

extern "C" void waitToAttach();

void printArgs(int argc, char *argv[]);

#endif /* MOLLY_H */
