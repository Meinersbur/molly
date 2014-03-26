#ifndef MOLLY_EMULATION_H
#define MOLLY_EMULATION_H

#include <cstdint>

#define MOLLY_ATTR(X) 
#define MOLLY_PRAGMA(X)



namespace molly {

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



  template<typename T, int64_t... __L>
  class array {
    typedef typename __storage_type<T, __L...>::type storageTy;

  private:
    storageTy data;

  public:
    decltype(data[0]) &operator[](int64_t idx) {
      return data[idx];
    }
  }; // class array
} // namespace molly

static inline bool __molly_isMaster() {
  return true;
}

#endif /* MOLLY_EMULATION_H */
