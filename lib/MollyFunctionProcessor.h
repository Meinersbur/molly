#ifndef MOLLY_MOLLYFUNCTIONPROCESSOR_H
#define MOLLY_MOLLYFUNCTIONPROCESSOR_H

#include <llvm/Support/Compiler.h>
#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"

namespace molly {

  class MollyFunctionProcessor {
  protected:
    MollyFunctionProcessor() {}
    /* implicit */ MollyFunctionProcessor(const MollyFunctionProcessor &that) LLVM_DELETED_FUNCTION;

  public:
    virtual llvm::FunctionPass *asPass() = 0;
    virtual llvm::AnalysisResolver *asResolver() = 0;

    virtual llvm::Value *getClusterCoordinate( int i ) = 0;

  public:
   virtual void isolateFieldAccesses() = 0;
   virtual void replaceRemainaingIntrinsics() = 0;

  public:
    static MollyFunctionProcessor *create(MollyPassManager *pm, llvm::Function *func);
  }; // MollyFunctionProcessor

} // namespace molly
#endif /* MOLLY_MOLLYFUNCTIONPROCESSOR_H */
