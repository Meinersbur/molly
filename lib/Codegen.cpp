#include "Codegen.h"

#include "IslExprBuilder.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include "islpp/MultiPwAff.h"
#include <llvm/IR/Function.h>
#include "IslExprBuilder.h"
#include "MollyIntrinsics.h"
#include <polly/ScopInfo.h>
#include "polly/CodeGen/CodeGeneration.h"
#include "MollyScopStmtProcessor.h"
#include "islpp/Map.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore) : stmtCtx(stmtCtx), irBuilder(stmtCtx->getLLVMContext()) {
  auto bb = stmtCtx->getBasicBlock();
  if (insertBefore) {
    assert(insertBefore->getParent() == bb);
    irBuilder.SetInsertPoint(insertBefore);
  } else {
    // Insert at end of block instead
    irBuilder.SetInsertPoint(bb);
  }
}


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx) : stmtCtx(stmtCtx), irBuilder(stmtCtx->getBasicBlock()) {
}


StmtEditor MollyCodeGenerator::getStmtEditor() { 
  return StmtEditor(stmtCtx->getStmt());
}


std::map<isl_id *, llvm::Value *> & MollyCodeGenerator :: getIdToValueMap() {
  return stmtCtx->getIdToValueMap();
}


// SCEVAffinator::getPwAff
llvm::Value *MollyCodeGenerator::codegenAff(const isl::PwAff &aff) {
  auto &valueMap = getIdToValueMap();
  auto result = polly::buildIslAff(irBuilder.GetInsertPoint(), aff.takeCopy(), valueMap, stmtCtx->asPass());
  return result;
}


std::vector<llvm::Value *> MollyCodeGenerator::codegenMultiAff(const isl::MultiPwAff &maff) {
  std::vector<llvm::Value *> result;

  auto nDims = maff.getOutDimCount();
  result.reserve(nDims);
  for (auto i = nDims-nDims; i < nDims; i+=1 ) {
    auto aff = maff.getPwAff(i);
    auto value = codegenAff(aff);
    result.push_back(value);
  }

  return result;
}


llvm::Value *MollyCodeGenerator::codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices) {
  auto call = callLocalPtrIntrinsic(fvar, indices);
  return irBuilder.Insert(call, "ptr_local");
}


void MollyCodeGenerator::codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, isl::Map accessRelation) {
  auto ptrVal = codegenPtrLocal(fvar, indices);
  auto store = irBuilder.CreateStore(val, ptrVal);

  auto editor = getStmtEditor();
  accessRelation.setInTupleId_inplace(editor.getDomainTupleId());
  editor.addWriteAccess(store, fvar, accessRelation.move());
}


void MollyCodeGenerator::codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::MultiPwAff index) {
  auto indices = codegenMultiAff(index);
  auto accessRel = index.toMap();
  codegenStoreLocal(val, fvar, indices, accessRel);
}
