#ifndef MOLLY_MOLLYSCOPPROCESSOR_H
#define MOLLY_MOLLYSCOPPROCESSOR_H

#include <llvm/Support/Compiler.h>
#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include <vector>
//#include "Islfwd.h"


namespace molly {

  class MollyScopProcessor {
  protected:
    MollyScopProcessor() {}
    /* implicit */ MollyScopProcessor(const MollyScopProcessor &that) LLVM_DELETED_FUNCTION;

  public:
    virtual std::vector<const llvm::SCEV *> getClusterCoordinates() = 0;
    virtual bool hasFieldAccess() = 0;

    virtual void computeScopDistibution() = 0;
    virtual void genCommunication() = 0;
    virtual void pollyCodegen() = 0;

  public:
    static MollyScopProcessor *create(MollyPassManager *pm, polly::Scop *scop);
  }; // class MollyScopProcessor

} // namespace molly
#endif /* MOLLY_MOLLYSCOPPROCESSOR_H */
