#ifndef ISLPP_MULTIPWAFF_H
#define ISLPP_MULTIPWAFF_H

#include <cassert>

struct isl_multi_pw_aff;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  class MultiPwAff {
#pragma region Low-level
      private:
    isl_multi_pw_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_multi_pw_aff *take() { assert(aff); isl_multi_pw_aff *result = aff; aff = nullptr; return result; }
    isl_multi_pw_aff *takeCopy() const;
    isl_multi_pw_aff *keep() const { return aff; }
  protected:
    void give(isl_multi_pw_aff *aff);

    explicit MultiPwAff(isl_multi_pw_aff *aff) : aff(aff) { }
  public:
    static MultiPwAff wrap(isl_multi_pw_aff *aff) { return MultiPwAff(aff); }
#pragma endregion

  public:
        MultiPwAff(void) : aff(nullptr) {}
    MultiPwAff(const MultiPwAff &that) : aff(that.takeCopy()) {}
    MultiPwAff(MultiPwAff &&that) : aff(that.take()) { }
    ~MultiPwAff(void);

        const MultiPwAff &operator=(const MultiPwAff &that) { give(that.takeCopy()); return *this; }
    const MultiPwAff &operator=(MultiPwAff &&that) { give(that.take()); return *this; }

  }; // class MultiPwAff
} // namespace isl
#endif /* ISLPP_MULTIPWAFF_H */