#ifndef ISLPP_SETSPACE_H
#define ISLPP_SETSPACE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Obj.h" // class Obj<SetSpace, isl_space>
#include "SetSpacelike.h" // SetSpacelike<SetSpace>

#include <isl/space.h> // isl_space


namespace isl {

  class SetSpace : /*public Obj<SetSpace, isl_space>,*/ public Space, public SetSpacelike<SetSpace> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    //void release() { isl_space_free(takeOrNull()); }
    //StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    SetSpace() { }

    /* implicit */ SetSpace(const ObjTy &that) : Space(that) { }
    /* implicit */ SetSpace(ObjTy &&that) : Space(std::move(that)) { }
    //const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    //const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    //ISLPP_PROJECTION_ATTRS Ctx *getCtx() ISLPP_PROJECTION_FUNCTION;
    //void print(llvm::raw_ostream &out) const;
    //void dump() const;
#pragma endregion

    static SetSpace enwrap(__isl_take StructTy *obj) {
      SetSpace result;
      result.reset(obj);
      return result; // NRVO
    }

    static SetSpace enwrapCopy(__isl_keep StructTy *obj) {
      SetSpace result;
      result.reset(obj);
      // A temporary obj such that we can call its takeCopyOrNull method, no need to require implementors to implement a second version of it
      //result.obj = result.takeCopyOrNull();
      result.addref();
      return result; // NRVO
    }

#if 0
#pragma region isl::SetSpacelike
    ISLPP_PROJECTION_ATTRS SetSpace getSpace() ISLPP_PROJECTION_FUNCTION { return *this; }
    ISLPP_PROJECTION_ATTRS SetSpace getSpacelike() ISLPP_PROJECTION_FUNCTION { return *this; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_space_dim(keep(), type); }
#pragma endregion
#endif

  public:
    //operator Space() const;

    void matrixTypePos(size_t offset, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type &type, pos_t &pos)const {
      auto n1 = dim(c1);
      if (offset < n1) {
        type = c1;
        pos = offset;
        return;
      }
      offset -= n1;

      auto n2 = dim(c2);
      if (offset < n2) {
        type = c2;
        pos = offset;
        return;
      }
      offset -= n2;

      auto n3 = dim(c3);
      if (offset < n3) {
        type = c3;
        pos = offset;
        return;
      }
      offset -= n3;

      auto n4 = dim(c4);
      if (offset < n4) {
        type = c4;
        pos = offset;
        return;
      }
      offset -= n4;
      llvm_unreachable("out of range");
    }
    void matrixTypePos(size_t offset, isl_dim_type &type, pos_t &pos) const {
      matrixTypePos(offset, isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div, type, pos);
    }
  }; // class SetSpace

} // namespace molly

#endif /* ISLPP_SETSPACE_H */
