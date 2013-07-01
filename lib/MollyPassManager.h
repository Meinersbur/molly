/// obsolete, replaced by GlobalPassManager
// This file is the result of the shortcomings of LLVM's pass manager,
// Namely:
// - Cannot access lower-level passes (Except on-the-fly FunctionPass from ModulePass)
// - Cannot force to preserve passes which contain information modified between mutliple passes (Here: Need to remember SCoPs and alter the schedule before CodeGen; In worst case, SCoPs will be recomputed form scratch with the trivial schedule otherwise)
#ifndef MOLLY_MOLLYPASSMANAGER_H
#define MOLLY_MOLLYPASSMANAGER_H

namespace llvm {
  //#include <llvm/Pass.h>
  class ModulePass;
} // namespace molly

namespace polly {
} // namespace polly

namespace molly {
  extern char &MollyPassManagerID;
  llvm::ModulePass *createMollyPassManager();
} // namespace molly
#endif /* MOLLY_MOLLYPASSMANAGER_H */
