#ifndef ISLPP_OBJ_H
#define ISLPP_OBJ_H

#include "islpp_common.h"
#include <llvm/Support/Compiler.h>
#include <llvm/Support/raw_ostream.h>
#include <cassert>
#include <string>
#include <type_traits>
#include <llvm/Support/Debug.h>

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
} // namespace isl

#ifndef __isl_keep
#define __isl_keep
#endif

#ifndef __isl_take
#define __isl_take
#endif

#ifndef __isl_give
#define __isl_give
#endif


namespace isl {

  // Curiously recursive template pattern to avoid virtual function calls
  template<typename D, typename S>
  class Obj {
#ifndef NDEBUG
  public:
    std::string _printed; // A cached toString() output to allow readable value inspection during debugging
    mutable bool _lastWasCopy; // set if the last operation was to make a (logical) copy of it; if the object is freed then this means that it would have been cheaper to move the object instead of making that copy
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
      this->_lastWasCopy = false;
#endif
      return result; 
    }

    StructTy *takeOrNull() { 
      StructTy *result = obj; 
      this->obj = nullptr; 
#ifndef NDEBUG
      this->_printed.clear();
      this->_lastWasCopy = false;
#endif
      return result; 
    }

    StructTy *takeCopy() const { 
      assert(obj); 
      return takeCopyOrNull(); 
    }

    StructTy *takeCopyOrNull() const {
      auto result = getDerived()->addref();
      assert(!result == !obj);
#ifndef NDEBUG
      this->_lastWasCopy = true;
#endif
      return result;
    }

    StructTy *keep() const { 
      assert(obj); 
#ifndef NDEBUG
      this->_lastWasCopy = false;
#endif
      return obj;
    }

    StructTy *keepOrNull() const { 
#ifndef NDEBUG
      this->_lastWasCopy = false;
#endif
      return obj; 
    }

    StructTy **change() LLVM_LVALUE_FUNCTION { 
      if (!this)
        return nullptr;
#ifndef NDEBUG
      this->_printed = "<outdated>";
      this->_lastWasCopy = false;
      //TODO: need to run code after the obj has been updated, possibly by returning a proxy object that destructs after the expression
#endif
      return &obj; 
    }


    void clear() LLVM_LVALUE_FUNCTION {
      reset();
    }

  public:
    void give(StructTy *obj) ISLPP_INPLACE_FUNCTION {
      assert(obj); 
      getDerived()->release();
      this->obj = obj;
#ifndef NDEBUG
      if (obj) {
        llvm::raw_string_ostream stream(this->_printed);
        getDerived()->print(stream);
        //stream.flush();
      } else {
        this->_printed.clear();
      }
      this->_lastWasCopy = false;
#endif
    }

#ifndef NDEBUG
    void give(StructTy *obj, const std::string &printed) {
      assert(obj); 
      getDerived()->release(); 
      this->obj = obj; 
      this->_printed = printed;
      this->_lastWasCopy = false;
    }

    void give(StructTy *obj, std::string &&printed) {
      assert(obj); 
      getDerived()->release(); 
      this->obj = obj; 
      this->_printed = std::move(printed);
      this->_lastWasCopy = false;
    }
#endif

  protected:
    void obj_give(const Obj<D,S> &that) {
      if (&that == this)
        return;

      if (this->obj)
        getDerived()->release(); 

      this->obj = that.takeCopy(); 
#ifndef NDEBUG
      this->_printed = that._printed;
      this->_lastWasCopy = false;
#endif
    }

    void obj_give(Obj<D,S> &&that) {
      assert(&that != this);

      if (this->obj)
        getDerived()->release(); 

#ifndef NDEBUG
      this->_printed = std::move(that._printed);
      this->_lastWasCopy = false;
#endif
      this->obj = that.take(); 
    }

    void obj_reset(const ObjTy &that) {
      if (&that == this)
        return;

      if (this->obj)
        getDerived()->release(); 

      this->obj = that.takeCopyOrNull(); 
#ifndef NDEBUG
      if (this->obj) 
        this->_printed = that._printed;
      else
        this->_printed.clear();
      this->_lastWasCopy = false;
#endif
    }

