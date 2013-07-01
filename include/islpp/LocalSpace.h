#ifndef ISLPP_LOCALSPACE_H
#define ISLPP_LOCALSPACE_H

#include "islpp_common.h"
#include "Dim.h"
#include <cassert>
#include <iterator>
#include "Spacelike.h"
#include "Space.h"

#include <isl/space.h>

struct isl_local_space;


namespace isl {
  class Ctx;
  class Space;
  class Id;
  class Aff;
  class BasicMap;
  class Dim;
} // namespace isl


namespace isl {
  class LocalSpace;


  




  /// A local space is essentially a space with zero or more existentially quantified variables.
  class LocalSpace : public Spacelike3<LocalSpace> {
    friend class Spacelike3<LocalSpace>;

#pragma region Low-Level
  private:
    isl_local_space *space;

  protected:
    explicit LocalSpace(isl_local_space *space) : space(space){}

  public:
    isl_local_space *take() { assert(space); isl_local_space *result = space; space = nullptr; return result; }
    isl_local_space *takeCopy() const;
    isl_local_space *keep() const { assert(space); return space; }
    isl_local_space *keepOrNull() const { return space; }
    void give(isl_local_space *set) ;


    static LocalSpace wrap(isl_local_space *space) { return LocalSpace(space); }
#pragma endregion


  public:
    LocalSpace() : space(nullptr) {}
    /* implicit */ LocalSpace(LocalSpace &&that) : space(that.take()) { }
    /* implicit */ LocalSpace(const LocalSpace &that) : space(that.takeCopy()) {  }
    ~LocalSpace();

    const LocalSpace &operator=(LocalSpace &&that) { assert(!this->space); this->space = that.take(); return *this; }
    const LocalSpace &operator=(const LocalSpace &that) { give(that.takeCopy()); return *this; }

#pragma region Conversion from isl::Space
    /* implicit */ LocalSpace(Space &&);
    /* implicit */ LocalSpace(const Space &);

    const LocalSpace &operator=(Space &&);
    const LocalSpace &operator=(const Space &that);
#pragma endregion


#pragma region Creational
    LocalSpace copy() const { return LocalSpace::wrap(takeCopy()); }
    LocalSpace &&move() { return std::move(*this); }
#pragma endregion


#pragma region Build something basic from this space
    Aff createZeroAff() const;
    Aff createConstantAff(int) const;

    Constraint createEqualityConstraint() const;
    Constraint createInequalityConstraint() const;
#pragma endregion


#pragma region Spacelike
    bool hasTupleId(isl_dim_type type) const { return getSpace().hasTupleId(type); }
    Id getTupleId(isl_dim_type type) const { return getSpace().getTupleId(type); }
#pragma endregion


    Ctx *getCtx() const;
    bool isSet() const;
    int dim(isl_dim_type type) const;
    bool hasDimId(isl_dim_type type, unsigned pos) const;
    Id getDimId(isl_dim_type type, unsigned pos) const;
    bool hasDimName(isl_dim_type type, unsigned pos) const;
    const char *getDimName(isl_dim_type type, unsigned pos) const;
    void setDimName(isl_dim_type type, unsigned pos, const char *s);
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);
    Space getSpace() const;
    Aff getDiv(int pos) const;

    void domain();
    void range();
    void fromDomain();
    void addDims(isl_dim_type type, unsigned n);
    void insertDims(isl_dim_type type, unsigned first, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);



    //typedef LocalSpaceDimIter dim_iterator;
    //typedef LocalSpaceDimIter dim_const_iterator;
    //dim_const_iterator dim_begin(DimTypeFlags filter = DimType::All) const { return LocalSpaceDimIter(this, filter, (isl_dim_type)-1, 0)++; }
    //dim_const_iterator dim_end() const { return LocalSpaceDimIter(this, (DimTypeFlags)0, isl_dim_all, 0); }

    //typedef LocalSpaceDimtypeIter dimtype_iterator;
    //dimtype_iterator dimtype_begin() { return LocalSpaceDimtypeIter(this, (isl_dim_type)-1); }
    //dimtype_iterator dimtype_end() { return LocalSpaceDimtypeIter(this, isl_dim_all); }

  //protected:
    LocalSpace getSpacelike() const { return copy(); }
  }; // class LocalSpace

  
  inline LocalSpace enwrap(__isl_take isl_local_space *ls) { return LocalSpace::wrap(ls); }


  BasicMap lifting(LocalSpace &&ls);

  bool isEqual(const LocalSpace &ls1, const LocalSpace &ls2);
  LocalSpace intersect( LocalSpace &&ls1,  LocalSpace &&ls2);

} // namepsace isl
#endif /* ISLPP_LOCALSPACE_H */
