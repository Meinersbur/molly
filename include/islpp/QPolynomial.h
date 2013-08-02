#ifndef ISLPP_QPOLYNOMIAL_H
#define ISLPP_QPOLYNOMIAL_H

#include <cassert>

struct isl_qpolynomial;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class QPolynomial {
#pragma region Low-level
  private:
    isl_qpolynomial *poly;

  public: // Public because otherwise we had to add a lot of friends
    isl_qpolynomial *take() { assert(poly); isl_qpolynomial *result = poly; poly = nullptr; return result; }
    isl_qpolynomial *takeCopy() const;
    isl_qpolynomial *keep() const { return poly; }
  protected:
    void give(isl_qpolynomial *poly);

    explicit QPolynomial(isl_qpolynomial *poly) : poly(poly) { }
  public:
    static QPolynomial wrap(isl_qpolynomial *poly) { return QPolynomial(poly); }
#pragma endregion

  public:
    QPolynomial(void) : poly(nullptr) {}
    /* implicit */ QPolynomial(const QPolynomial &that) : poly(that.takeCopy()) {}
    /* implicit */ QPolynomial(QPolynomial &&that) : poly(that.take()) { }
    ~QPolynomial(void);

    const QPolynomial &operator=(const QPolynomial &that) { give(that.takeCopy()); return *this; }
    const QPolynomial &operator=(QPolynomial &&that) { give(that.take()); return *this; }

  }; // class QPolynomial
} // namespace isl
#endif /* ISLPP_QPOLYNOMIAL_H */
