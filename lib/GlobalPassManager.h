#ifndef MOLLY_GLOBALPASSANAGER_H
#define MOLLY_GLOBALPASSANAGER_H
// This file is the result of the shortcomings of LLVM's pass manager,
// Namely:
// - Cannot access lower-level passes (Except on-the-fly FunctionPass from ModulePass)
// - Cannot force to preserve passes which contain information modified between mutliple passes (Here: Need to remember SCoPs and alter the schedule before CodeGen; In worst case, SCoPs will be recomputed form scratch with the trivial schedule otherwise)


#include <llvm/Support/Compiler.h>
#include <llvm/Pass.h> // ModulePass (baseclass of GlobalPassManager)


namespace molly {

  /// A pass manager that remembers the result of analyses of smaller units (e.g. ModulePass using a RegionPass)
  class GlobalPassManager : public llvm::ModulePass {
  public:

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
    }

    virtual bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
      pushModule(&M);
    }

    void addPass(llvm::Pass *);


    void pushModule(llvm::Module *module) {
      beginModule();
    }

    void beginModule();
    void pushFunction(llvm::Function *function);
    void endModule();

  }; // class GlobalPassManager

} // namespace molly
#endif /* MOLLY_GLOBALPASSANAGER_H */
