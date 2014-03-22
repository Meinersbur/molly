#ifndef ISLPP_POINT_H
#define ISLPP_POINT_H

#include "islpp_common.h"
#include <cassert>
#include <isl/space.h> // enum isl_dim_type;
#include <isl/point.h>
#include "Int.h"
#include "Obj.h"
#include "Ctx.h"
#include <isl/deprecated/point_int.h>

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

  class Point : public Obj<Point, isl_point> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_point_free(takeOrNull()); }
    StructTy *addref() const { return isl_point_copy(keepOrNull()); }

  public:
    Point() { }

    /* implicit */ Point(ObjTy &&that) : Obj(std::move(that)) { }
    /* implicit */ Point(const ObjTy &that) : Obj(that) { }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }

    Ctx *getCtx() const { return Ctx::enwrap(isl_point_get_ctx(keep())); }
    void print(llvm::raw_ostream &out) const;
    void printFormatted(llvm::raw_ostream &out, bool printTupleNames= true) const;
    void dump() const { isl_point_dump(keep()); }
#pragma endregion


#pragma region Creational
    static Point createZero(Space space);
#pragma endregion


#pragma region Conversion
    /// to singleton BasicSet
    BasicSet toBasicSet() const;
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
      BasicSet toBasicSet() &&;
#endif

    /// to singleton Set
    Set toSet() const;
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
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

  static inline llvm::raw_ostream & operator<<(llvm::raw_ostream &os, const Point &p) {
    p.print(os);
    return os;
  }

} // namespace isl
#endif /* ISLPP_POINT_H */
