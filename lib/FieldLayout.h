#ifndef MOLLY_FIELDLAYOUT_H
#define MOLLY_FIELDLAYOUT_H

#include <islpp/PwMultiAff.h>

#pragma region Forward declarations
namespace isl {
} // namespace isl

namespace molly {
class FieldType;
} // namespace molly
#pragma endregion


namespace molly {

  class FieldLayout {
    FieldType *fieldTy;

    isl::PwMultiAff mapping; // { logical indexset from type -> (rank|physical location) }
    //TODO: There could be mappings that cannot be represented by affine expression; Need to chain m

  public:
  }; // class FieldLayout

} // namespace molly

#endif /* MOLLY_FIELDLAYOUT_H */
