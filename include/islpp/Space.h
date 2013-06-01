#ifndef ISLPP_SPACE_H
#define ISLPP_SPACE_H

#include "islpp_common.h"
#include <assert.h>
#include <isl/space.h> // enum isl_dim_type;

#include "Ctx.h"

struct isl_space;

namespace isl {
  class Ctx;
  class Id;
  class Set;
  class BasicSet;
  class Map;
  class BasicMap;
  class Int;
} // namespace isl


namespace isl {
  /// Whenever a new set, relation or similiar object is created from scratch, the space in which it lives needs to be specified using an isl_space. Each space involves zero or more parameters and zero, one or two tuples of set or input/output dimensions. The parameters and dimensions are identified by an isl_dim_type and a position. The type isl_dim_param refers to parameters, the type isl_dim_set refers to set dimensions (for spaces with a single tuple of dimensions) and the types isl_dim_in and isl_dim_out refer to input and output dimensions (for spaces with two tuples of dimensions). Local spaces (see Local Spaces) also contain dimensions of type isl_dim_div. Note that parameters are only identified by their position within a given object. Across different objects, parameters are (usually) identified by their names or identifiers. Only unnamed parameters are identified by their positions across objects. The use of unnamed parameters is discouraged.
  class Space {

#pragma region Low-Level
  private:
    isl_space *space;

  protected:
    explicit Space(isl_space *space);

  public:
    isl_space *take() { assert(space); isl_space *result = space; space = nullptr; return result; }
    isl_space *takeCopy() const;
    isl_space *keep() const { return space; }
    void give(isl_space *space) { assert(!this->space); this->space = space; }

    static Space wrap(isl_space *space) { return Space(space); }
#pragma endregion

  public:
    Space() : space(nullptr) {};
    /* implicit */ Space(Space &&that) : space(that.take()) { }
    /* implicit */ Space(const Space &that) : space(that.takeCopy()) {  }
    ~Space();

    const Space &operator=(const Space &that) { give(that.takeCopy()); return *this; }
    const Space &operator=(Space &&that) { give(that.take()); return *this; }

#pragma region Creational
    static Space createMapSpace(const Ctx *ctx, unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/);
    static Space createParamsSpace(const Ctx *ctx, unsigned nparam);
    static Space createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim);
    static Space createMapFromDomainAndRange(Space &&domain, Space &&range);

    Space copy() const { return Space::wrap(takeCopy()); }
    Space &&move() { return std::move(*this); }
#pragma endregion


#pragma region Create Sets/Maps using this map
    Set emptySet() const;
    Set universeSet() const;

    Map emptyMap() const;
    Map universeMap() const;

    Aff createZeroAff() const;
    Aff createConstantAff(const Int &) const;
    Aff createVarAff(isl_dim_type type, unsigned pos) const;
#pragma endregion

    Ctx *getCtx() const { return enwrap(isl_space_get_ctx(keep())); }

    unsigned dim(isl_dim_type type) const;
    unsigned getParamDims() const;
    unsigned getSetDims() const;
    unsigned getInDims() const;
    unsigned getOutDims() const;
    unsigned getTotalDims() const;
    bool isParamsSpace() const;
    bool isSetSpace() const;
    bool isMapSpace() const;

    bool hasDimId(isl_dim_type type, unsigned pos) const;
    void setDimId(isl_dim_type type, unsigned pos, Id &&id);
    Id getDimId(isl_dim_type type, unsigned pos) const;
    void setDimName(isl_dim_type type, unsigned pos, const char *name);
    bool hasDimName(isl_dim_type type, unsigned pos)const ;
    const char *getDimName(isl_dim_type type, unsigned pos)const;
    int findDimById(isl_dim_type type, const Id &id)const;
    int findDimByName(isl_dim_type, const char *name)const;

    void setTupleId(isl_dim_type type,  Id &&id);
    void resetTupleId(isl_dim_type type);
    bool hasTupleId(isl_dim_type type) const;
    Id getTupleId(isl_dim_type type) const;
    void setTupleName(isl_dim_type type, const char *s);
    bool hasTupleName(isl_dim_type type) const;
    const char *getTupleName(isl_dim_type type) const;

    bool isWrapping() const;
    void wrap();
    void unwrap();

    void domain();
    void fromDomain();
    void range();
    void fromRange();
    void params();
    void setFromParams();
    void reverse();
    void insertDims(isl_dim_type type, unsigned pos, unsigned n);
    void addDims(isl_dim_type type, unsigned n);
    void dropDims(isl_dim_type type, unsigned first, unsigned n);
    void moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n);
    void mapFromSet();
    void zip();
    void curry();
    void uncurry();

#pragma region Create
    Set createUniverseSet() const;
    BasicSet createUniverseBasicSet() const;
    Map createUniverseMap() const;
    BasicMap createUniverseBasicMap() const;
#pragma endregion
  }; // class Space


  bool isEqual(const Space &space1, const Space &space2);
  /// checks whether the first argument is equal to the domain of the second argument. This requires in particular that the first argument is a set space and that the second argument is a map space.
  bool isDomain(const Space &space1, const Space &space2);
  bool isRange(const Space &space1, const Space &space2);

  Space join(Space &&left, Space &&right);
  Space alignParams(Space &&space1, Space &&space2);
} // namespace isl

#endif /* ISLPP_SPACE_H */
