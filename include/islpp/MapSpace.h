#ifndef ISLPP_MAPSPACE_H
#define ISLPP_MAPSPACE_H

#include "islpp_common.h"
#include "Obj.h"
#include "MapSpacelike.h"

#include <isl/space.h>


namespace isl {

  class MapSpace : public Obj<MapSpace, isl_space>, public MapSpacelike<MapSpace> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    void release() { isl_space_free(takeOrNull()); }
    StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    MapSpace() { }

    /* implicit */ MapSpace(const ObjTy &that) : Obj(that) { }
    /* implicit */ MapSpace(ObjTy &&that) : Obj(std::move(that)) { }
    const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    ISLPP_PROJECTION_ATTRS Ctx *getCtx() ISLPP_PROJECTION_FUNCTION;
    void print(llvm::raw_ostream &out) const;
    void dump() const;
#pragma endregion

#pragma region isl::MapSpacelike
    ISLPP_PROJECTION_ATTRS MapSpace getSpace() ISLPP_PROJECTION_FUNCTION{ return *this; }
    ISLPP_PROJECTION_ATTRS MapSpace getSpacelike() ISLPP_PROJECTION_FUNCTION{ return *this; }

    ISLPP_PROJECTION_ATTRS count_t dim(isl_dim_type type) ISLPP_PROJECTION_FUNCTION{ return isl_space_dim(keep(), type); }
#pragma endregion

  public:
    operator Space() const;

  }; // class MapSpace

} // namespace molly

#endif /* ISLPP_MAPSPACE_H */
