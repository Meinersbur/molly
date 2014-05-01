/// Similar to isl::Aff, but allows coefficients on all dim types
/// Similar to isl::Constraint, but does not represent an (in-)equality
/// Usually used to construct Constraint

#ifndef ISLPP_EXPR_H
#define ISLPP_EXPR_H

#include "islpp_common.h"
#include "Obj.h"
#include <isl/ctx.h>
#include <isl/space.h>


struct isl_constraint;

namespace isl {
  class ExprImpl;
  class Dim;
  class Int;
  class LocalSpace;
  class Constraint;
} // namespace isl


namespace isl {

  class Expr : public Obj<Expr, ExprImpl> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release();
    StructTy *addref() const;

  public:
    Expr() { }

    /* implicit */ Expr(const ObjTy &that) : Obj(that) { }
    /* implicit */ Expr(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    Ctx *getCtx() const;
    void print(llvm::raw_ostream &out) const;
    //void dump() const;
#pragma endregion


#pragma region Creational
    static Expr createZero(LocalSpace &&space);
    static Expr createConstant(LocalSpace &&space, int);
    static Expr createConstant(LocalSpace &&space, const Int &);
    static Expr createVar(LocalSpace &&space, isl_dim_type type, unsigned pos);
#pragma endregion

    LocalSpace getLocalSpace() const;
    Int getCoefficient(isl_dim_type type, unsigned pos) const;
  //  ISLPP_PROJECTION_ATTRS Int getCoefficient(Dim dim) ISLPP_PROJECTION_FUNCTION{ return Int::enwrap(isl_aff_get_coefficient(takeCopy(), dim.getType(), dim.getPos()); }
    Int getConstant() const;

    Expr &operator-=(const Expr &that);
  }; // class Expr


  Expr add(Expr &&lhs, Expr const &rhs);
  Expr sub(Expr &&lhs, Expr const &rhs);

  static inline Expr operator+(Expr &&lhs, Expr &&rhs) { return isl::add(lhs.move(), rhs.move()); }
  static inline Expr operator-(Expr &&lhs, Expr &&rhs) { return isl::sub(lhs.move(), rhs.move()); }


  Constraint operator==(const Expr &lhs, const Expr &rhs);
  Constraint operator==(const Expr &lhs, int rhs);

} // namespace isl
#endif /* ISLPP_EXPR_H */
