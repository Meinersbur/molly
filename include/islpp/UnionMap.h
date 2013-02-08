#ifndef ISLPP_UNIONMAP_H
#define ISLPP_UNIONMAP_H

#include <cassert>

struct isl_union_map;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class UnionMap {
#pragma region Low-level
  private:
    isl_union_map *map;

  public: // Public because otherwise we had to add a lot of friends
    isl_union_map *take() { assert(map); isl_union_map *result = map; map = nullptr; return result; }
    isl_union_map *takeCopy() const;
    isl_union_map *keep() const { return map; }
  protected:
    void give(isl_union_map *aff);

  public:
    static UnionMap wrap(isl_union_map *map) { UnionMap result; result.give(map); return result; }
#pragma endregion

  public:
    UnionMap(void) : map(nullptr) {}
    /* implicit */ UnionMap(const UnionMap &that) : map(that.takeCopy()) {}
    /* implicit */ UnionMap(UnionMap &&that) : map(that.take()) { }
    ~UnionMap(void);

    const UnionMap &operator=(const UnionMap &that) { give(that.takeCopy()); return *this; }
    const UnionMap &operator=(UnionMap &&that) { give(that.take()); return *this; }

  }; // class UnionMap
} // namespace isl
#endif /* ISLPP_UNIONMAP_H */