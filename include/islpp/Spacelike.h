#ifndef ISLPP_SPACELIKE_H
#define ISLPP_SPACELIKE_H

#include <isl/space.h> // enum isl_dim_type
#include "Obj.h" // class Obj (base of Map)

namespace isl {
  class Space;
  class Id;
  class Dim;
} // namespace isl


namespace isl {
  class Spacelike : public Obj {
  protected:
    bool findDim(const Dim &dim, isl_dim_type &type, unsigned &pos);
  public:
    virtual ~Spacelike() {}

    virtual Space getSpace() const = 0;

    virtual unsigned dim(isl_dim_type type) const = 0;    
    unsigned dimParam() const { return dim(isl_dim_param); }

    virtual bool hasTupleId(isl_dim_type type) const { return getTupleId(type).isValid(); } 
    virtual Id getTupleId(isl_dim_type type) const { return getSpace().getTupleId(type); }
    virtual void setTupleId(isl_dim_type type, Id &&id) = 0;

    virtual bool hasTupleName(isl_dim_type type) const { return getTupleName(type); }
    virtual const char *getTupleName(isl_dim_type type) const { return getSpace().getTupleName(type); }
    virtual void setTupleName(isl_dim_type type, const char *s) = 0;

    virtual bool hasDimId(isl_dim_type type, unsigned pos) const { return getDimId(type, pos).isValid(); }
    virtual Id getDimId(isl_dim_type type, unsigned pos) const { return getSpace().getDimId(type, pos); }
    Id getDimIdOrNull(isl_dim_type type, unsigned pos) const;
    virtual void setDimId(isl_dim_type type, unsigned pos, Id &&id) = 0;
    virtual int findDimById(isl_dim_type type, const Id &id) const { return getSpace().findDimById(type, id); } 

    virtual bool hasDimName(isl_dim_type type, unsigned pos) const { return getDimName(type, pos); }
    virtual const char *getDimName(isl_dim_type type, unsigned pos) const { return getSpace().getDimName(type, pos); }
    virtual void setDimName(isl_dim_type type, unsigned pos, const char *s)  = 0;
    virtual int findDimByName(isl_dim_type type, const char *name) const { return getSpace().findDimByName(type, name); }

    virtual void insertDims(isl_dim_type type, unsigned pos, unsigned n) = 0;
    virtual void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) = 0;
    virtual void addDims(isl_dim_type type, unsigned n) = 0;
    Dim addDim(isl_dim_type type);
    Dim addOutDim();

    virtual void removeDims(isl_dim_type type, unsigned first, unsigned n) = 0;
    //virtual void removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) = 0;
  }; // class Spacelike
} // namepspace isl
#endif /* ISLPP_SPACELIKE_H */
