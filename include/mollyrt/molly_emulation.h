#ifndef MOLLY_EMULATION_H
#define MOLLY_EMULATION_H

#include <cstdint>
#include <type_traits>

#define MOLLY_ATTR(X) 
#define MOLLY_PRAGMA(X)



namespace molly {

#pragma region __storage_type
  template<typename T, int64_t... __L>
  class __storage_type;

  template<typename T>
  class __storage_type<T> {
  public:
    typedef T type;
  }; //class  __storage_type

  template<typename T, int64_t Lfirst, int64_t... __L>
  class __storage_type<T,Lfirst,__L...> {
  public:
    typedef typename __storage_type<T, __L...>::type type[Lfirst];
  }; //class  __storage_type
#pragma endregion

#if 0
#pragma region __storage_type_but_one
  template<typename T, int64_t... __L>
  class __storage_type_but_one;

  template<typename T, int64_t Lfirst>
  class __storage_type_but_one<T,Lfirst> {
  public:
    typedef T type;
  }; //class  __storage_type_but_one

  template<typename T, int64_t Lfirst, int64_t... __L>
  class __storage_type_but_one<T, Lfirst, __L...> {
  public:
    typedef typename __storage_type_but_one<T, __L...>::type type[Lfirst];
  }; //class  __storage_type_but_one
#pragma endregion
#else
  template<typename T, int64_t... __L>
  class __storage_type_but_one;

  template<typename T, int64_t Lfirst, int64_t... __L>
  class __storage_type_but_one<T, Lfirst, __L...> {
  public:
    typedef typename __storage_type<T, __L...>::type type;
  }; //class  __storage_type_but_one
#endif

#pragma region _select
  static inline int _select(int i); // Forward declaration

  template<typename FirstType, typename... Types>
  static inline int _select(int i, FirstType first, Types... list)  {
      if (i == 0)
        return first;
      return _select(i - 1, list...); // This is no recursion, every _select has a different signature
  }

   static inline int _select(int i) { // overload for compiler-time termination, should never be called
#ifdef _MSC_VER
      __assume(false);
#else
      __builtin_unreachable();
#endif
  }
#pragma endregion


  template<typename T, int64_t... __L>
  class array {
    typedef typename __storage_type<T, __L...>::type storageTy;

  private:
    storageTy data;

  public:
    //decltype(data[0])
   typename  __storage_type_but_one<T,__L...>::type
      &operator[](int64_t idx) {
      return data[idx];
    }

    int64_t length(int64_t d) const { 
      return _select(d, __L...);
    }

    template<typename Dummy = void>
    typename std::enable_if<(sizeof...(__L) == 1), typename std::conditional<true, int, Dummy>::type >::type
    length() {
        return length(0);
    }
  }; // class array


  static inline int64_t mod(int64_t divident, int64_t divisor) {
    return (divisor + divident % divisor) % divisor;
  }

} // namespace molly

static inline bool __molly_isMaster() {
  return true;
}

static inline void waitToAttach() {
}

#endif /* MOLLY_EMULATION_H */
