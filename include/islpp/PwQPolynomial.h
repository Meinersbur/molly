#ifndef ISLPP_PWQPOLYNOMIAL_H
#define ISLPP_PWQPOLYNOMIAL_H

#include <cassert>

struct isl_pw_qpolynomial;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class PwQPolynomial {
#pragma region Low-level
  private:
    isl_pw_qpolynomial *poly;

  public: // Public because otherwise we had to add a lot of friends
    isl_pw_qpolynomial *take() { assert(poly); isl_pw_qpolynomial *result = poly; poly = nullptr; return result; }
    isl_pw_qpolynomial *takeCopy() const;
    isl_pw_qpolynomial *keep() const { return poly; }
  protected:
    void give(isl_pw_qpolynomial *poly);

    explicit PwQPolynomial(isl_pw_qpolynomial *poly) : poly(poly) { }
  public:
    static PwQPolynomial wrap(isl_pw_qpolynomial *poly) { return PwQPolynomial(poly); }
#pragma endregion

  public:
    PwQPolynomial(void) : poly(nullptr) {}
    PwQPolynomial(const PwQPolynomial &that) : poly(that.takeCopy()) {}
    PwQPolynomial(PwQPolynomial &&that) : poly(that.take()) { }
    ~PwQPolynomial(void);

    const PwQPolynomial &operator=(const PwQPolynomial &that) { give(that.takeCopy()); return *this; }
    const PwQPolynomial &operator=(PwQPolynomial &&that) { give(that.take()); return *this; }

  }; // class PwQPolynomial
} // namespace isl
#endif /* ISLPP_PWQPOLYNOMIAL_H */