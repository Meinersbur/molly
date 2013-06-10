#ifndef ISLPP_UNIONPWMULTIAFF_H
#define ISLPP_UNIONPWMULTIAFF_H

#include <cassert>

struct isl_union_pw_multi_aff;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class UnionPwMultiAff final {
#pragma region Low-level
  private:
    isl_union_pw_multi_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_union_pw_multi_aff *take() { assert(aff); isl_union_pw_multi_aff *result = aff; aff = nullptr; return result; }
    isl_union_pw_multi_aff *takeCopy() const;
    isl_union_pw_multi_aff *keep() const { return aff; }
  protected:
    void give(isl_union_pw_multi_aff *aff);

    explicit UnionPwMultiAff(isl_union_pw_multi_aff *aff) : aff(aff) { }
  public:
    static UnionPwMultiAff wrap(isl_union_pw_multi_aff *aff) { return UnionPwMultiAff(aff); }
#pragma endregion

  public:
    UnionPwMultiAff(void) : aff(nullptr) {}
    /* implicit */ UnionPwMultiAff(const UnionPwMultiAff &that) : aff(that.takeCopy()) {}
    /* implicit */ UnionPwMultiAff(UnionPwMultiAff &&that) : aff(that.take()) { }
    ~UnionPwMultiAff(void);

    const UnionPwMultiAff &operator=(const UnionPwMultiAff &that) { give(that.takeCopy()); return *this; }
    const UnionPwMultiAff &operator=(UnionPwMultiAff &&that) { give(that.take()); return *this; }

  }; // class UnionPwMultiAff
} // namespace isl
#endif /* ISLPP_UNIONPWMULTIAFF_H */