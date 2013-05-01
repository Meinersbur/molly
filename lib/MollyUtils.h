#ifndef MOLLY_MOLLYUTILS_H
#define MOLLY_MOLLYUTILS_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>

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


template<typename TargetListTy, typename T>
void list_push_back(TargetListTy &list, T &&elt) {
  list.push_back(elt);
}
template<typename TargetList, typename IteratorTy>
void collect(TargetList &dstList, const IteratorTy &&itBegin, const IteratorTy &&itEnd) {
  for (auto it = itBegin; it!=itEnd; ++it) {
    list_push_back(dstList, *it);
  }
}

template<typename T, typename S>
void collect(llvm::SmallVectorImpl<T> &dstList, llvm::iplist<S> &srcList) {
  for (auto it = srcList.begin(), end = srcList.end(); it!=end; ++it) {
    list_push_back(dstList, &*it);
  }
}
/*
template<typename T, typename TargetList>
void collect(TargetList &dstList, const llvm::iplist<T> &srcList) {
  for (auto it = srcList.begin(), end = srcList.end(); it!=end; ++it) {
    list_push_back(dstList, &*it);
  }
}
*/

} // namespace molly
#endif /* MOLLY_MOLLYUTILS_H */
