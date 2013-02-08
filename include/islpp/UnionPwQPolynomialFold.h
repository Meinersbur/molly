#ifndef ISLPP_UNIONPWQPOLYNOMIALFOLD_H
#define ISLPP_UNIONPWQPOLYNOMIALFOLD_H

#include <cassert>

struct isl_union_pw_qpolynomial_fold;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class UnionPwQPolynomialFold {
#pragma region Low-level
      private:
    isl_union_pw_qpolynomial_fold *poly;

  public: // Public because otherwise we had to add a lot of friends
    isl_union_pw_qpolynomial_fold *take() { assert(poly); isl_union_pw_qpolynomial_fold *result = poly; poly = nullptr; return result; }
    isl_union_pw_qpolynomial_fold *takeCopy() const;
    isl_union_pw_qpolynomial_fold *keep() const { return poly; }
  protected:
    void give(isl_union_pw_qpolynomial_fold *poly);

    explicit UnionPwQPolynomialFold(isl_union_pw_qpolynomial_fold *poly) : poly(poly) { }
  public:
    static UnionPwQPolynomialFold wrap(isl_union_pw_qpolynomial_fold *poly) { return UnionPwQPolynomialFold(poly); }
#pragma endregion

  public:
        UnionPwQPolynomialFold(void) : poly(nullptr) {}
    UnionPwQPolynomialFold(const UnionPwQPolynomialFold &that) : poly(that.takeCopy()) {}
    UnionPwQPolynomialFold(UnionPwQPolynomialFold &&that) : poly(that.take()) { }
    ~UnionPwQPolynomialFold(void);

        const UnionPwQPolynomialFold &operator=(const UnionPwQPolynomialFold &that) { give(that.takeCopy()); return *this; }
    const UnionPwQPolynomialFold &operator=(UnionPwQPolynomialFold &&that) { give(that.take()); return *this; }

  }; // class UnionPwQPolynomialFold
} // namespace isl
#endif /* ISLPP_UNIONPWQPOLYNOMIALFOLD_H */