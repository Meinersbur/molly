#ifndef ISLPP_VAL_H
#define ISLPP_VAL_H 

#include "islpp_common.h"

#include "Obj.h"  // class Obj<> (base of Val)

#include "Ctx.h"
#include <isl/val.h>
#include <cassert>


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
    void dump() const;
    // isl_val_to_str
#pragma endregion


#pragma region Conversion
     ISLPP_EXSITU_ATTRS double toDouble() ISLPP_EXSITU_FUNCTION { return isl_val_get_d(keep()); }
#pragma endregion


#pragma region Construction
    static Val zero(Ctx *ctx) { return enwrap(isl_val_zero(ctx->keep())); }
    static Val one(Ctx *ctx) { return enwrap(isl_val_one(ctx->keep())); }
    static Val nan(Ctx *ctx) { return enwrap(isl_val_nan(ctx->keep())); }
    static Val infty(Ctx *ctx) { return enwrap(isl_val_infty(ctx->keep())); }
     static Val neginfty(Ctx *ctx) { return enwrap(isl_val_neginfty(ctx->keep())); }
     static Val intFrom(Ctx *ctx, long i) { return enwrap(isl_val_int_from_si( ctx->keep(), i )); }
      static Val intFrom(Ctx *ctx, unsigned long ui) { return enwrap(isl_val_int_from_ui( ctx->keep(), ui )); }
        static Val intFromChunks(Ctx *ctx, size_t n, size_t size, const void *chunks) { return enwrap(isl_val_int_from_chunks( ctx->keep(), n, size, chunks )); }

        static Val fromStr(Ctx *ctx, const char *str) { return enwrap(isl_val_read_from_str(ctx->keep(), str)); }
#pragma endregion

      ISLPP_EXSITU_ATTRS long getNumSi() ISLPP_EXSITU_FUNCTION { return isl_val_get_num_si(keep()); }
      ISLPP_EXSITU_ATTRS long getDenSi() ISLPP_EXSITU_FUNCTION { return isl_val_get_den_si(keep()); }
      ISLPP_EXSITU_ATTRS double getD() ISLPP_EXSITU_FUNCTION { return isl_val_get_d(keep()); }

      ISLPP_EXSITU_ATTRS size_t nAbsNumChunks(size_t bytesPerChunk) ISLPP_EXSITU_FUNCTION { return isl_val_n_abs_num_chunks(keep(), bytesPerChunk); }
      ISLPP_EXSITU_ATTRS bool getAbsNumChunks(size_t size, void *chunks) ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_get_abs_num_chunks(keep(), size, chunks)); }

      ISLPP_EXSITU_ATTRS int sgn() ISLPP_EXSITU_FUNCTION { return isl_val_sgn(keep()); }
      ISLPP_EXSITU_ATTRS bool isZero() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_zero(keep())); }
      ISLPP_EXSITU_ATTRS bool isOne() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_one(keep())); }
      ISLPP_EXSITU_ATTRS bool isNegone() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_negone(keep())); }
      ISLPP_EXSITU_ATTRS bool isNonneg() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_nonneg(keep())); }
      ISLPP_EXSITU_ATTRS bool isNonpos() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_nonpos(keep())); }
      ISLPP_EXSITU_ATTRS bool isPos() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_pos(keep())); }
      ISLPP_EXSITU_ATTRS bool isNeg() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_pos(keep())); }
      ISLPP_EXSITU_ATTRS bool isInt() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_int(keep())); }
      ISLPP_EXSITU_ATTRS bool isRat() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_rat(keep())); }
      ISLPP_EXSITU_ATTRS bool isNan() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_nan(keep())); }
      ISLPP_EXSITU_ATTRS bool isInfty() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_infty(keep())); }
      ISLPP_EXSITU_ATTRS bool isNeginfty() ISLPP_EXSITU_FUNCTION { return checkBool(isl_val_is_neginfty(keep())); }

    void printProperties(llvm::raw_ostream &out, int depth, int indent) const;
  }; // class Val


  static inline Val enwrap(isl_val *val) { return Val::enwrap(val); }
  static inline Val enwrapCopy(isl_val *val) { return Val::enwrapCopy(val); }

 static inline Val set(Val v, long i) { return Val::enwrap(isl_val_set_si(v.take(), i)); }

  static inline Val abs(Val v) { return Val::enwrap(isl_val_abs(v.take())); }
   static inline Val neg(Val v) { return Val::enwrap(isl_val_neg(v.take())); }
   static inline Val floor(Val v) { return Val::enwrap(isl_val_floor(v.take())); }
   static inline Val ceil(Val v) { return Val::enwrap(isl_val_ceil(v.take())); }
   static inline Val trunc(Val v) { return Val::enwrap(isl_val_trunc(v.take())); }
   static inline Val twoexp(Val v) { return Val::enwrap(isl_val_2exp(v.take())); }

   static inline Val min(Val v1, Val v2) { return Val::enwrap(isl_val_min(v1.take(), v2.take())); }
   static inline Val max(Val v1, Val v2) { return Val::enwrap(isl_val_max(v1.take(), v2.take())); }
   static inline Val add(Val v1, Val v2) { return Val::enwrap(isl_val_add(v1.take(), v2.take())); }
   static inline Val add(Val v1, unsigned long v2) { return Val::enwrap(isl_val_add_ui(v1.take(), v2)); }
   static inline Val sub(Val v1, Val v2) { return Val::enwrap(isl_val_sub(v1.take(), v2.take())); }
   static inline Val sub(Val v1, unsigned long v2) { return Val::enwrap(isl_val_sub_ui(v1.take(), v2)); }
   static inline Val mul(Val v1, Val v2) { return Val::enwrap(isl_val_mul(v1.take(), v2.take())); }
   static inline Val mul(Val v1, unsigned long v2) { return Val::enwrap(isl_val_mul_ui(v1.take(), v2)); }
   static inline Val div(Val v1, Val v2) { return Val::enwrap(isl_val_div(v1.take(), v2.take())); }
   static inline Val mod(Val v1, Val v2) { return Val::enwrap(isl_val_mod(v1.take(), v2.take())); }
   static inline Val gcd(Val v1, Val v2) { return Val::enwrap(isl_val_gcd(v1.take(), v2.take())); }
   static inline Val gcd(Val v1, Val v2, Val &x, Val &y) { return Val::enwrap(isl_val_gcdext(v1.take(), v2.take(), x.change(), y.change())); }

   static inline bool cmp(const Val &v, long i) { return isl_val_cmp_si(v.keep(), i); }
   static inline bool lt(const Val &v1, const Val &v2) { return isl_val_lt(v1.keep(), v2.keep()); }
   static inline bool le(const Val &v1, const Val &v2) { return isl_val_le(v1.keep(), v2.keep()); }
   static inline bool gt(const Val &v1, const Val &v2) { return isl_val_gt(v1.keep(), v2.keep()); }
   static inline bool ge(const Val &v1, const Val &v2) { return isl_val_ge(v1.keep(), v2.keep()); }
   static inline bool ne(const Val &v1, const Val &v2) { return isl_val_ne(v1.keep(), v2.keep()); }

   static inline bool isDivisibleBy(const Val &v1, const Val &v2) { return isl_val_is_divisible_by(v1.keep(), v2.keep()); }


} // namespace isl
#endif /* ISLPP_VAL_H */
