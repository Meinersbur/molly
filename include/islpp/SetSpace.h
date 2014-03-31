#ifndef ISLPP_SETSPACE_H
#define ISLPP_SETSPACE_H

#include "islpp_common.h"
#include "Islfwd.h"
#include "Obj.h" // class Obj<SetSpace, isl_space>
#include "SetSpacelike.h" // SetSpacelike<SetSpace>

#include <isl/space.h> // isl_space


namespace isl {

  class SetSpace : public Obj<SetSpace, isl_space>, public SetSpacelike<SetSpace> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_space_free(takeOrNull()); }
    StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    SetSpace() { }

    /* implicit */ SetSpace(const ObjTy &that) : Obj(that) { }
    /* implicit */ SetSpace(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    ISLPP_PROJECTION_ATTRS Ctx *getCtx() ISLPP_PROJECTION_FUNCTION;
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion

#pragma region isl::SetSpacelike
    ISLPP_PROJECTION_ATTRS SetSpace getSpace() ISLPP_PROJECTION_FUNCTION { return *this; }
    ISLPP_PROJECTION_ATTRS SetSpace getSpacelike() ISLPP_PROJECTION_FUNCTION { return *this; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION { return isl_space_dim(keep(), type); }
#pragma endregion

  public:
    operator Space() const;
  }; // class SetSpace

} // namespace molly

#endif /* ISLPP_SETSPACE_H */
