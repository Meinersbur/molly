#include "MollyUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include <polly/ScopInfo.h>

using namespace llvm;
using namespace polly;
using namespace molly;


/// Convenience function to get a list if instructions so we can modify the function without invalidating iterators
/// Should be more effective if we filter useful instructions first (lambda?)
void molly::collectInstructionList(Function *func, SmallVectorImpl<Instruction*> &list) {
  auto bbList = &func->getBasicBlockList();
  for (auto it = bbList->begin(), end = bbList->end(); it!=end; ++it) {
    auto bb = &*it;
    collectInstructionList(bb, list);
  }
}


void molly::collectInstructionList(llvm::BasicBlock *bb, llvm::SmallVectorImpl<llvm::Instruction*> &list) {
  auto instList = &bb->getInstList();
  for (auto it = instList->begin(), end = instList->end(); it!=end; ++it) {
    auto *inst = &*it;
    list.push_back(inst);
  }
}


void molly::collectScopStmts(polly::Scop *scop, llvm::SmallVectorImpl<polly::ScopStmt*> &list) {
  for (auto it = scop->begin(), end = scop->end(); it!=end; ++it) {
    ScopStmt* stmt = *it;
    list.push_back(stmt);
  }
}
