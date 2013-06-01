#ifndef ISLPP_LOCALSPACE_H
#define ISLPP_LOCALSPACE_H

#include "islpp_common.h"
#include <cassert>
#include <iterator>

#include <isl/space.h>

struct isl_local_space;

namespace isl {
  class Ctx;
  class Space;
  class Id;
  class Aff;
  class BasicMap;
  class Dim;
}


namespace isl {
  class LocalSpace;

  typedef enum {
    DimTypeCst = (1 << isl_dim_cst),
    DimTypeParam = (1 << isl_dim_param),
    DimTypeIn = (1 << isl_dim_in),
    DimTypeOut = (1 << isl_dim_out),
    DimTypeSet = (1 << isl_dim_set),
    DimTypeDiv = (1 << isl_dim_div),
    DimTypeAll = (DimTypeCst | DimTypeParam | DimTypeIn | DimTypeOut | DimTypeSet | DimTypeDiv)
  } DimTypeFlags;
  namespace DimType {
    static const DimTypeFlags Cst = DimTypeCst;
    static const DimTypeFlags Param = DimTypeParam;
    static const DimTypeFlags Out = DimTypeOut;
    static const DimTypeFlags Set = DimTypeSet;
    static const DimTypeFlags Div = DimTypeDiv;
    static const DimTypeFlags All = DimTypeAll;
  }

  class LocalSpaceDimtypeIter : public std::iterator<std::forward_iterator_tag, isl_dim_type> {
    const LocalSpace *owner;
    isl_dim_type current;
  public:
    explicit LocalSpaceDimtypeIter(const LocalSpace *owner, isl_dim_type current) : owner(owner), current(current) {}

    iterator &operator++/*preincrement*/();
    value_type operator*() const { return current; }

    bool operator==(const LocalSpaceDimtypeIter &that) const { return (this->current == that.current) && (this->current == that.current); }
    bool operator!=(const LocalSpaceDimtypeIter &that) const { return !operator==(that); }
  };


  class LocalSpaceDimIter : public std::iterator<std::forward_iterator_tag, Dim> {
    typedef LocalSpaceDimIter iterator;
    const LocalSpace *owner;
    DimTypeFlags typeFilter;

    isl_dim_type currentType;
    int currentTypeSize;
    int currentPos;
  public:
    explicit LocalSpaceDimIter(const LocalSpace *owner, DimTypeFlags typeFilter, isl_dim_type currentType, int currentPos) : owner(owner), typeFilter(typeFilter), currentType(currentType), currentPos(currentPos) {}

    const iterator &operator=(const iterator &that) {
      this->owner = that.owner;
      this->currentType = that.currentType;
      this->currentPos = that.currentPos;
      return *this;
    }

    bool operator==(LocalSpaceDimIter &that) const {
      return (this->owner == that.owner) && (this->currentType == that.currentType) && (this->currentPos == that.currentPos);
    }
    bool operator!=(LocalSpaceDimIter &that) const {
      return !operator==(that);
    }

    value_type operator*() const;

    iterator &operator++/*preincrement*/();

    iterator operator++/*postincrement*/(int) {
      iterator tmp = *this; ++*this; return tmp;
    }
  };

  /// A local space is essentially a space with zero or more existentially quantified variables.
  class LocalSpace {
#pragma region Low-Level
  private:
    isl_local_space *space;

  protected:
    explicit LocalSpace(isl_local_space *space) : space(space){}

  public:
    isl_local_space *take() { assert(space); isl_local_space *result = space; space = nullptr; return result; }
    isl_local_space *takeCopy() const;
    isl_local_space *keep() const { return space; }
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
    LocalSpace copy() { return LocalSpace::wrap(takeCopy()); }
#pragma endregion


#pragma region Build something basic from this space
    Aff createZeroAff() const;
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



    typedef LocalSpaceDimIter dim_iterator;
    typedef LocalSpaceDimIter dim_const_iterator;
    dim_const_iterator dim_begin(DimTypeFlags filter = DimType::All) const { return LocalSpaceDimIter(this, filter, (isl_dim_type)-1, 0)++; }
    dim_const_iterator dim_end() const { return LocalSpaceDimIter(this, (DimTypeFlags)0, isl_dim_all, 0); }

    typedef LocalSpaceDimtypeIter dimtype_iterator;
    dimtype_iterator dimtype_begin() { return LocalSpaceDimtypeIter(this, (isl_dim_type)-1); }
    dimtype_iterator dimtype_end() { return LocalSpaceDimtypeIter(this, isl_dim_all); }
  }; // class LocalSpace


  BasicMap lifting(LocalSpace &&ls);

  bool isEqual(const LocalSpace &ls1, const LocalSpace &ls2);
  LocalSpace intersect( LocalSpace &&ls1,  LocalSpace &&ls2);


  LocalSpace enwrap(__isl_take isl_local_space *ls) { return LocalSpace::wrap(ls); }

} // namepsace isl
#endif /* ISLPP_LOCALSPACE_H */
