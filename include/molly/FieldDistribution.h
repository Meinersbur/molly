#ifndef MOLLY_FIELDDISTRIBUTION_H
#define MOLLY_FIELDDISTRIBUTION_H

namespace llvm {
  class ModulePass;
} // namespace llvm


namespace molly {
  class FieldDistributionPass;
  extern char &FieldDistributionPassID;
  llvm::ModulePass *createFieldDistributionPass();
} // namespace molly

#endif /* MOLLY_FIELDDISTRIBUTION_H */