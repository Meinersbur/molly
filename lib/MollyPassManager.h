/// obsolete, replaced by GlobalPassManager
// This file is the result of the shortcomings of LLVM's pass manager,
// Namely:
// - Cannot access lower-level passes (Except on-the-fly FunctionPass from ModulePass)
// - Cannot force to preserve passes which contain information modified between mutliple passes (Here: Need to remember SCoPs and alter the schedule before CodeGen; In worst case, SCoPs will be recomputed form scratch with the trivial schedule otherwise)
#ifndef MOLLY_MOLLYPASSMANAGER_H
#define MOLLY_MOLLYPASSMANAGER_H

#include <llvm/Pass.h>

namespace molly {
  class MollyPassManager : public llvm::ModulePass {
  private:

  public:
    static char ID;
    MollyPassManager();

    static MollyPassManager *create();

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    virtual bool runOnModule(llvm::Module &M);

    void add(llvm::Pass *pass);

  }; // class MollyPassManager

   MollyPassManager *createMollyPassManager();
} // namespace molly
#endif /* MOLLY_MOLLYPASSMANAGER_H */
