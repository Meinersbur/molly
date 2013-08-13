#ifndef MOLLY_MOLLYREGIONPROCESSOR_H
#define MOLLY_MOLLYREGIONPROCESSOR_H

#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"


namespace molly {

  class MollyRegionProcessor {

  public:
        virtual llvm::AnalysisResolver *asResolver() = 0;

  public:
    static MollyRegionProcessor *create(MollyPassManager *pm, llvm::Region *region);
  }; // class MollyRegionProcessor

} // namespace molly
#endif /* MOLLY_MOLLYREGIONPROCESSOR_H */
