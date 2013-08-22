#ifndef ISLPP_DIMRANGE_H
#define ISLPP_DIMRANGE_H

#include <isl/space.h> // enum isl_dim_type
#include "Islfwd.h"
#include "Space.h"
#include "LocalSpace.h"

namespace isl {

   class DimRange {
    private:
      isl_dim_type type;
    unsigned first;
    unsigned count;

    // TODO: PointerUnion
    Space space;
    //LocalSpace localspace;

   protected:
     DimRange(Space &&space, isl_dim_type type, unsigned first, unsigned count) : space(space.move()), first(first), count(count)  {}
     //DimRange(LocalSpace &&localspace, isl_dim_type type, unsigned first, unsigned count) : space(), localspace(localspace.move()), first(first), count(count)  {}

   public:
     DimRange() : space(), type(isl_dim_cst), first(0),count(0) {}
     static DimRange enwrap(Space space, isl_dim_type type, unsigned first, unsigned count) { return DimRange(space.move(), type, first, count); }


  }; // class DimRange


} // namespace isl
#endif /* ISLPP_DIMRANGE_H */
