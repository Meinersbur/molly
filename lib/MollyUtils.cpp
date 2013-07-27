#include "MollyUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include <polly/ScopInfo.h>
#include <llvm/Analysis/RegionInfo.h>
#include "MollyFieldAccess.h"
#include <llvm/IR/Instructions.h>

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


void molly::collectAllRegions(llvm::RegionInfo *regionInfo, llvm::SmallVectorImpl<llvm::Region*> &dstList) {
  collectAllRegions(regionInfo->getTopLevelRegion(), dstList);
}


void molly::collectAllRegions(llvm::Region *region, llvm::SmallVectorImpl<llvm::Region*> &dstList) {
  dstList.push_back(region);
  for (auto it = region->begin(), end = region->end(); it!=end;++it) {
    auto subregion = *it;
    collectAllRegions(subregion, dstList);
  }
}


llvm::Pass *molly::createPassFromId(const void *passId) {
  auto passRegistry = PassRegistry::getPassRegistry();
  auto passInfo = passRegistry->getPassInfo(passId);
  assert(passInfo);
  auto passIsAnalysis = passInfo->isAnalysis();
  auto pass = passInfo->createPass();
  return pass;
}


llvm::Function *molly::getParentFunction(llvm::Function *func) { 
  return func;
}
llvm::Function *molly::getParentFunction(const llvm::Region *region) {
  return region->getEntry()->getParent(); 
}
llvm::Function *molly::getParentFunction(llvm::BasicBlock *bb) { 
  return bb->getParent(); 
}
llvm::Function *molly::getParentFunction(const polly::Scop *scop) { 
  return getParentFunction(&scop->getRegion()); 
}


#if 0
bool molly::isFieldAccessScopStmt(llvm::BasicBlock *bb, polly::ScopStmt *scopStmt) {
  return isFieldAccessBasicBlock(scopStmt->getBasicBlock());
}


bool molly::isFieldAccessBasicBlock(llvm::BasicBlock *bb) {
  assert(bb);

  MollyFieldAccess facc;
  for (auto &i : *bb) {
    auto instr = &i;
    facc = MollyFieldAccess::create(instr, nullptr, nullptr);
    if (facc.isValid())
      break;
  }

  if (!facc.isValid())
    return false;


    for (auto &i : *bb) {
      auto instr = &i;

      if (instr == facc.getAccessor())
        continue;

      if (instr == facc.getFieldCall())
        continue;

      return false;
    }

    return true;
}
#endif 
