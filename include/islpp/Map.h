#ifndef ISLPP_MAP_H
#define ISLPP_MAP_H

#include <cassert>
#include <string>

struct isl_map;

namespace llvm {
  class raw_ostream;
} // namespace llvm

namespace isl {
  class Ctx;
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

  public:
    static Map wrap(isl_map *map) { Map result; result.give(map);  return result; }
#pragma endregion

  public:
    Map() : map(nullptr) {}
    Map(const Map &that) : map(that.takeCopy()) {}
    Map(Map &&that) : map(that.take()) { }
    ~Map() { give(nullptr); }

    const Map &operator=(const Map &that) { give(that.takeCopy()); return *this; }
    const Map &operator=(Map &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Map readFrom(Ctx *ctx, const char *str);
#pragma endregion

#pragma region Printing
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;
#pragma endregion

    Ctx *getCtx() const;

    bool isEmpty() const;

  }; // class Map
} // namespace isl
#endif /* ISLPP_MAP_H */