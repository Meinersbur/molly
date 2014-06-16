#include "MollyUtils.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include <polly/ScopInfo.h>
#include <llvm/Analysis/RegionInfo.h>
#include "MollyFieldAccess.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/RegionPrinter.h"

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
    auto &subregion = *it;
    collectAllRegions(subregion.get(), dstList);
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


llvm::Function *molly::getFunctionOf(llvm::Function *func) { 
  return func;
}
llvm::Function *molly::getFunctionOf(const llvm::Region *region) {
  return region->getEntry()->getParent(); 
}
llvm::Function *molly::getFunctionOf(llvm::BasicBlock *bb) { 
  return bb->getParent(); 
}
llvm::Function *molly::getFunctionOf(const polly::Scop *scop) { 
  return getFunctionOf(&scop->getRegion()); 
}
llvm::Function *molly::getFunctionOf(const polly::ScopStmt *stmt) {
  return getFunctionOf(stmt->getParent());
}
Function *molly::getFunctionOf(Value *v) {
  if (Function *F = dyn_cast<Function>(v))
    return F;

  if (Instruction *I = dyn_cast<Instruction>(v))
    return I->getParent()->getParent();

  if (BasicBlock *B = dyn_cast<BasicBlock>(v))
    return B->getParent();

  return nullptr;
}


const llvm::Module *molly::getModuleOf(const llvm::Function *func){
  return func->getParent();
}
llvm::Module *molly::getModuleOf(llvm::Function *func){
  return func->getParent();
}
llvm::Module *molly::getModuleOf(polly::Scop *scop) {
  return getFunctionOf(scop)->getParent();
}
llvm::Module *molly::getModuleOf(polly::ScopStmt *scopStmt) {
  return getFunctionOf(scopStmt)->getParent();
}
const llvm::Module *molly::getModuleOf(const llvm::BasicBlock *bb) {
  return bb->getParent()->getParent();
}
llvm::Module *molly::getModuleOf(llvm::BasicBlock *bb) {
  return bb->getParent()->getParent();
}
const llvm::Module *molly::getModuleOf(const llvm::GlobalValue *v){
  return v->getParent();
}
llvm::Module *molly::getModuleOf(llvm::GlobalValue *v) {
  return v->getParent();
}
llvm::Module *molly::getModuleOf(llvm::IRBuilder<> &builder) {
  return getModuleOf(builder.GetInsertBlock());
}


llvm::Region *molly::getRegionOf(polly::Scop *scop) {
  return &scop->getRegion();
}
llvm::Region *molly::getRegionOf(polly::ScopStmt *scopStmt) {
  auto result = scopStmt->getRegion();
  assert(result->contains(scopStmt->getBasicBlock()));
  return const_cast<Region*>(result);
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


unsigned molly::positionInBasicBlock(llvm::Instruction *instr) {
  assert(instr);
  auto bb = instr->getParent();
  assert(bb);

  unsigned i = 0;
  for (auto it = bb->begin(), end = bb->end(); it != end; ++it) {
    if (&*it == instr)
      return i;
    i += 1;
  }
  llvm_unreachable("Instruction not in basic block it said itself it is in");
}



void viewRegion(const Function *f) {
  auto F = const_cast<Function*>(f);
  assert(!F->isDeclaration());

  FunctionPassManager FPM(F->getParent());
  auto V = createRegionViewerPass();
  FPM.add(V);
  FPM.doInitialization();
  FPM.run(*F);
  FPM.doFinalization();
}


void viewRegion(RegionInfo *RI) {
 auto F = RI->getTopLevelRegion()->getEntry()->getParent();
  auto viewer = createRegionViewerPass();
  viewer->setResolver(RI->getResolver());
  viewer->runOnFunction(*F);
  delete viewer;
}


void viewRegionOnly(const Function *f) {
  auto F = const_cast<Function*>(f);
  assert(!F->isDeclaration());

  FunctionPassManager FPM(F->getParent());
  auto V = createRegionOnlyViewerPass();
  FPM.add(V);
  FPM.doInitialization();
  FPM.run(*F);
  FPM.doFinalization();
}


void viewRegionOnly(RegionInfo *RI) {
  auto F = RI->getTopLevelRegion()->getEntry()->getParent();
  auto viewer = createRegionOnlyViewerPass();
  viewer->setResolver(RI->getResolver());
  viewer->runOnFunction(*F);
  delete viewer;
}
