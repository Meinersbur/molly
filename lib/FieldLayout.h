#ifndef MOLLY_FIELDLAYOUT_H
#define MOLLY_FIELDLAYOUT_H

#include "islpp/PwMultiAff.h"
#include "islpp/Islfwd.h"

#pragma region Forward declarations
namespace isl {
} // namespace isl

namespace molly {
  class FieldType;
  class LayoutMapping;
} // namespace molly
#pragma endregion


namespace molly {

  class FieldLayout {
    FieldType *fieldTy;

    LayoutMapping *mapping;

    isl::PwMultiAff affineMapping; // { logical indexset from type -> (rank|physical location) }
    //TODO: There could be mappings that cannot be represented by affine expression; Need to chain them

  public:
    ~FieldLayout();

#pragma region Distribution
    // Single index can be map to multiple ranks to allow redundancy
    isl::Map getRankMapping(); /* { [indexset] -> [rank] } */ 

    isl::Map getLocalIndices(); /* { [indexset] -> set[] }*/ 
#pragma endregion

  }; // class FieldLayout

} // namespace molly

#endif /* MOLLY_FIELDLAYOUT_H */