    void obj_reset(ObjTy &&that) {
      assert (&that != this);

      if (this->obj)
        getDerived()->release(); 

#ifndef NDEBUG
      this->_printed = std::move(that._printed);
      this->_lastWasCopy = false;
#endif
      this->obj = that.takeOrNull();
    }

  public:
    //FIXME: Name collision with isl_space_reset
    void reset(StructTy *obj = nullptr) {
      if (this->obj)
        getDerived()->release();
      this->obj = obj;
#ifndef NDEBUG
      if (obj) {
        llvm::raw_string_ostream stream(this->_printed);
        getDerived()->print(stream);
        //stream.flush();
      } else
        this->_printed.clear();
      this->_lastWasCopy = false;
#endif
    }


#ifndef NDEBUG
    void reset(StructTy *obj, const std::string &printed) {
      if (obj)
        getDerived()->release(); 
      this->obj = obj; 
      this->_printed = printed;
      this->_lastWasCopy = false;
    }

    void reset(StructTy *obj, std::string &&printed) {
      if (obj)
        getDerived()->release();
      this->obj = obj; 
      this->_printed = std::move(printed);
      this->_lastWasCopy = false;
    }
#endif


  public:
    ~Obj() { 
#ifndef NDEBUG
      if (obj && _lastWasCopy) {
        //llvm::dbgs() << "PerfWarn: isl::Obj " << _printed << " has been copied from, but not reused after that. Try to use _inplace methods or rvalue references.\n";
      }
#endif
      getDerived()->release();
#ifndef NDEBUG
      this->obj = nullptr;
#endif
    }


  protected:
#ifndef NDEBUG
    Obj() : _printed(), _lastWasCopy(false), obj(nullptr) {  }
#else
    Obj() : obj(nullptr) {  }
#endif

#ifndef NDEBUG
    explicit Obj(ObjTy &&that) : _printed(std::move(that._printed)), _lastWasCopy(false), obj(that.takeOrNull()) { }
    explicit Obj(const ObjTy &that) : _printed(that._printed), _lastWasCopy(false), obj(that.takeCopyOrNull()) { }
#else
    explicit Obj(ObjTy &&that) : obj(that.takeOrNull()) { }
    explicit Obj(const ObjTy &that) : obj(that.keepOrNull()) { }
#endif

    explicit Obj(StructTy *obj) : obj(obj) {
#ifndef NDEBUG
      if (obj) {
        llvm::raw_string_ostream stream(_printed);
        getDerived()->print(stream);
        //stream.flush();
      }
      this->_lastWasCopy = false;
#endif
    }

  public:
    bool isValid() const { return obj; }
    bool isNull() const { return !obj; }
    //LLVM_EXPLICIT operator bool() { return isValid(); }

    std::string toString() {
      std::string buf;
      if (obj) {
        llvm::raw_string_ostream stream(buf);
        getDerived()->print(stream);
        //stream.flush();
      }
      return buf;
    }

    // Necessary, since we have move ctor/assignment?
    static void swap(ObjTy &lhs, ObjTy &rhs) {
      std::swap(lhs.obj, rhs.obj);
#ifndef NDEBUG
      std::swap(lhs._printed, rhs._printed);
      std::swap(lhs._lastWasCopy, rhs._lastWasCopy);
#endif
    }

    ObjTy copy() const { 
      ObjTy result;
      result.obj_reset(*getDerived());
#ifndef NDEBUG
      // Should have been done implicitly anyway
      this->_lastWasCopy = true;
#endif
      return result;
    }
    ObjTy &&move() { 
      return std::move(*getDerived()); 
    }

    static ObjTy enwrap(__isl_take StructTy *obj) {
      ObjTy result;
      result.reset(obj);
      return result; // NRVO
    }

    static ObjTy enwrapCopy(__isl_keep StructTy *obj) {
      ObjTy result;
      result.reset(obj);
      // A temporary obj such that we can call its takeCopyOrNull method, no need to require implementors to implement a second version of it
      //result.obj = result.takeCopyOrNull();
       result.addref();
      return result; // NRVO
    }

    // Default implementation if superclass doesn't provide one
    void dump() const {
      getDerived()->print( llvm::errs() );
    }
  }; // class Obj

} // namespace isl
#endif /* ISLPP_OBJ_H */
