#ifndef ISLPP_VALLIST_H
#define ISLPP_VALLIST_H

#include "islpp_common.h"
#include "Islfwd.h"

#include "Obj.h"
#include "Ctx.h"
#include <isl/val.h>


namespace isl {

  template<>
  class List<Val> : public Obj<List<Val>, isl_val_list> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_val_list_free(takeOrNull()); }
    StructTy *addref() const { return isl_val_list_copy(keepOrNull()); }

  public:
    List() { }

    /* implicit */ List(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ List(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_val_list_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion

  }; // class List<Val>

} // namespace isl
#endif /* ISLPP_VALLIST_H */
