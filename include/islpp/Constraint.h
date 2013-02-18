#ifndef ISLPP_CONSTRAINT_H
#define ISLPP_CONSTRAINT_H

#include <cassert>
#include <string>

struct isl_constraint;
enum isl_dim_type; 

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
  class Constraint {
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
    Constraint(void) : constraint(nullptr) {}
    Constraint(const Constraint &that) : constraint(that.takeCopy()) {}
    Constraint(Constraint &&that) : constraint(that.take()) { }
    ~Constraint(void);

    const Constraint &operator=(const Constraint &that) { give(that.takeCopy()); return *this; }
    const Constraint &operator=(Constraint &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Constraint createEquality(LocalSpace &&);
    static Constraint createInequality(LocalSpace &&);
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
  }; // class Constraint
} // namespace isl
#endif /* ISLPP_CONSTRAINT_H */