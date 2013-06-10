#ifndef MOLLY_FLATTENMAPPING_H
#define MOLLY_FLATTENMAPPING_H

#include "LayoutMapping.h"

namespace molly {
  class FieldType;
} // namespace molly


namespace molly {

  /// Maps any multi-dimensional space to linear using row-major order
  class FlattenMapping final : public LayoutMapping {
  private:
    FieldType *type;

  public:
    FlattenMapping() {
    }

     virtual LayoutMapping *getInnerMapping() {
       // No inner mapping, this is always the last one
       return nullptr; 
     }


     virtual unsigned getOffset(llvm::ArrayRef<int> coords) {
     }

  }; // class FlattenMapping
} // namespace molly

#endif /* MOLLY_FLATTENMAPPING_H */
