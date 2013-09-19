#ifndef MOLLY_LAYOUTMAPPING_H
#define MOLLY_LAYOUTMAPPING_H

#include <llvm/ADT/ArrayRef.h>

namespace llvm {
  class Function;
} // namespace llvm


namespace molly {

  /// Baseclass for all mapping techniques (RectangularMapping, ZCurveMapping, ...)
  class LayoutMapping {
  public:
    virtual ~LayoutMapping();

    //virtual LayoutMapping *getInnerMapping() = 0;
    //virtual llvm::Function *getMappingFunction() = 0;

    //virtual unsigned getOffset(llvm::ArrayRef<int> coords) = 0;

  }; // class LayoutMapping


  class LayoutLinearizer : public LayoutMapping {
  public:
    ~LayoutLinearizer() {}


  }; // class LayoutLinearizer

} // namespace molly
#endif /* MOLLY_LAYOUTMAPPING_H */
