#ifndef MOLLY_FIELDCODEGEN_H
#define MOLLY_FIELDCODEGEN_H

#pragma region Forward declarations
namespace llvm {
  class ModulePass;
  class FunctionPass;
} // namespace llvm

namespace polly {
  class ScopPass;
} // namespace polly
#pragma endregion


namespace molly {

  //polly::ScopPass *createFieldScopCodeGenPass();
  extern const char &ModuleFieldGenPassID;
  llvm::ModulePass *createModuleFieldGenPass();

  extern const char &FieldCodeGenPassID;
  llvm::FunctionPass *createFieldCodeGenPass();

} // namespace molly

#endif /* MOLLY_FIELDCODEGEN_H */
