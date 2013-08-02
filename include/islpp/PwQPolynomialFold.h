#ifndef ISLPP_PWQPOLYNOMIALFOLD_H
#define ISLPP_PWQPOLYNOMIALFOLD_H

#include <cassert>

struct isl_pw_qpolynomial_fold;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class PwQPolynomialFold {
#pragma region Low-level
  private:
    isl_pw_qpolynomial_fold *poly;

  public: // Public because otherwise we had to add a lot of friends
    isl_pw_qpolynomial_fold *take() { assert(poly); isl_pw_qpolynomial_fold *result = poly; poly = nullptr; return result; }
    isl_pw_qpolynomial_fold *takeCopy() const;
    isl_pw_qpolynomial_fold *keep() const { return poly; }
  protected:
    void give(isl_pw_qpolynomial_fold *poly);

    explicit PwQPolynomialFold(isl_pw_qpolynomial_fold *poly) : poly(poly) { }
  public:
    static PwQPolynomialFold wrap(isl_pw_qpolynomial_fold *poly) { return PwQPolynomialFold(poly); }
#pragma endregion

  public:
    PwQPolynomialFold(void) : poly(nullptr) {}
    /* implicit */ PwQPolynomialFold(const PwQPolynomialFold &that) : poly(that.takeCopy()) {}
    /* implicit */ PwQPolynomialFold(PwQPolynomialFold &&that) : poly(that.take()) { }
    ~PwQPolynomialFold(void);

    const PwQPolynomialFold &operator=(const PwQPolynomialFold &that) { give(that.takeCopy()); return *this; }
    const PwQPolynomialFold &operator=(PwQPolynomialFold &&that) { give(that.take()); return *this; }

  }; // class PwQPolynomialFold
} // namespace isl
#endif /* ISLPP_PWQPOLYNOMIALFOLD_H */
