#ifndef ISLPP_UNIONSET_H
#define ISLPP_UNIONSET_H

#include <cassert>

struct isl_union_set;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class UnionSet {
#pragma region Low-level
  private:
    isl_union_set *set;

  public: // Public because otherwise we had to add a lot of friends
    isl_union_set *take() { assert(set); isl_union_set *result = set; set = nullptr; return result; }
    isl_union_set *takeCopy() const;
    isl_union_set *keep() const { return set; }
  protected:
    void give(isl_union_set *aff);

  public:
    static UnionSet wrap(isl_union_set *set) { UnionSet result; result.give(set); return result; }
#pragma endregion

  public:
    UnionSet(void) : set(nullptr) {}
    /* implicit */ UnionSet(const UnionSet &that) : set(that.takeCopy()) {}
    /* implicit */ UnionSet(UnionSet &&that) : set(that.take()) { }
    ~UnionSet(void);

    const UnionSet &operator=(const UnionSet &that) { give(that.takeCopy()); return *this; }
    const UnionSet &operator=(UnionSet &&that) { give(that.take()); return *this; }

  }; // class UnionSet
} // namespace isl
#endif /* ISLPP_UNIONSET_H */