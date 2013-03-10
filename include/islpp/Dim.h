#ifndef ISLPP_DIM_H
#define ISLPP_DIM_H

//#include <cassert>
#include <isl/space.h> // enum isl_dim_type

struct isl_space;
struct isl_local_space;
struct isl_map;
struct isl_basic_map;
struct isl_set;
struct isl_basic_set;

namespace isl {
  class Map;
  class Id;
} // namespace isl


namespace isl {
  class DimImpl1;
  class DimImpl2;

  typedef union {
      isl_space *space;
      isl_local_space *lspace;
      isl_map *map;
      isl_basic_map *bmap;
      isl_set *set;
      isl_basic_set *bset;
  } dim_owner_t;

  class Dim {
    friend class DimImpl;
  protected:
    DimImpl1 *pimpl;
    dim_owner_t owner;
    isl_dim_type type;
    unsigned pos;

  private:
    Dim() {}
  protected: 
    Dim(isl_dim_type type, unsigned pos) : type(type), pos(pos) {}

  public:
    //void assertOwner(isl_space *space) const { assert(this->ownerSpace); assert(this->ownerSpace == space); }
    // void assertOwner(isl_local_space *space) const { assert(this->ownerLocalSpace); assert(this->ownerLocalSpace == space); }

    //protected:
    //  explicit Dim(isl_space *ownerSpace, isl_local_space *ownerLocalSpace, isl_dim_type type, int pos) : ownerSpace(ownerSpace), ownerLocalSpace(ownerLocalSpace), type(type), pos(pos) {}

  public:
    //static Dim wrap(isl_space *ownerSpace, isl_dim_type type, int pos) { return Dim(ownerSpace, NULL, type, pos); }
    //static Dim wrap(isl_local_space *ownerSpace, isl_dim_type type, int pos) { return Dim(NULL, ownerSpace, type, pos); }
    static Dim wrap(isl_map *map, isl_dim_type type, unsigned pos);

    ~Dim();

    isl_dim_type getType() const { return type; }
    int getPos() const { return pos; }

    Map getOwnerMap() const;

    bool hasName() const;
    const char *getName() const;
    bool hasId() const;
    Id getId() const;

    unsigned getTypeDimCount() const;

  }; // class Dim
} // namespace isl
#endif