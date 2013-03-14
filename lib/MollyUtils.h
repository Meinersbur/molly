#ifndef MOLLY_MOLLYUTILS_H
#define MOLLY_MOLLYUTILS_H

namespace llvm {
  class Function;
  class Instruction;
  template<typename T> class SmallVectorImpl;
  class BasicBlock;
} // namespace llvm

namespace polly {
  class Scop;
  class ScopStmt;
} // namespace polly


namespace molly {

/// Convenience function to get a list if instructions so we can modify the function without invalidating iterators
/// Should be more effective if we filter useful instructions first (lambda?)
void collectInstructionList(llvm::Function *func, llvm::SmallVectorImpl<llvm::Instruction*> &list);

void collectInstructionList(llvm::BasicBlock *bb, llvm::SmallVectorImpl<llvm::Instruction*> &list);

void collectScopStmts(polly::Scop *scop, llvm::SmallVectorImpl<polly::ScopStmt*> &list);

} // namespace molly
#endif /* MOLLY_MOLLYUTILS_H */
