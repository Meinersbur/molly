#ifndef ISLPP_PWMULTIAFF_H
#define ISLPP_PWMULTIAFF_H

#include <cassert>
#include "Pw.h"

struct isl_pw_multi_aff;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  template<>
  class Pw<MultiAff> {
#pragma region Low-level
  private:
    isl_pw_multi_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_pw_multi_aff *take() { assert(aff); isl_pw_multi_aff *result = aff; aff = nullptr; return result; }
    isl_pw_multi_aff *takeCopy() const;
    isl_pw_multi_aff *keep() const { return aff; }
  protected:
    void give(isl_pw_multi_aff *aff);

    explicit Pw(isl_pw_multi_aff *aff) : aff(aff) { }
  public:
    static PwMultiAff wrap(isl_pw_multi_aff *aff) { return PwMultiAff(aff); }
#pragma endregion

  public:
    Pw() : aff(nullptr) {}
    Pw(const PwMultiAff &that) : aff(that.takeCopy()) {}
    Pw(PwMultiAff &&that) : aff(that.take()) { }
    ~Pw();

    const PwMultiAff &operator=(const PwMultiAff &that) { give(that.takeCopy()); return *this; }
    const PwMultiAff &operator=(PwMultiAff &&that) { give(that.take()); return *this; }

  }; // class PwMultiAff
} // namespace isl
#endif /* ISLPP_PWMULTIAFF_H */