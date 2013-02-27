#ifndef MOLLY_FIELDCODEGEN_H
#define MOLLY_FIELDCODEGEN_H

namespace llvm {
  class FunctionPass;
} // namespace llvm

namespace polly {
  class ScopPass;
} // namespace polly


namespace molly {

  polly::ScopPass *createFieldScopCodeGenPass();
  llvm::FunctionPass *createFieldCodeGenPass();

} // namespace molly

#endif /* MOLLY_FIELDCODEGEN_H */