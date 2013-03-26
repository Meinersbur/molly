#ifndef MOLLY_GLOBALPASSANAGER_H
#define MOLLY_GLOBALPASSANAGER_H

#include <llvm/Support/Compiler.h>
#include <llvm/Pass.h> // ModulePass (baseclass of GlobalPassManager)


namespace molly {

  /// A pass manager that remembers the result of analyses of smaller units (e.g. ModulePass using a RegionPass)
  class GlobalPassManager : public llvm::ModulePass {
  public:

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
    }

    virtual bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
    }

  }; // class GlobalPassManager

} // namespace molly
#endif /* MOLLY_GLOBALPASSANAGER_H */
