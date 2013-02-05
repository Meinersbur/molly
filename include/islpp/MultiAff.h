#ifndef ISLPP_MULTIAFF_H
#define ISLPP_MULTIAFF_H

#include <cassert>

struct isl_multi_aff;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  class MultiAff {
#pragma region Low-level
      private:
    isl_multi_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_multi_aff *take() { assert(aff); isl_multi_aff *result = aff; aff = nullptr; return result; }
    isl_multi_aff *takeCopy() const;
    isl_multi_aff *keep() const { return aff; }
  protected:
    void give(isl_multi_aff *aff);

    explicit MultiAff(isl_multi_aff *aff) : aff(aff) { }
  public:
    static MultiAff wrap(isl_multi_aff *aff) { return MultiAff(aff); }
#pragma endregion

  public:
        MultiAff(void) : aff(nullptr) {}
    MultiAff(const MultiAff &that) : aff(that.takeCopy()) {}
    MultiAff(MultiAff &&that) : aff(that.take()) { }
    ~MultiAff(void);

        const MultiAff &operator=(const MultiAff &that) { give(that.takeCopy()); return *this; }
    const MultiAff &operator=(MultiAff &&that) { give(that.take()); return *this; }

  }; // class MultiAff
} // namespace isl
#endif /* ISLPP_MULTIAFF_H */