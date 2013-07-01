#include "islpp_impl_common.h"
#include "islpp/Dim.h"

#include "islpp/Spacelike.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include <isl/space.h>
#include <isl/map.h>
#include <llvm/Support/ErrorHandling.h>

using namespace isl;


     Dim Dim:: enwrap(Space space, isl_dim_type type, unsigned pos) {
      return Dim(space.takeCopy(), type, pos);
    }

         Dim Dim::enwrap(LocalSpace localspace, isl_dim_type type, unsigned pos) {
      return Dim(localspace.takeCopy(), type, pos);
    }

#if 0
Dim Dim::wrap(__isl_keep isl_space *space, isl_dim_type type, unsigned pos) {
  assert(space);

  switch (type) {
  case isl_dim_param:
  case isl_dim_in:
  case isl_dim_out:
    break;
    //TODO: isl_dim_all
  default:
    llvm_unreachable("Unsupported dim");
  }

  Dim result;
  result.type = type;
  result.pos = pos;
  result.id = Id::wrap(isl_space_get_dim_id(space, type, pos));
  result.typeDims = isl_space_dim(space, type);
  return result;
}
#endif

#if 0
Dim Dim::wrap(__isl_keep isl_local_space *space, isl_dim_type type, unsigned pos) {
  assert(space);

  switch (type) {
  case isl_dim_param:
  case isl_dim_in:
  case isl_dim_out:
    break;
        //TODO: isl_dim_all
  default:
    llvm_unreachable("Unsupported dim");
  }

  return Dim(type, pos, Id::enwrap(isl_local_space_get_dim_id(space, type, pos)), isl_local_space_dim(space, type));
}
#endif

#if 0
Dim Dim::wrap(__isl_keep isl_map *map, isl_dim_type type, unsigned pos) { 
  assert(map);

  switch (type) {
  case isl_dim_param:
  case isl_dim_in:
  case isl_dim_out:
    break;
  default:
    llvm_unreachable("Unsupported dim");
  }

  Dim result;
  result.type = type;
  result.pos = pos;
  assert(pos < isl_map_dim(map, type));
  if (isl_map_has_dim_id(map, type, pos)) {
    result.id = Id::wrap(isl_map_get_dim_id(map, type, pos));
  } else {
    result.id = Id();
  }
  result.typeDims = isl_map_dim(map, type);
  return result;
}
#endif

#if 0
 Dim Dim::wrap(const Spacelike &spacelike, isl_dim_type type, unsigned pos) {
  switch (type) {
  case isl_dim_param:
  case isl_dim_in:
  case isl_dim_out:
    break;
        //TODO: isl_dim_all
  default:
    llvm_unreachable("Unsupported dim");
  }
  assert(pos < spacelike.dim(type));

  return Dim(type, pos, spacelike.getDimIdOrNull(type, pos), spacelike.dim(type));
 }
#endif

#if 0
class DimBase : public Dim {
  // Don't add field members or virtual functions, might break eveything!
protected:
  DimBase(isl_dim_type type, unsigned pos) : Dim(type, pos) {}
public:
  // Default implementations
  // redeclare all methods, so derived classes do not call methods of dim again which would cause an infinite recursion
  ~DimBase() {};
  Map getOwnerMap() const { return Map(); }
  bool hasName() const;
  const char * getName() const ;
  bool hasId() const ;
  Id getId() const ;
  unsigned getTypeDimCount() const;
}; // class DimBase


class MapDim : public DimBase {
private:
  isl_map *getOwner() const { auto result = owner.map; assert(result); return result; }



public:
  MapDim(isl_map *map, isl_dim_type type, unsigned pos);

  ~MapDim() {
    free();
  }

  // Because there is no such thing like a member pointer to a destructor
  void free() {
    isl_map_free(getOwner());
    owner.map = nullptr;
  }

  Map getOwnerMap() const {
    return Map::wrap(isl_map_copy(getOwner()));
  }

  bool hasName() const { 
    return isl_map_has_dim_name(getOwner(), type, pos);
  }
  const char *getName() const  {
    return isl_map_get_dim_name(getOwner(), type, pos);
  }
  bool hasId() const {
    return isl_map_has_dim_id(getOwner(), type, pos);
  }
  Id getId() const {
    return Id::wrap(isl_map_get_dim_id(getOwner(), type, pos));
  }
  unsigned getTypeDimCount() const {
    return isl_map_dim(getOwner(), type);
  }
}; // class MapDim




