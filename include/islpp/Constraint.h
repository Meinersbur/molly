#ifndef ISLPP_CONSTRAINT_H
#define ISLPP_CONSTRAINT_H

#include "islpp_common.h"
#include <cassert>
#include <string>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/constraint.h>
#include <isl/aff.h>
#include "Aff.h"
#include "Obj.h"
#include "Ctx.h"
#include "LocalSpace.h"
#include "Int.h"
#include "Expr.h"
#include <isl/deprecated/constraint_int.h>

struct isl_constraint;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Set;
  class LocalSpace;
  class Int;
  class Aff;
  class Ctx;
} // namespace isl


namespace isl {
  class Constraint : public Obj<Constraint, struct isl_constraint> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_constraint_free(takeOrNull()); }
    StructTy *addref() const { return isl_constraint_copy(keepOrNull()); }

  public:
    Constraint() : Obj() { }
    static ObjTy enwrap(StructTy *obj) { ObjTy result; result.give(obj); return result; }

    /* implicit */ Constraint(const ObjTy &that) : Obj(that) { }
    /* implicit */ Constraint(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }


    Ctx *getCtx() const { return Ctx::enwrap(isl_constraint_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_constraint_dump(keep()); }
#pragma endregion


#pragma region Creational
    static Constraint createEquality(LocalSpace &&);
    static Constraint createInequality(LocalSpace &&);

    static Constraint createEqualityFromAff(Aff &&aff) { return enwrap(isl_equality_from_aff(aff.take())); }
    static Constraint createInequalityFromAff(Aff &&aff) { return enwrap(isl_inequality_from_aff(aff.take())); }

    static Constraint createEq(Aff &&lhs, Aff &&rhs) {
      auto term = isl_aff_sub(lhs.take(), rhs.take());
      auto c = isl_equality_from_aff(term);
      return enwrap(c);
    }

    static Constraint createGe(Aff &&lhs, Aff &&rhs) {   // lhs >= rhs
      auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs >= 0
      auto c = isl_inequality_from_aff(term); // TODO: Confirm
      return enwrap(c);
    }
    static Constraint createLe(Aff &&lhs, Aff &&rhs) { return createGe(std::move(rhs), std::move(rhs)); }

    static Constraint createGt(Aff &&lhs, Aff &&rhs) {  // lhs > rhs
      auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs > 0
      term = isl_aff_add_constant_si(term, -1); // lhs - rhs - 1 >= 0
      auto c = isl_inequality_from_aff(term);
      return enwrap(c);
    }
    static Constraint createLt(Aff &&lhs, Aff &&rhs) { return createGt(lhs.move(), rhs.move()); }
#pragma endregion


#pragma region Conversion
BasicSet toBasicSet() const;
#pragma endregion


#pragma region Printing
    //void print(llvm::raw_ostream &out) const;
    //std::string toString() const;
    //void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion

    //Ctx *getCtx() const;
    LocalSpace getLocalSpace() const;
    void setConstant_inplace(const Int &v) ISLPP_INPLACE_FUNCTION;
    void setConstant_inplace(int v) ISLPP_INPLACE_FUNCTION;
    void setCoefficient_inplace(isl_dim_type type, int pos, const Int & v) ISLPP_INPLACE_FUNCTION;
    void setCoefficient_inplace(isl_dim_type type, int pos, int v) ISLPP_INPLACE_FUNCTION;

    bool isEquality() const;
    bool isLowerBound(isl_dim_type type, unsigned pos) const;
    bool isUpperBound(isl_dim_type type, unsigned pos) const;
    Int getConstant() const;
    Int getCoefficient(isl_dim_type type, int pos) const;
    bool involvesDims(isl_dim_type type, unsigned first, unsigned n) const;
    Aff getDiv(int pos) const;
    const char *getDimName(isl_dim_type type, unsigned pos) const;
    Aff getBound(isl_dim_type type, int pos) const;
    Aff getAff() const;

#if 0
#pragma region Construction helpers
    Constraint term(isl_dim_type type, int pos, int v) { return wrap(isl_constraint_set_coefficient_si(takeCopy(), type, pos, v)); }
    Constraint term(int v) {  return wrap(isl_constraint_set_constant_si(takeCopy(), v)); }

    Constraint eq() {
      auto aff = isl_constraint_get_aff(takeCopy());
      aff = isl_aff_neg(aff);
      auto eq = isl_equality_from_aff(aff);
      return wrap(eq);
    }
    Constraint lt() {
      auto aff = isl_constraint_get_aff(takeCopy());
    }

#pragma endregion
#endif
  }; // class Constraint


  static inline Constraint setConstant(Constraint &&c, const Int &v) {
    return Constraint::enwrap(isl_constraint_set_constant(c.take(), v.keep()));
  }
  static inline Constraint setCoefficient(Constraint &&c,isl_dim_type type, int pos, const Int &v) {
    return Constraint::enwrap(isl_constraint_set_coefficient(c.take(),type,pos, v.keep()));
  }


  Constraint makeLtConstaint(const Constraint &lhs, const Constraint &rhs) ;

  Constraint makeLrConstaint(const Constraint &lhs, const Constraint &rhs);

  static inline Constraint makeEqConstraint(Constraint &&lhs, const Constraint &rhs) {
    assert(lhs.isEquality());
    assert(rhs.isEquality());
    assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
    auto space = lhs.getLocalSpace();

    auto result = lhs.move();

    auto c = result.getConstant() - rhs.getConstant();
    result = setConstant(std::move(result), std::move(c)); 

    for (auto it = space.dim_begin(), end = space.dim_end(); it!=end; ++it) {
      auto dim = *it;
      auto type = dim.getType();
      auto pos = dim.getPos();

      auto coeff = result.getCoefficient(type, pos) - rhs.getCoefficient(type, pos);
      result = setCoefficient(std::move(result), type, pos, std::move(coeff)); 
    }

    return result;
  }


  static inline Constraint makeEqConstraint(Constraint &&lhs, int rhs) {
    assert(lhs.isEquality());
    assert(lhs.getConstant() == 0);
    auto result = isl_constraint_set_constant_si(lhs.take(), rhs);
    return Constraint::enwrap(result);
  }


  static inline Constraint makeGeConstraint(const Constraint &lhs, const Constraint &rhs) {
    assert(lhs.isEquality());
    assert(rhs.isEquality());
    assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
    auto space = lhs.getLocalSpace();

    auto result = space.createInequalityConstraint();

    auto c = lhs.getConstant() - rhs.getConstant();
    result = setConstant(std::move(result), std::move(c)); 

    for (auto it = space.dim_begin(), end = space.dim_end(); it!=end; ++it) {
      auto dim = *it;
      auto type = dim.getType();
      auto pos = dim.getPos();

      auto coeff = lhs.getCoefficient(type, pos) - rhs.getCoefficient(type, pos); // TODO: Confirm order
      result = setCoefficient(std::move(result), type, pos, std::move(coeff)); 
    }

    return result;
  }


  static inline Constraint makeGtConstraint(const Constraint &lhs, const Constraint &rhs) {
    assert(lhs.isEquality());
    assert(rhs.isEquality());
    assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
    auto space = lhs.getLocalSpace();

    auto result = space.createInequalityConstraint();

    auto c = lhs.getConstant() - rhs.getConstant();
    result = setConstant(std::move(result), std::move(c)); 

    for (auto it = space.dim_begin(), end = space.dim_end(); it!=end; ++it) {
      auto dim = *it;
      auto type = dim.getType();
      auto pos = dim.getPos();

      auto coeff = lhs.getCoefficient(type, pos) - rhs.getCoefficient(type, pos); // TODO: Confirm order
      result = setCoefficient(std::move(result), type, pos, std::move(coeff)); 
    }

    return result;
  }

} // namespace isl
#endif /* ISLPP_CONSTRAINT_H */
