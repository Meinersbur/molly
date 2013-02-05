#ifndef ISLPP_MAP_H
#define ISLPP_MAP_H

#include <cassert>

struct isl_map;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  class Map {
#pragma region Low-level
      private:
    isl_map *map;

  public: // Public because otherwise we had to add a lot of friends
    isl_map *take() { assert(map); isl_map *result = map; map = nullptr; return result; }
    isl_map *takeCopy() const;
    isl_map *keep() const { return map; }
  protected:
    void give(isl_map *map);

    explicit Map(isl_map *map) : map(map) { }
  public:
    static Map wrap(isl_map *map) { return Map(map); }
#pragma endregion

  public:
        Map(void) : map(nullptr) {}
    Map(const Map &that) : map(that.takeCopy()) {}
    Map(Map &&that) : map(that.take()) { }
    ~Map(void);

        const Map &operator=(const Map &that) { give(that.takeCopy()); return *this; }
    const Map &operator=(Map &&that) { give(that.take()); return *this; }

  }; // class Map
} // namespace isl
#endif /* ISLPP_MAP_H */