#pragma region Impl1
namespace isl {
  class DimImpl1 {
  protected: 
    typedef void (Dim::*FuncFree_t)();
    typedef Map (Dim::*FuncGetOwnerMap_t)() const;
    typedef bool (Dim::*FuncHasName_t)() const;
    typedef const char *(Dim::*FuncGetName_t)() const;
    typedef bool (Dim::*FuncHasId_t)() const;
    typedef Id (Dim::*FuncGetId_t)() const;
    typedef unsigned (Dim::*FuncGetTypeDimCount_t)() const;
    // The vtable
    FuncFree_t vptr_free;
    FuncGetOwnerMap_t vptr_getOwnerMap;
    FuncHasName_t vptr_hasName;
    FuncGetName_t vptr_getName;
    FuncHasId_t vptr_hasId;
    FuncGetId_t vptr_getId;
    FuncGetTypeDimCount_t vptr_getTypeDimCount;
  public: 
    void free(Dim *self) { return (self->*vptr_free)(); }
    Map getOwnerMap(const Dim *self) { return (self->*vptr_getOwnerMap)(); }
    bool hasName(const Dim *self) { return (self->*vptr_hasName)(); }
    const char *getName(const Dim *self) { return (self->*vptr_getName)(); }
    bool hasId(const Dim *self) { return (self->*vptr_hasId)(); }
    Id getId(const Dim *self) { return (self->*vptr_getId)(); }
    unsigned getTypeDimCount(const Dim *self) { return (self->*vptr_getTypeDimCount)(); }
  }; // DimImpl
} // namespace isl

template<typename T>
class DimImpl1Impl : public DimImpl1 {
public:
  DimImpl1Impl() {
    vptr_free = static_cast<FuncFree_t>(&T::free);
    vptr_getOwnerMap = static_cast<FuncGetOwnerMap_t>(&T::getOwnerMap);
    vptr_hasName = static_cast<FuncHasName_t>(&T::hasName);
    vptr_getName = static_cast<FuncGetName_t>(&T::getName);
    vptr_hasId = static_cast<FuncHasId_t>(&T::hasId);
    vptr_getId = static_cast<FuncGetId_t>(&T::getId);
    vptr_getTypeDimCount = static_cast<FuncGetTypeDimCount_t>(&T::getTypeDimCount);
  }
}; // class DimImpl1Impl

static DimImpl1Impl<MapDim> MapDim_vtable1; // Singleton 
#pragma endregion 


#pragma region Impl2
namespace isl {
  class DimImpl2 {
    // using the vtable of derived classes
  public:
    virtual void free(Dim *self) = 0;
    virtual Map getOwnerMap(const Dim *self) = 0;
    virtual bool hasName(const Dim *self) = 0;
    virtual const char *getName(const Dim *self) = 0;
    virtual bool hasId(const Dim *self) = 0;
    virtual Id getId(const Dim *self) = 0;
    virtual unsigned getTypeDimCount(const Dim *self) = 0;
  }; // class DimImpl
} // namespace isl

template<typename T>
class DimImpl2Impl : DimImpl2 {
public:
  virtual void free(Dim *self) override { return static_cast<T*>(self)->~T(); }
  virtual Map getOwnerMap(const Dim *self) override { return static_cast<const T*>(self)->getOwnerMap(); }
  virtual bool hasName(const Dim *self) override { return static_cast<const T*>(self)->hasName(); }
  virtual const char *getName(const Dim *self) override {  return static_cast<const T*>(self)->getName(); } 
  virtual bool hasId(const Dim *self) override { return static_cast<const T*>(self)->hasName(); }
  virtual Id getId(const Dim *self) override {  return static_cast<const T*>(self)->getId(); } 
  virtual unsigned getTypeDimCount(const Dim *self) override {  return static_cast<const T*>(self)->getTypeDimCount(); } 
}; // class DimImpl2Impl

static DimImpl2Impl<MapDim> MapDim_vtable2; // Singleton 
#pragma endregion


MapDim:: MapDim(isl_map *map, isl_dim_type type, unsigned pos) : DimBase(type, pos) {
  this->pimpl = &MapDim_vtable1;
  this->owner.map = map;
}
Dim Dim::wrap(isl_map *map, isl_dim_type type, unsigned pos) {
  Dim result;
  new (&result) MapDim(map, type, pos);  
  return result;
}

Dim::~Dim() {
  pimpl->free(this);
}
Map Dim::getOwnerMap() const {
  return pimpl->getOwnerMap(this);
}
bool Dim::hasName() const {
  return pimpl->hasName(this);
}
const char *Dim::getName() const {
  return pimpl->getName(this);
}
bool Dim::hasId() const {
  return pimpl->hasId(this);
}
Id Dim::getId() const {
  return pimpl->getId(this);
}
unsigned Dim::getTypeDimCount() const {
  return pimpl->getTypeDimCount(this);
}
#endif

