#ifndef MOLLY_MOLLYUTILS_H
#define MOLLY_MOLLYUTILS_H

#pragma region Includes and forward declarations
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ilist.h>
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include "llvm/IR/GlobalValue.h"

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


llvm::Function *getFunctionOf(llvm::Function *func);
llvm::Function *getFunctionOf(const llvm::Region *region);
llvm::Function *getFunctionOf(llvm::BasicBlock *bb);
llvm::Function *getFunctionOf(const polly::Scop *scop);
llvm::Function *getFunctionOf(const polly::ScopStmt *stmt);
llvm::Function *getFunctionOf(llvm::Value *v);

const llvm::Module *getModuleOf(const llvm::Function *func);
llvm::Module *getModuleOf(llvm::Function *func);
llvm::Module *getModuleOf(polly::Scop *scop);
llvm::Module *getModuleOf(polly::ScopStmt *scopStmt);
const llvm::Module *getModuleOf(const llvm::BasicBlock *bb);
llvm::Module *getModuleOf(llvm::BasicBlock *bb);
const llvm::Module *getModuleOf(const llvm::GlobalValue *bb);
llvm::Module *getModuleOf(llvm::GlobalValue *bb);
llvm::Module *getModuleOf(DefaultIRBuilder &builder);

llvm::Region *getRegionOf(polly::Scop *);
llvm::Region *getRegionOf(polly::ScopStmt *);

llvm::Function *createFunction( llvm::Type *rtnTy, llvm::ArrayRef<llvm::Type*> argTys, llvm::Module *module,  llvm::GlobalValue::LinkageTypes linkage =  llvm::GlobalValue::PrivateLinkage ,llvm::StringRef name = llvm::StringRef());
llvm::Function *createFunction( llvm::Type *rtnTy, llvm::Module *module,  llvm::GlobalValue::LinkageTypes linkage =  llvm::GlobalValue::PrivateLinkage ,llvm::StringRef name = llvm::StringRef());

/// Get the index of the instruction in its BasicBlock
/// This function should be avoided if possible because of its linear cost
unsigned positionInBasicBlock(llvm::Instruction *instr);

} // namespace molly

void viewRegion(const llvm::Function *f);
void viewRegion(llvm::RegionInfo *RI);
void viewRegionOnly(const llvm::Function *f);
void viewRegionOnly(llvm::RegionInfo *RI);

#endif /* MOLLY_MOLLYUTILS_H */
