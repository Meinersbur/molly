#ifndef ISLPP_POINT_H
#define ISLPP_POINT_H

#include "islpp_common.h"
#include <cassert>
#include <isl/space.h> // enum isl_dim_type;

struct isl_point;

namespace llvm {
} // namespace llvm

namespace isl {
  class Space;
  class Ctx;
  class Int;
} // namespace isl


namespace isl {
  class Point {
#pragma region Low-level
  private:
    isl_point *point;

  public: // Public because otherwise we had to add a lot of friends
    isl_point *take() { assert(point); isl_point *result = point; point = nullptr; return result; }
    isl_point *takeCopy() const;
    isl_point *keep() const { assert(point); return point; }
  protected:
    void give(isl_point *point);
  public:
    static Point wrap(isl_point *point) { Point result; result.give(point); return result; }
#pragma endregion

  public:
    Point(void) : point(nullptr) {}
    Point(const Point &that) : point(that.takeCopy()) {}
    Point(Point &&that) : point(that.take()) { }
    ~Point(void);

    const Point &operator=(const Point &that) { give(that.takeCopy()); return *this; }
    const Point &operator=(Point &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Point createZero(Space &&space);

    Point copy() const { return Point::wrap(takeCopy()); }
#pragma endregion

    Space getSpace() const;
    Ctx *getCtx() const;

    bool isVoid() const;

    Int getCoordinate(isl_dim_type type, int pos) const;
    void setCoordinate(isl_dim_type type, int pos, const Int &v);
    void add(isl_dim_type type, int pos, unsigned val);
    void sub(isl_dim_type type, int pos, unsigned val);
  }; // class Point
} // namespace isl
#endif /* ISLPP_POINT_H */
