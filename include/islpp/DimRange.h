#ifndef ISLPP_DIMRANGE_H
#define ISLPP_DIMRANGE_H

#include <isl/space.h> // enum isl_dim_type
#include "Islfwd.h"
#include <assert.h>


namespace isl {

  /// May also have sth like LocalDimRange
  class DimRange {
  private:
    isl_dim_type type;
    unsigned first;
    unsigned count;
    isl_space *space;

    DimRange(isl_dim_type type, unsigned first, unsigned count, __isl_take isl_space *space) : type(type), first(first), count(count), space(space) {
      assert(first + count <= isl_space_dim(space, type));
    }

  public:
    DimRange() : type(isl_dim_cst/*cst dimension is for internal use only*/) {}
    ~DimRange() { isl_space_free(space); space = NULL; }

    static DimRange enwrap(isl_dim_type type, unsigned first, unsigned count, __isl_take isl_space *space);
    static DimRange enwrap(isl_dim_type type, unsigned first, unsigned count, Space &&space);
    static DimRange enwrap(isl_dim_type type, unsigned first, unsigned count, const Space &space);

  public:
    bool isNull() { return type==isl_dim_cst; }
    bool isValid() { return type!=isl_dim_cst; }

    isl_dim_type getType() const { return type; }
    unsigned getBeginPos() const {return first;}
    unsigned getCount() const { return count; }
    unsigned getEndPos() const { return first+count; }

    Space getSpace() const;
  }; // class DimRange

} // namespace isl
#endif /* ISLPP_DIMRANGE_H */
