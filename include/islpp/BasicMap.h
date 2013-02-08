#ifndef ISLPP_BASICMAP_H
#define ISLPP_BASICMAP_H

#include <cassert>

struct isl_basic_map;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {
  class BasicMap {
#pragma region Low-level
  private:
    isl_basic_map *map;

  public: // Public because otherwise we had to add a lot of friends
    isl_basic_map *take() { assert(map); isl_basic_map *result = map; map = nullptr; return result; }
    isl_basic_map *takeCopy() const;
    isl_basic_map *keep() const { return map; }
  protected:
    void give(isl_basic_map *aff);

  public:
    static BasicMap wrap(isl_basic_map *map) { BasicMap result; result.give(map); return result; }
#pragma endregion

  public:
    BasicMap(void) : map(nullptr) {}
    /* implicit */ BasicMap(const BasicMap &that) : map(that.takeCopy()) {}
    /* implicit */ BasicMap(BasicMap &&that) : map(that.take()) { }
    ~BasicMap(void);

    const BasicMap &operator=(const BasicMap &that) { give(that.takeCopy()); return *this; }
    const BasicMap &operator=(BasicMap &&that) { give(that.take()); return *this; }

  }; // class BasicMap
} // namespace isl
#endif /* ISLPP_BASICMAP_H */