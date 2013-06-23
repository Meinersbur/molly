#ifndef ISLPP_MULTIVAL_H
#define ISLPP_MULTIVAL_H

#include "Ctx.h"
#include "Multi.h"
#include "Val.h"
#include "Obj.h"
#include <isl/val.h>

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {

  template<>
#define MultiVal Multi<Val> LLVM_FINAL
  class MultiVal : public Obj3<Multi<Val>, isl_multi_val> {
#undef MultiVal

  public:
    //typedef isl_multi_val StructTy;
    //typedef Multi<Val> ObjTy;

  public:
    void release() { isl_multi_val_free(takeOrNull()); }

  public:
    static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    Ctx *getCtx() const { return Ctx::wrap(isl_multi_val_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const {  }

  }; // class Multi<Val>

  MultiVal enwrap(isl_multi_val *obj) { return MultiVal::enwrap(obj); }

} // namespace isl
#endif /* ISLPP_MULTIVAL_H */
