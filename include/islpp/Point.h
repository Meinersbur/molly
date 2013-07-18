#ifndef ISLPP_POINT_H
#define ISLPP_POINT_H

#include "islpp_common.h"
#include <cassert>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/point.h>
#include "Int.h"
#include "Obj.h"
#include "Ctx.h"

struct isl_point;

namespace llvm {
} // namespace llvm

namespace isl {
  class Space;
  class Ctx;
  class Int;
  class Set;
  class BasicSet;
  class Set;
  class Map;
  class BasicSet;
} // namespace isl


namespace isl {

  class Point : public Obj3<Point, isl_point> {

#pragma region isl::Obj3
    friend class isl::Obj3<ObjTy, StructTy>;
  protected:
    void release() { isl_point_free(takeOrNull()); }

  public:
    Point() { }

    /* implicit */ Point(ObjTy &&that) : Obj3(std::move(that)) { }
    /* implicit */ Point(const ObjTy &that) : Obj3(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

  public:
    StructTy *takeCopyOrNull() const { return isl_point_copy(keepOrNull()); }

    Ctx *getCtx() const { return Ctx::wrap(isl_point_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void dump() const { isl_point_dump(keep()); }
#pragma endregion


#if 0
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
#endif


#pragma region Creational
    static Point createZero(Space &&space);

    //Point copy() const { return Point::wrap(takeCopy()); }
#pragma endregion


#pragma region Conversion
    /// to singleton BasicSet
    BasicSet toBasicSet() const;
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
      BasicSet toBasicSet() &&;
#endif

    /// to singleton Set
    Set toSet() const;
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
      BasicSet toSet() &&;
#endif
#pragma endregion


    Space getSpace() const;
    //Ctx *getCtx() const;

    bool isVoid() const;

    Int getCoordinate(isl_dim_type type, int pos) const;
    void setCoordinate(isl_dim_type type, int pos, const Int &v);
    void add(isl_dim_type type, int pos, unsigned val);
    void sub(isl_dim_type type, int pos, unsigned val);

    Set apply(const Map &map) const;
  }; // class Point


  static inline Point enwrap(isl_point *obj) { return Point::enwrap(obj); }

  static inline Point setCoordinate(Point &&point, isl_dim_type type, int pos, const Int &val) { return Point::enwrap(isl_point_set_coordinate(point.take(), type, pos, val.keep())); }
  static inline Point setCoordinate(const Point &point, isl_dim_type type, int pos, const Int &val) { return Point::enwrap(isl_point_set_coordinate(point.takeCopy(), type, pos, val.keep())); }

} // namespace isl
#endif /* ISLPP_POINT_H */
