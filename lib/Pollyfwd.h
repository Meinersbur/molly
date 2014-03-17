#ifndef MOLLY_POLLYFWD_H
#define MOLLY_POLLYFWD_H

namespace llvm {
  // #include <llvm/IR/IRBuilder.h>
  class ConstantFolder;
  template<bool preserveNames> class IRBuilderDefaultInserter;
  template<bool preserveNames, typename T, typename Inserter> class IRBuilder;
} // namespace llvm

namespace polly {
   // #include <polly/ScopInfo.h>
  class Scop;
  class ScopStmt;
  class MemoryAccess;

  // #include <polly/ScopPass.h>
  class ScopPass;

  // #include <polly/IRBuilder.h>
  template <bool PreserveNames> class PollyBuilderInserter;
  typedef PollyBuilderInserter<true> IRInserter;
  typedef llvm::IRBuilder<true, llvm::ConstantFolder, IRInserter> PollyIRBuilder;

  // #include <polly/Accesses.h>
  class Access;

} // namespace polly

#endif /* MOLLY_POLLYFWD_H */
