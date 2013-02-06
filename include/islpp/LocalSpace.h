#ifndef ISLPP_LOCALSPACE_H
#define ISLPP_LOCALSPACE_H

#include <cassert>

struct isl_local_space;
enum isl_dim_type;

namespace isl {
  class Ctx;
  class Space;
  class Id;
  class Aff;
  class BasicMap;
}


namespace isl {
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
    const LocalSpace &operator=(const LocalSpace &that) { give(that.takeCopy()); }

#pragma region Conversion from isl::Space
    /* implicit */ LocalSpace(Space &&);
    /* implicit */ LocalSpace(const Space &);

    const LocalSpace &operator=(Space &&);
    const LocalSpace &operator=(const Space &that);
#pragma endregion

    LocalSpace copy() { return LocalSpace::wrap(takeCopy()); }

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
  }; // class LocalSpace


  BasicMap lifting(LocalSpace &&ls);

  bool isEqual(const LocalSpace &ls1, const LocalSpace &ls2);
  LocalSpace intersect( LocalSpace &&ls1,  LocalSpace &&ls2);

} // namepsace isl
#endif /* ISLPP_LOCALSPACE_H */