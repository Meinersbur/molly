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
  class Val LLVM_FINAL : public Obj2<isl_val> {
#pragma region Low-level
  public: // Public because otherwise we had to add a lot of friends
    //isl_pw_multi_aff *take() { assert(pwmaff); isl_pw_multi_aff *result = pwmaff; pwmaff = nullptr; return result; }
    isl_val *takeCopy() const LLVM_OVERRIDE { return isl_val_copy(keep()); }

    // isl_pw_multi_aff *keep() const { assert(pwmaff); return pwmaff; }
  protected:
    void release() { isl_val_free(take()); }
    //void give(isl_pw_multi_aff *pwmaff) { assert(pwmaff); if (this->pwmaff) isl_pw_multi_aff_free(pwmaff); this->pwmaff = pwmaff; }

  public:
    ~Val() { release(); }
    Val() : Obj2() { }
    static Val enwrap(isl_val *val) { assert(val); Val result; result.give(val); return result; }
#pragma endregion

    Ctx *getCtx() const { return Ctx::wrap(isl_val_get_ctx(keep())); }

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth, int indent) const;
#pragma endregion
  }; // class Val

  inline Val enwrap(isl_val *val) { Val::enwrap(val); }

} // namespace isl
#endif /* ISLPP_VAL_H */
