#ifndef ISLPP_VEC_H
#define ISLPP_VEC_H

#include "islpp_common.h"
#include "Obj.h"
#include <isl/vec.h>
#include "Ctx.h"
#include "Int.h"
#include "Val.h"
#include <isl/deprecated/vec_int.h>

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {

  class Vec : public Obj<Vec, isl_vec> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_vec_free(takeOrNull()); }
    StructTy *addref() const { return isl_vec_copy(keepOrNull()); }

  public:
    Vec() { }

    /* implicit */ Vec(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Vec(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_vec_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_vec_dump(keep()); }
#pragma endregion



#pragma region Creational
    static Vec create(Ctx *ctx, unsigned size) { return enwrap(isl_vec_alloc(ctx->keep(), size)); }

    static Vec readFromFile(Ctx *ctx, FILE *input) { return enwrap(isl_vec_read_from_file(ctx->keep(), input)); }
#pragma endregion Creational


#pragma region Conversion
  BasicSet toBasicSet() const;
#pragma endregion


#pragma region Properties
    //Ctx *getCtx() const { return Ctx::enwrap(isl_vec_get_ctx(keep())); }

    int getSize() const { return isl_vec_size(keep()); }
#pragma endregion

    bool getElement(int pos, Int &v) const { isl_int val; auto retval = isl_vec_get_element(keep(), pos, &val); v = Int::wrap(val); return retval==0; }
    Int getElement(pos_t pos) const { isl_int result; auto retval = isl_vec_get_element(keep(), pos, &result); assert(retval==0); return Int::wrap(result); }
    Val getElementVal(int pos) const { return Val::enwrap(isl_vec_get_element_val(keep(), pos)); }

    Vec setElement(int pos, const Int &v) const { return enwrap(isl_vec_set_element(takeCopy(), pos, v.keep())); }
    Vec setElement(int pos, int v) const { return enwrap(isl_vec_set_element_si(takeCopy(), pos, v)); }
    Vec setElement(int pos, Val &&v) const { return enwrap(isl_vec_set_element_val(takeCopy(), pos, v.take())); }

    Vec set(const Int &v) const { return enwrap(isl_vec_set(takeCopy(), v.keep())); } 
    Vec set(int v) const { return enwrap(isl_vec_set_si(takeCopy(), v)); } 
    Vec set(Val &&v) const { return enwrap(isl_vec_set_val(takeCopy(), v.take())); } 


#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion


#pragma region Operations
    //Int lcm() const { isl_int result; isl_vec_lcm(keep(), &result); return Int::wrap(result); }
    Vec ceil() const { return enwrap(isl_vec_ceil(takeCopy())); }
    Vec normalize() const { return enwrap(isl_vec_normalize(takeCopy())); }
    Vec clear() const { return enwrap(isl_vec_clr(takeCopy())); }
    Vec neg() const { return enwrap(isl_vec_neg(takeCopy())); }
    //Vec scale(const Int &m) const { return enwrap(isl_vec_scale(takeCopy(), m.keep())); }
    Vec fdiv_r(const Int &m) const { return enwrap(isl_vec_fdiv_r(takeCopy(), m.keep())); }
    Vec extend(unsigned size) const { return enwrap(isl_vec_extend(takeCopy(), size)); }
    Vec zeroExtend(unsigned size) const { return enwrap(isl_vec_zero_extend(takeCopy(), size)); }
    Vec sort() const { return enwrap( isl_vec_sort(takeCopy())); }
#pragma endregion


    Vec dropEls(unsigned pos, unsigned n) { return enwrap(isl_vec_drop_els(takeCopy(), pos, n)); }
    Vec insertEls(unsigned pos, unsigned n) { return enwrap(isl_vec_insert_els(takeCopy(), pos, n)); }
    Vec insertZeroEls(unsigned pos, unsigned n) { return enwrap(isl_vec_insert_zero_els(takeCopy(), pos, n)); }
 
  
  
  }; // class Vec


  inline Vec enwrap(isl_vec *vec) { return Vec::enwrap(vec); }

  inline bool isEqual(const Vec &vec1, const Vec &vec2) { return isl_vec_is_equal(vec1.keep(), vec2.keep()); }
  inline int cmpElement(const Vec &vec1, const Vec &vec2, int pos){ return isl_vec_cmp_element(vec1.keep(), vec2.keep(), pos); }
  inline Vec add(const Vec &vec1, const Vec &vec2){ return Vec::enwrap(isl_vec_add(vec1.keep(), vec2.keep())); }
  inline Vec concat(const Vec &vec1, const Vec &vec2){ return Vec::enwrap(isl_vec_concat(vec1.keep(), vec2.keep())); }

} // namespace isl
#endif /* ISLPP_VEC_H */
