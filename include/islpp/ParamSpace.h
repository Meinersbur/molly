#ifndef ISLPP_PARAMSPACE_H
#define ISLPP_PARAMSPACE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Space.h"

namespace isl {

  class ParamSpace : public Space {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    //void release() { isl_space_free(takeOrNull()); }
    //StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    ParamSpace() { }

    /* implicit */ ParamSpace(const ObjTy &that) : Space(that) { }
    /* implicit */ ParamSpace(ObjTy &&that) : Space(std::move(that)) { }
    //const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    //const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    //ISLPP_PROJECTION_ATTRS Ctx *getCtx() ISLPP_PROJECTION_FUNCTION;
    //void print(llvm::raw_ostream &out) const;
    //void dump() const;
#pragma endregion

  public:
    static ParamSpace enwrap(__isl_take StructTy *obj) {
      ParamSpace result;
      result.reset(obj);
      return result; // NRVO
    }

    static ParamSpace enwrapCopy(__isl_keep StructTy *obj) {
      ParamSpace result;
      result.reset(obj);
      // A temporary obj such that we can call its takeCopyOrNull method, no need to require implementors to implement a second version of it
      //result.obj = result.takeCopyOrNull();
      result.addref();
      return result; // NRVO
    }


  }; // class ParamSpace

} // namespace molly

#endif /* ISLPP_PARAMSPACE_H */
