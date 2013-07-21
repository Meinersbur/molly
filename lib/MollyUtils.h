#ifndef MOLLY_MOLLYUTILS_H
#define MOLLY_MOLLYUTILS_H

#pragma region Includes and forward declarations
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>

namespace llvm {
  class Function;
  class Instruction;
  template<typename T> class SmallVectorImpl;
  class BasicBlock;
  class Region;
  class RegionInfo;
  class Pass;
} // namespace llvm

namespace polly {
  class Scop;
  class ScopStmt;
} // namespace polly
#pragma endregion


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

void collectAllRegions(llvm::RegionInfo *, llvm::SmallVectorImpl<llvm::Region*> &dstList);
void collectAllRegions(llvm::Region *region, llvm::SmallVectorImpl<llvm::Region*> &dstList);


llvm::Pass *createPassFromId(const void *id);
static inline llvm::Pass *createPassFromId(const char &id) { return createPassFromId(&id); }

template<typename T>
static inline T *createPass() {
  return static_cast<T*>(createPassFromId(T::ID));
}


llvm::Function *getParentFunction(llvm::Function *func);
llvm::Function *getParentFunction(const llvm::Region *region);
llvm::Function *getParentFunction(llvm::BasicBlock *bb);
llvm::Function *getParentFunction(const polly::Scop *scop);

} // namespace molly
#endif /* MOLLY_MOLLYUTILS_H */
