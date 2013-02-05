#ifndef ISLPP_CONSTRAINT_H
#define ISLPP_CONSTRAINT_H

#include <isl/constraint.h>

#include <llvm/Support/Compiler.h>
#include <assert.h>

struct isl_constraint;

namespace isl {
  class Set;
  class LocalSpace;
} // naespace isl


namespace isl {
  class Constraint {
    friend class BasicSet;
    //friend Set addContraint(Set &&, Constraint &&);

  private:
     isl_constraint *constraint;

    Constraint(const Constraint &) LLVM_DELETED_FUNCTION;
    const Constraint &operator=(const Constraint &) LLVM_DELETED_FUNCTION;



  protected:
    explicit Constraint(isl_constraint *constraint);

    static Constraint wrap(isl_constraint *constraint);

  public:
        isl_constraint *take() { assert(constraint); isl_constraint *result = constraint; constraint = nullptr; return result; }
    isl_constraint *keep() { return constraint; }
    void give(isl_constraint *constraint) { assert(!this->constraint); this->constraint = constraint; }

  public:
        Constraint(Constraint &&that) { this->constraint = that.take(); }
        ~Constraint();

    static Constraint createEquality(LocalSpace &&);
    static Constraint createInequality(LocalSpace &&);

    void setCoefficient(enum isl_dim_type type, int pos, int v);
    void setConstant(int v);
  };

} // namespace isl
#endif /* ISLPP_CONSTRAINT_H */