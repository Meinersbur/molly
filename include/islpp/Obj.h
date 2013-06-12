#ifndef ISLPP_OBJ_H
#define ISLPP_OBJ_H

#include <assert.h>

namespace isl {
  class Ctx;
} // namespace isl


namespace isl {
  template<typename T>
  class Obj2 {
  public:
    typedef T StructTy;

  private: 
    T *obj;

  protected:
    virtual void release() = 0;

  public:
    T *take() { assert(obj); T *result = obj; this->obj = nullptr; return result; }
    virtual T *takeCopy() const = 0;
    T *keep() const { assert(obj); return obj; }

  protected:
    void give(T *obj) { assert(obj); this->release(); this->obj = obj;  }

  public:
    virtual ~Obj2() { assert(obj == 0 && "Call release in the derived class's destructor; we cannot do it here because release() is virtual, i.e. not callable in the destructor"); }
    Obj2() : obj(nullptr) {}

  protected:
    explicit Obj2(T* obj) : obj(obj) {}

  public:
     virtual Ctx *getCtx() const = 0;
  }; // class Obj2


  class Obj {
  public:
    virtual ~Obj() {}
    virtual Ctx *getCtx() const = 0;
  }; // class Obj
} // namepspace isl
#endif /* ISLPP_OBJ_H */
