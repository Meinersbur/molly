#ifndef MOLLY_SCOPFIELDCODEGEN_H
#define MOLLY_SCOPFIELDCODEGEN_H

namespace llvm {
   class ModulePass;
  class FunctionPass;
} // namespace llvm

namespace polly {
  class ScopPass;
} // namespace polly


namespace molly {
  polly::ScopPass *createScopFieldCodeGenPass();
} // namespace molly
#endif /* MOLLY_SCOPFIELDCODEGEN_H */
