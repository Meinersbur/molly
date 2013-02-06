#ifndef ISLPP_AFF_H
#define ISLPP_AFF_H

#include <cassert>

struct isl_aff;

namespace llvm {
} // namespace llvm

namespace isl {

} // namespace isl


namespace isl {

  class Aff {
#pragma region Low-level
      private:
    isl_aff *aff;

  public: // Public because otherwise we had to add a lot of friends
    isl_aff *take() { assert(aff); isl_aff *result = aff; aff = nullptr; return result; }
    isl_aff *takeCopy() const;
    isl_aff *keep() const { return aff; }
  protected:
    void give(isl_aff *aff);

    explicit Aff(isl_aff *aff) : aff(aff) { }
  public:
    static Aff wrap(isl_aff *aff) { return Aff(aff); }
#pragma endregion

  public:
        Aff(void) : aff(nullptr) {}
     /* implicit */ Aff(const Aff &that) : aff(that.takeCopy()) {}
     /* implicit */ Aff(Aff &&that) : aff(that.take()) { }
    ~Aff(void);

        const Aff &operator=(const Aff &that) { give(that.takeCopy()); return *this; }
    const Aff &operator=(Aff &&that) { give(that.take()); return *this; }

  }; // class Aff
} // namespace isl
#endif /* ISLPP_AFF_H */