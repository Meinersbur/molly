#ifndef MOLLY_MOLLYINLINEPREPA_H
#define MOLLY_MOLLYINLINEPREPA_H

namespace llvm {
  class Pass;
} // namespace llvm


namespace molly {

  extern char &MollyInlinePassID;
  llvm::Pass *createMollyInlinePass();

} // namespace molly
#endif /* MOLLY_MOLLYINLINEPREPA_H */
