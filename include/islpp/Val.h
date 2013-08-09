#ifndef ISLPP_VAL_H
#define ISLPP_VAL_H 1

#include "islpp_common.h"
#include <assert.h>
#include "Obj.h"
#include <isl/val.h>
#include "Ctx.h"

struct isl_val;

namespace isl {
  class Val;
} // namespace isl

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {
  class Val : public Obj<Val, isl_val> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_val_free(takeOrNull()); }
    StructTy *addref() const { return isl_val_copy(keepOrNull()); }

  public:
    Val() { }

    /* implicit */ Val(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Val(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_val_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_val_dump(keep()); }
#pragma endregion



   // Ctx *getCtx() const { return Ctx::enwrap(isl_val_get_ctx(keep())); }

#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth, int indent) const;
#pragma endregion
  }; // class Val


  static inline Val enwrap(isl_val *val) { return Val::enwrap(val); }
  static inline Val enwrapCopy(isl_val *val) { return Val::enwrapCopy(val); }

} // namespace isl
#endif /* ISLPP_VAL_H */
