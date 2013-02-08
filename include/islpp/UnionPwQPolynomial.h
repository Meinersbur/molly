#ifndef ISLPP_UNIONPWQPOLYNOMIAL_H
#define ISLPP_UNIONPWQPOLYNOMIAL_H

#include <cassert>

struct isl_union_pw_qpolynomial;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class UnionPwQPolynomial {
#pragma region Low-level
  private:
    isl_union_pw_qpolynomial *poly;

  public: // Public because otherwise we had to add a lot of friends
    isl_union_pw_qpolynomial *take() { assert(poly); isl_union_pw_qpolynomial *result = poly; poly = nullptr; return result; }
    isl_union_pw_qpolynomial *takeCopy() const;
    isl_union_pw_qpolynomial *keep() const { return poly; }
  protected:
    void give(isl_union_pw_qpolynomial *poly);

    explicit UnionPwQPolynomial(isl_union_pw_qpolynomial *poly) : poly(poly) { }
  public:
    static UnionPwQPolynomial wrap(isl_union_pw_qpolynomial *poly) { return UnionPwQPolynomial(poly); }
#pragma endregion

  public:
    UnionPwQPolynomial(void) : poly(nullptr) {}
    /* implicit */ UnionPwQPolynomial(const UnionPwQPolynomial &that) : poly(that.takeCopy()) {}
    /* implicit */ UnionPwQPolynomial(UnionPwQPolynomial &&that) : poly(that.take()) { }
    ~UnionPwQPolynomial(void);

    const UnionPwQPolynomial &operator=(const UnionPwQPolynomial &that) { give(that.takeCopy()); return *this; }
    const UnionPwQPolynomial &operator=(UnionPwQPolynomial &&that) { give(that.take()); return *this; }

  }; // class UnionPwQPolynomial
} // namespace isl
#endif /* ISLPP_UNIONPWQPOLYNOMIAL_H */