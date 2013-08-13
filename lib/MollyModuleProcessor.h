#ifndef MOLLY_MOLLYMODULEPROCESSOR_H
#define MOLLY_MOLLYMODULEPROCESSOR_H

#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"


namespace molly {

  class MollyModuleProcessor {

  public:
    virtual llvm::AnalysisResolver *asResolver() = 0;

  public:
    static MollyModuleProcessor *create(MollyPassManager *pm, llvm::Module *module);
  }; // MollyModuleProcessor

} // namespace molly
#endif /* MOLLY_MOLLYMODULEPROCESSOR_H */
