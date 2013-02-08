#ifndef ISLPP_PWAFF_H
#define ISLPP_PWAFF_H

#include <cassert>

struct isl_pw_aff;

namespace llvm {
} // namespace llvm

namespace isl {

} // namespace isl


namespace isl {
  class PwAff {
#pragma region Low-level
  private:
    isl_pw_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_pw_aff *take() { assert(aff); isl_pw_aff *result = aff; aff = nullptr; return result; }
    isl_pw_aff *takeCopy() const;
    isl_pw_aff *keep() const { return aff; }
  protected:
    void give(isl_pw_aff *aff);

    explicit PwAff(isl_pw_aff *aff) : aff(aff) { }
  public:
    static PwAff wrap(isl_pw_aff *aff) { return PwAff(aff); }
#pragma endregion

  public:
    PwAff(void) : aff(nullptr) {}
    PwAff(const PwAff &that) : aff(that.takeCopy()) {}
    PwAff(PwAff &&that) : aff(that.take()) { }
    ~PwAff(void);

    const PwAff &operator=(const PwAff &that) { give(that.takeCopy()); return *this; }
    const PwAff &operator=(PwAff &&that) { give(that.take()); return *this; }

  }; // class PwAff
} // namespace isl
#endif /* ISLPP_PWAFF_H */