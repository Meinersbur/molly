#ifndef MOLLY_SCOPSTMTSPLIT_H
#define MOLLY_SCOPSTMTSPLIT_H

namespace llvm {
  class Pass;
}

namespace molly {
  extern char &ScopStmtSplitPassID;
  llvm::Pass *createScopStmtSplitPass();
} // namespace molly

#endif /* MOLLY_SCOPSTMTSPLIT_H */
