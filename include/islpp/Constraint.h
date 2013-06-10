#ifndef ISLPP_CONSTRAINT_H
#define ISLPP_CONSTRAINT_H

#include "islpp_common.h"
#include <cassert>
#include <string>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/constraint.h>
#include <isl/aff.h>
#include "Aff.h"

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
  class Constraint final {
#pragma region Low-level
  private:
    isl_constraint *constraint;

  public: // Public because otherwise we had to add a lot of friends
    isl_constraint *take() { assert(constraint); isl_constraint *result = constraint; constraint = nullptr; return result; }
    isl_constraint *takeCopy() const;
    isl_constraint *keep() const { return constraint; }
  protected:
    void give(isl_constraint *constraint);

  public:
    static Constraint wrap(isl_constraint *constraint) { Constraint result; result.give(constraint); return result; }
#pragma endregion

  public:
    Constraint() : constraint(nullptr) {}
    Constraint(const Constraint &that) : constraint(that.takeCopy()) {}
    Constraint(Constraint &&that) : constraint(that.take()) { }
    ~Constraint();

    const Constraint &operator=(const Constraint &that) { give(that.takeCopy()); return *this; }
    const Constraint &operator=(Constraint &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Constraint createEquality(LocalSpace &&);
    static Constraint createInequality(LocalSpace &&);

    static Constraint createEqualityFromAff(Aff &&aff) { return wrap(isl_equality_from_aff(aff.take())); }
    static Constraint createInequalityFromAff(Aff &&aff) { return wrap(isl_inequality_from_aff(aff.take())); }

    static Constraint createEq(Aff &&lhs, Aff &&rhs) {
      auto term = isl_aff_sub(lhs.take(), rhs.take());
      auto c = isl_equality_from_aff(term);
      return wrap(c);
    }

    static Constraint createGe(Aff &&lhs, Aff &&rhs) {   // lhs >= rhs
       auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs >= 0
       auto c = isl_inequality_from_aff(term); // TODO: Confirm
      return wrap(c);
    }
    static Constraint createLe(Aff &&lhs, Aff &&rhs) { return createGe(std::move(rhs), std::move(rhs)); }

    static Constraint createGt(Aff &&lhs, Aff &&rhs) {  // lhs > rhs
      auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs > 0
      term = isl_aff_add_constant_si(term, -1); // lhs - rhs - 1 >= 0
      auto c = isl_inequality_from_aff(term);
      return wrap(c);
    }
    static Constraint createLt(Aff &&lhs, Aff &&rhs) { return createGt(lhs.move(), rhs.move()); }

#pragma endregion

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
    void printProperties(llvm::raw_ostream &out, int depth = 1, int indent = 0) const;
#pragma endregion

    Ctx *getCtx() const;
    LocalSpace getLocalSpace() const;
    void setConstant(const Int &v);
    void setConstant(int v);
    void setCoefficient(isl_dim_type type, int pos, const Int & v);
    void setCoefficient(isl_dim_type type, int pos, int v);

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
#pragma region Cunstruction helpers
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
} // namespace isl
#endif /* ISLPP_CONSTRAINT_H */
