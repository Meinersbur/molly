#ifndef ISLPP_OBJ_H
#define ISLPP_OBJ_H

//#include "islpp_common.h"
#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>
#include <cassert>
#include <string>
#include <type_traits>

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
} // namespace isl


namespace isl {

  // Curiously recursive template pattern to avoid virtual function calls
  template<typename D, typename S>
  class Obj3 {
#ifndef NDEBUG
  public:
    std::string _printed;
#endif

  public:
    typedef S StructTy;
    typedef D ObjTy;

  private:
    D *getDerived() { return static_cast<D*>(this); }
    const D *getDerived() const { return static_cast<const D*>(this); }

  private:
    StructTy *obj;

  public:
    StructTy *take() { 
      assert(obj); 
      StructTy *result = obj; 
      this->obj = nullptr; 
#ifndef NDEBUG
      this->_printed.clear();
#endif
      return result; 
    }

    StructTy *takeOrNull() { 
      StructTy *result = obj; 
      this->obj = nullptr; 
#ifndef NDEBUG
      this->_printed.clear();
#endif
      return result; 
    }

    StructTy *takeCopy() const { 
      assert(obj); 
      return getDerived()->takeCopyOrNull(); 
    }

    //virtual T *takeCopyOrNull() const = 0;

    StructTy *keep() const { 
      assert(obj); 
      return obj;
    }

    StructTy *keepOrNull() const { 
      return obj; 
    }

  protected:
    void give(StructTy *obj) {
      assert(obj); 
      getDerived()->release();
      this->obj = obj;
#ifndef NDEBUG
      this->_printed = getDerived()->toString();
#endif
    }

#ifndef NDEBUG
    void give(StructTy *obj, const std::string &printed) {
      assert(obj); 
      getDerived()->release(); 
      this->obj = obj; 
      this->_printed = printed;
    }

    void give(StructTy *obj, std::string &&printed) {
      assert(obj); 
      getDerived()->release(); 
      this->obj = obj; 
      this->_printed = std::move(printed);
    }
#endif


    void reset(StructTy *obj = nullptr) {
      if (obj)
      getDerived()->release();
#ifndef NDEBUG
      if (obj)
        this->_printed = getDerived()->toString();
      else
        this->_printed .clear();
#endif
    }

#ifndef NDEBUG
    void reset(StructTy *obj, const std::string &printed) {
      if (obj)
      getDerived()->release(); 
      this->obj = obj; 
      if (obj) 
        this->_printed = printed;
      else
        this->_printed .clear();
    }

    void reset(StructTy *obj, std::string &&printed) {
      if (obj)
      getDerived()->release();
      this->obj = obj; 
      if (obj) 
        this->_printed = std::move(printed);
      else
        this->_printed .clear();
    }
#endif

  public:
    ~Obj3() { 
      getDerived()->release();
#ifndef NDEBUG
      this->obj = nullptr;
#endif
    }



    Obj3() : obj(nullptr) {  }
    explicit Obj3(StructTy *obj) : obj(obj) {
#ifndef NDEBUG
      if (obj) {
        llvm::raw_string_ostream stream(_printed);
        getDerived()->print(stream);
        stream.flush();
      }
#endif
    }
#ifndef NDEBUG
    Obj3(StructTy *obj, const std::string &printed) : _printed(printed), obj(obj) { }
    Obj3(StructTy *obj,  std::string &&printed) : _printed(std::move(printed)), obj(obj) { }
#endif

  public:
    bool isValid () const { return obj; }
    bool isNull() const { return !obj; }
    LLVM_EXPLICIT operator bool() { return obj; }

    std::string toString() {
      std::string buf;
      if (obj) {
        llvm::raw_string_ostream stream(_printed);
        getDerived()->print(stream);
        stream.flush();
      }
      return buf;
    }
  }; // class Obj3


#if 0
  // Non-template dependent stuff from Obj2<T>
  class ObjBase {
  protected:
    virtual void release() = 0;

  public:
    bool isNull() const { return !isValid(); }
    virtual bool isValid() const = 0;

#pragma region Printing
    virtual void print(llvm::raw_ostream &) const = 0;
    std::string toString() const;
#pragma endregion

    virtual Ctx *getCtx() const = 0;
  }; // class ObjBase


  template<typename T>
  class Obj2 : public ObjBase {
  public:
    typedef T StructTy;

  private: 
#ifndef NDEBUG
    std::string _printed;
#endif
    T *obj;

  public:
    T *take() { 
      assert(obj); 
      T *result = obj; 
      this->obj = nullptr; 
#ifndef NDEBUG
      this->_printed.clear();
#endif
      return result; 
    }

    T *takeOrNull() { 
      T *result = obj; 
      this->obj = nullptr; 
#ifndef NDEBUG
      this->_printed.clear();
#endif
      return result; 
    }

    T *takeCopy() const  { 
      assert(obj); 
      return takeCopyOrNull(); 
    }

    virtual T *takeCopyOrNull() const = 0;

    T *keep() const { 
      assert(obj); 
      return obj;
    }

    T *keepOrNull() const { 
      return obj; 
    }

  protected:
#ifndef NDEBUG
    void give(T *obj, const std::string *printed = nullptr) {
#else
    void give(T *obj) {
#endif
      assert(obj); 
      this->release(); 
      this->obj = obj; 
#ifndef NDEBUG
      if (printed)
        this->_printed = *printed;
      else
        this->_printed = toString();
#endif
    }

#ifndef NDEBUG
    void reset(T *obj = nullptr, const std::string *printed = nullptr) { 
#else
    void reset(T *obj = nullptr) { 
#endif
      this->release(); 
      this->obj = obj; 
#ifndef NDEBUG
      if (obj) {
        if (printed)
          this->_printed = *printed;
        else
          this->_printed = toString();
      } else
        this->_printed.clear();
#endif
    }

  public:
    virtual ~Obj2() { 
      assert(obj == 0 && "Call release in the derived class's destructor; we cannot do it here because release() is virtual, i.e. not callable in the destructor"); 
    }

  protected:
    Obj2() : obj(nullptr) {}

#ifndef NDEBUG
    //explicit Obj2(T* obj): obj(obj), _printed(toString()) { }
    Obj2(T* obj, const std::string &printed): _printed(printed), obj(obj) { }
    Obj2(T* obj, std::string &&printed): _printed(std::move(printed)), obj(obj) { }
#else
    explicit Obj2(T* obj): obj(obj) { }
#endif


  public:
    bool isValid() const LLVM_OVERRIDE { return obj; }
  }; // class Obj2
#endif


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
    T *takeOrNull() { T *result = obj; this->obj = nullptr; return result; }
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
