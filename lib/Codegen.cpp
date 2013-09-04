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
#include "MollyPassManager.h"
#include <clang/CodeGen/MollyRuntimeMetadata.h>
#include <llvm/IR/Intrinsics.h>
#include "CommunicationBuffer.h"
#include "MollyScopProcessor.h"
#include "AffineMapping.h"

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


std::map<isl_id *, llvm::Value *> & MollyCodeGenerator::getIdToValueMap() {
  return stmtCtx->getIdToValueMap();
}


llvm::Module *MollyCodeGenerator::getModule() {
 return irBuilder.GetInsertBlock()->getParent()->getParent();
}


clang::CodeGen::MollyRuntimeMetadata *MollyCodeGenerator::getRtMetadata() {
  return stmtCtx->getPassManager()->getRuntimeMetadata();
}


// SCEVAffinator::getPwAff
llvm::Value *MollyCodeGenerator::codegenAff(const isl::PwAff &aff) {
  auto &valueMap = getIdToValueMap();
  //TODO: What does polly::buildIslAff do with several pieces?
  assert(aff.nPiece() == 1);
  // If it splits the BB to test for piece inclusion, the additional ScopStmt would not consider them as belonging to them.
  // Three solution ideas:
  // 1. Use conditional assignment instead of branches
  // 2. Create a ad-hoc function that contains the case distinction; meant to be inlined later by llvm optimization passes
  // 3. Create new ScopStmts for each peace, let Polly generate code to distinguish them
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


llvm::Value *MollyCodeGenerator::codegenScev(const llvm::SCEV *scev) {
  return stmtCtx->getScopProcessor()->codegenScev(scev, irBuilder.GetInsertPoint());
}


  llvm::Value *MollyCodeGenerator::codegenId(const isl::Id &id) {
    auto user = static_cast<const SCEV*>(id.getUser());
    return codegenScev(user);
  }


  llvm::Value *MollyCodeGenerator::codegenLinearize(const isl::MultiPwAff &coord, const molly::AffineMapping *layout) {
    auto mapping = layout->getMapping();

    // toPwMultiAff() can give exponential number of pieces (in terms of dimensions)
    // alternatively, execute the two mappings sequentially: Fewer cases, but two levels of them
    auto coordMapping = mapping.pullback(coord.toPwMultiAff());
    auto value = codegenAff(coordMapping);
    //TODO: generate assert() to check the result being in the layout's buffer size
    return value;
}


llvm::Value *MollyCodeGenerator::codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices) {
  auto call = callLocalPtrIntrinsic(fvar, indices);
  return irBuilder.Insert(call, "ptr_local");
}


llvm::CallInst *MollyCodeGenerator::callCombufSend( molly::CommunicationBuffer *combuf ) {
  auto sendFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send);
  Value *args[] = { combuf->getVariableSend() };
  return this->irBuilder.CreateCall(sendFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecv(molly::CommunicationBuffer *combuf) {
    auto recvFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv);
  Value *args[] = { combuf->getVariableRecv() };
  return this->irBuilder.CreateCall(recvFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSendbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *dst) {
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_sendbuf_ptr);
  Value *args[] = { combuf->getVariableSend(), dst };
  return this->irBuilder.CreateCall(ptrFunc, args);
}


    llvm::CallInst *MollyCodeGenerator::callCombufRecvbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *src) {
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recvbuf_ptr);
  Value *args[] = { combuf->getVariableRecv(), src };
  return this->irBuilder.CreateCall(ptrFunc, args);      
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


void MollyCodeGenerator::codegenSend(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst) {
  auto dstCoords = codegenMultiAff(dst);
  //TODO: Use the dstCoords 
   callCombufSend(combuf);
}

void MollyCodeGenerator::codegenRecv(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src) {
  auto srcCoords = codegenMultiAff(src);
  //TODO: Use srcCoords
  callCombufRecv(combuf);
}


    llvm::Value *MollyCodeGenerator::codegenGetPtrSendBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst, const isl::MultiPwAff &index) {
     // Get the buffer to the destination
      auto rank = codegenLinearize(dst, combuf->getDstMapping());
    auto buf =  callCombufSendbufPtr(combuf, rank); 

    // Get the position within that buffer
      auto idx = codegenLinearize(index, combuf->getMapping());
      return irBuilder.CreateGEP(buf, idx);
    }
 
    
    llvm::Value *MollyCodeGenerator::codegenGetPtrRecvBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src, const isl::MultiPwAff &index) {
    // Get the buffer to the destination
      auto rank = codegenLinearize(src, combuf->getSrcMapping());
    auto buf = callCombufRecvbufPtr(combuf, rank); 

    // Get the position within that buffer
      auto idx = codegenLinearize(index, combuf->getMapping());
      return irBuilder.CreateGEP(buf, idx);
    }
