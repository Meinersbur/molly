#ifndef ISLPP_MAPSPACE_H
#define ISLPP_MAPSPACE_H

#include "islpp_common.h"
#include "Obj.h"
#include "MapSpacelike.h"

#include <isl/space.h>


namespace isl {

  class MapSpace : /*public Obj<MapSpace, isl_space>,*/ public Space,  public MapSpacelike<MapSpace> {

#pragma region isl::Obj
    friend class isl::Obj<ObjTy, StructTy>;
  protected:
    //void release() { isl_space_free(takeOrNull()); }
    //StructTy *addref() const { return isl_space_copy(keepOrNull()); }

  public:
    MapSpace() { }

    /* implicit */ MapSpace(const ObjTy &that) : Space(that) { }
    /* implicit */ MapSpace(ObjTy &&that) : Space(std::move(that)) { }
    //const ObjTy &operator=(const ObjTy &that) { obj_reset(that); return *this; }
    //const ObjTy &operator=(ObjTy &&that) { obj_reset(std::move(that)); return *this; }

    //ISLPP_PROJECTION_ATTRS Ctx *getCtx() ISLPP_PROJECTION_FUNCTION;
    //void print(llvm::raw_ostream &out) const;
    //void dump() const;
#pragma endregion

    static MapSpace enwrap(__isl_take StructTy *obj) {
      MapSpace result;
      result.reset(obj);
      return result; // NRVO
    }

    static MapSpace enwrapCopy(__isl_keep StructTy *obj) {
      MapSpace result;
      result.reset(obj);
      // A temporary obj such that we can call its takeCopyOrNull method, no need to require implementors to implement a second version of it
      //result.obj = result.takeCopyOrNull();
      result.addref();
      return result; // NRVO
    }

  }; // class MapSpace

} // namespace molly

#endif /* ISLPP_MAPSPACE_H */
