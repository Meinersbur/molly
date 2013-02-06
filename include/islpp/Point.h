#ifndef ISLPP_POINT_H
#define ISLPP_POINT_H

#include <cassert>

struct isl_point;

namespace llvm {
} // namespace llvm

namespace isl {
} // namespace isl


namespace isl {

  class Point {
#pragma region Low-level
      private:
    isl_point *point;

  public: // Public because otherwise we had to add a lot of friends
    isl_point *take() { assert(point); isl_point *result = point; point = nullptr; return result; }
    isl_point *takeCopy() const;
    isl_point *keep() const { return point; }
  protected:
    void give(isl_point *point);

    explicit Point(isl_point *point) : point(point) { }
  public:
    static Point wrap(isl_point *point) { return Point(point); }
#pragma endregion

  public:
        Point(void) : point(nullptr) {}
    Point(const Point &that) : point(that.takeCopy()) {}
    Point(Point &&that) : point(that.take()) { }
    ~Point(void);

        const Point &operator=(const Point &that) { give(that.takeCopy()); return *this; }
    const Point &operator=(Point &&that) { give(that.take()); return *this; }

  }; // class Point
} // namespace isl
#endif /* ISLPP_POINT_H */