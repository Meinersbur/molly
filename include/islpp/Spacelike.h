#ifndef ISLPP_SPACELIKE_H
#define ISLPP_SPACELIKE_H

#include "islpp_common.h"
#include <isl/space.h> // enum isl_dim_type
#include "Obj.h" // class Obj (base of Map)

namespace isl {
  class Space;
  class Id;
  class Dim;
} // namespace isl


namespace isl {
  template <typename T> // Curiously recursive template pattern
  class Spacelike2 {
  protected :
    virtual T copy() const = 0;
    virtual Ctx *getCtx() const = 0;

  public:
    virtual unsigned dim(isl_dim_type type) const = 0;  
    unsigned getParamDimCount() { return dim(isl_dim_param); }
    unsigned getSetDimCount() { assert(dim(isl_dim_in) == 0 && "Space must be set-like"); return dim(isl_dim_set); }
    unsigned getInDimCount() { assert(dim(isl_dim_in) > 0 && "Space must be map-like"); return dim(isl_dim_in); }
    unsigned getOutDimCount() { assert(dim(isl_dim_in) > 0 && "Space must be map-like"); return dim(isl_dim_out); }

    virtual bool hasTupleId(isl_dim_type type) const { return getTupleId(type).isValid(); }
    virtual Id getTupleId(isl_dim_type type) const = 0;
    virtual void setTupleId_inplace(isl_dim_type type, Id &&id) ISLPP_INPLACE_QUALIFIER = 0;
    void setTupleId_inplace(isl_dim_type type, const Id &id) ISLPP_INPLACE_QUALIFIER { setTupleId_inplace(type, copy(id)) }
    T setTupleId(isl_dim_type type, const Id  &id) const { auto result = copy(); result.setTupleId_inplace(type, copy(id)); return result; }
    T setTupleId(isl_dim_type type,       Id &&id) const { auto result = copy(); result.setTupleId_inplace(type, move(id)); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setTupleId(isl_dim_type type, const Id  &id) && { setTupleId_inplace(type, copy(id)); return *this; }
    T setTupleId(isl_dim_type type,       Id &&id) && { setTupleId_inplace(type, move(id)); return *this; }
#endif

    virtual bool hasTupleName(isl_dim_type type) const { return getTupleName(type); }
    virtual const char *getTupleName(isl_dim_type type) const { return getTupleId(type).getName(); }
    virtual void setTupleName_inplace(isl_dim_type type, const char *s) ISLPP_INPLACE_QUALIFIER { setTupleId_inplace(type, enwrap(isl_id_alloc(getCtx()->keep(), s, nullptr))); }
    T setTupleName(isl_dim_type type, const char *s) const { auto result = copy(); result.setTupleName_inplace(type, s); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setTupleName(isl_dim_type type, const char *s) && { setTupleName_inplace(type, s); return *this; }
#endif

    virtual bool hasDimId(isl_dim_type type, unsigned pos) const { return getDimId(type, pos).isValid(); }
    virtual Id getDimId(isl_dim_type type, unsigned pos) const = 0;
    virtual void setDimId_inplace(isl_dim_type type, unsigned pos, Id &&id) ISLPP_INPLACE_QUALIFIER = 0;
    void setDimId_inplace(isl_dim_type type, unsigned pos, const Id &id) ISLPP_INPLACE_QUALIFIER { setDimId_inplace(type, pos, copy(id)); }
    T setDimId(isl_dim_type type, unsigned pos,const Id  &id) const { auto result = copy(); result.setDimId_inplace(type, pos, copy(id)); return result; }
    T setDimId(isl_dim_type type, unsigned pos,      Id &&id) const { auto result = copy(); result.setDimId_inplace(type, pos, move(id)); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setDimId(isl_dim_type type, unsigned pos,const Id  &id) && { setDimId_inplace(type, pos, copy(id)); return *this; }
    T setDimId(isl_dim_type type, unsigned pos,      Id &&id) && { setDimId_inplace(type, pos, move(id)); return *this; }
#endif

    virtual bool hasDimName(isl_dim_type type, unsigned pos) const { return getDimName(type, pos); }
    virtual const char *getDimName(isl_dim_type type, unsigned pos) const { return getDimId(type, pos).getName(); }
    virtual void setDimName_inplace(isl_dim_type type, unsigned pos, const char *s) ISLPP_INPLACE_QUALIFIER { setDimId_inplace(type, pos, enwrap(isl_id_alloc(getCtx()->keep(), s, nullptr))); }
    T setDimName(isl_dim_type type, unsigned pos,const char *s) const { auto result = copy(); result.setDimName_inplace(type, pos, s); return result; }
#if ISLPP_HAS_RVALUE_THIS_QUALIFIER
    T setDimName(isl_dim_type type, unsigned pos,const char *s) && { setDimName_inplace(type, pos, s); return *this; }
#endif

    virtual void insertDims_inplace(isl_dim_type type, unsigned pos, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T insertDims(isl_dim_type type, unsigned pos, unsigned n) const { auto result = copy(); result.insertDims_inplace(type, pos, n); return result; }

      virtual void addDims_inplace(isl_dim_type type, unsigned n) ISLPP_INPLACE_QUALIFIER { insertDims_inplace(type, dim(type), n); }
    T addDims(isl_dim_type type, unsigned n) const { auto result = copy(); result.addDims_inplace(type, n); return result;  }

    virtual void moveDims_inplace(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) const { auto result = copy(); result.moveDims_inplace(dst_type, dst_pos, src_type, src_pos, n); return result; }
  
    virtual void removeDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER = 0;
    T removeDims(isl_dim_type type, unsigned first, unsigned n) const { auto result = copy(); result.removeDims_inplace(type, first, n); return result; }
   
    void dropDims_inplace(isl_dim_type type, unsigned first, unsigned n) ISLPP_INPLACE_QUALIFIER  { removeDims_inplace(type, first, n); }
    T dropDims(isl_dim_type type, unsigned first, unsigned n) const { return removeDims(type,first,n); }
  }; 

  class Spacelike : public Obj {
  protected:
    bool findDim(const Dim &dim, isl_dim_type &type, unsigned &pos);
  public:
    virtual ~Spacelike() {}

    virtual Space getSpace() const = 0;

    virtual unsigned dim(isl_dim_type type) const = 0;    
    unsigned dimParam() const { return dim(isl_dim_param); }

    virtual bool hasTupleId(isl_dim_type type) const;
    virtual Id getTupleId(isl_dim_type type) const;
    virtual void setTupleId(isl_dim_type type, Id &&id) = 0;

    virtual bool hasTupleName(isl_dim_type type) const;
    virtual const char *getTupleName(isl_dim_type type) const;
    virtual void setTupleName(isl_dim_type type, const char *s) = 0;

    virtual bool hasDimId(isl_dim_type type, unsigned pos) const;
    virtual Id getDimId(isl_dim_type type, unsigned pos) const;
    Id getDimIdOrNull(isl_dim_type type, unsigned pos) const;
    virtual void setDimId(isl_dim_type type, unsigned pos, Id &&id) = 0;
    virtual int findDimById(isl_dim_type type, const Id &id) const;

    virtual bool hasDimName(isl_dim_type type, unsigned pos) const;
    virtual const char *getDimName(isl_dim_type type, unsigned pos) const;
    virtual void setDimName(isl_dim_type type, unsigned pos, const char *s)  = 0;
    virtual int findDimByName(isl_dim_type type, const char *name) const;

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
