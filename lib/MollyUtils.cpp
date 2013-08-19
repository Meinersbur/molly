#include "MollyUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include <polly/ScopInfo.h>
#include <llvm/Analysis/RegionInfo.h>
#include "MollyFieldAccess.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Module.h"

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
llvm::Function *molly::getParentFunction(const polly::ScopStmt *stmt) {
  return getParentFunction(stmt->getParent());
}
Function *molly::getParentFunction(Value *v) {
  if (Function *F = dyn_cast<Function>(v))
    return F;

  if (Instruction *I = dyn_cast<Instruction>(v))
    return I->getParent()->getParent();

  if (BasicBlock *B = dyn_cast<BasicBlock>(v))
    return B->getParent();

  return nullptr;
}


const llvm::Module *molly::getParentModule(const llvm::Function *func){
  return func->getParent();
}
llvm::Module *molly::getParentModule(llvm::Function *func){
  return func->getParent();
}
llvm::Module *molly::getParentModule(polly::Scop *scop) {
  return getParentFunction(scop)->getParent();
}
llvm::Module *molly::getParentModule(polly::ScopStmt *scopStmt) {
  return getParentFunction(scopStmt)->getParent();
}
const llvm::Module *molly::getParentModule(const llvm::BasicBlock *bb) {
  return bb->getParent()->getParent();
}
llvm::Module *molly::getParentModule(llvm::BasicBlock *bb) {
  return bb->getParent()->getParent();
}
const llvm::Module *molly::getParentModule(const llvm::GlobalValue *v){
  return v->getParent();
}
llvm::Module *molly::getParentModule(llvm::GlobalValue *v) {
  return v->getParent();
}
llvm::Module *molly::getParentModule(llvm::IRBuilder<> &builder) {
  return getParentModule(builder.GetInsertBlock());
}


llvm::Region *molly::getParentRegion(polly::Scop *scop) {
  return &scop->getRegion();
}
llvm::Region *molly::getParentRegion(polly::ScopStmt *scopStmt) {
  return getParentRegion(scopStmt->getParent());
}


llvm::Function *molly::createFunction( llvm::Type *rtnTy, llvm::ArrayRef<llvm::Type*> argTys, llvm::Module *module, GlobalValue::LinkageTypes linkage ,llvm::StringRef name) {
  if (!rtnTy) {
    auto &llvmContext = module->getContext();
    rtnTy = Type::getVoidTy(llvmContext);
  }

  auto funcTy = FunctionType::get(rtnTy, argTys, false);
 return Function::Create(funcTy, linkage, name, module);
}

llvm::Function *molly::createFunction( llvm::Type *rtnTy, llvm::Module *module, GlobalValue::LinkageTypes linkage ,llvm::StringRef name) {
  if (!rtnTy) {
    auto &llvmContext = module->getContext();
    rtnTy = Type::getVoidTy(llvmContext);
  }

  auto funcTy = FunctionType::get(rtnTy, false);
 return Function::Create(funcTy, linkage, name, module);
}
