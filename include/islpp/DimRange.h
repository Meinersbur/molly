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
    bool isNull() const { return type==isl_dim_cst; }
    bool isValid() const { return type!=isl_dim_cst; }

    isl_dim_type getType() const { assert(isValid()); return type; }
    unsigned getBeginPos() const { assert(isValid()); return first;}
    unsigned getCount() const { assert(isValid()); return count; }
    unsigned getEndPos() const { assert(isValid()); return first+count; }

    // Alternative names
    unsigned getFirst() const { assert(isValid()); return first;} 
     unsigned getEnd() const { assert(isValid()); return first+count;}
      unsigned getLast() const { assert(isValid()); assert(count >= 1); return first+count-1;}

      unsigned relativePos(unsigned i) const { 
        assert(isValid()); 
        assert(first <= i && i < first+count); 
        return i - count; 
      }

    Space getSpace() const;
  }; // class DimRange

} // namespace isl
#endif /* ISLPP_DIMRANGE_H */
