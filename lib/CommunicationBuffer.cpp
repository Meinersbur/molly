#include "CommunicationBuffer.h"

#include "islpp/Set.h"
#include <llvm/IR/IRBuilder.h>
#include "FieldType.h"
#include <llvm/IR/GlobalVariable.h>
#include <random>
#include "RectangularMapping.h"
#include <polly/ScopInfo.h>
#include "MollyPassManager.h"
#include "MollyScopStmtProcessor.h"
#include "ClusterConfig.h"
#include "MollyFunctionProcessor.h"
#include "MollyScopProcessor.h"

using namespace molly;
//using namespace polly;
using namespace llvm;
using namespace std;


CommunicationBuffer::~CommunicationBuffer() {
  delete mapping;
}


llvm::Value *CommunicationBuffer::getSendBufferBase(DefaultIRBuilder &builder) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt64Ty(llvmContext);

  auto ptr = builder.CreateConstGEP1_64(varsend, 0, "combufsend_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}


llvm::Value *CommunicationBuffer::getRecvBufferBase(DefaultIRBuilder &builder) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt64Ty(llvmContext);

  auto ptr = builder.CreateConstGEP1_64(varrecv, 0, "combufrecv_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}


isl::Space CommunicationBuffer::getDstNodeSpace() {
  return relation.getSpace().findNthSubspace(isl_dim_out, 1).resetDimIds(isl_dim_set);
}


isl::Space CommunicationBuffer::getSrcNodeSpace() {
  return relation.getSpace().findNthSubspace(isl_dim_out, 0).resetDimIds(isl_dim_set);
}


void CommunicationBuffer::doLayoutMapping() {
  assert(!mapping && "Call just once and then never change");
  assert(!sendbufMapping);
  assert(!recvbufMapping);
  auto paramsSpace = relation.getParamsSpace();
  auto nFieldDims = fty->getNumDimensions();

  // { src[cluster] -> dst[cluster] }
  auto dstNodes = relation.wrap().reorganizeSubspaces(getSrcNodeSpace(), getDstNodeSpace());
  sendbufMapping = RectangularMapping::createRectangualarHullMapping(dstNodes);

  // { dst[cluster] -> src[cluster] }
  auto srcNodes = relation.wrap().reorganizeSubspaces(getDstNodeSpace(), getSrcNodeSpace());
  recvbufMapping = RectangularMapping::createRectangualarHullMapping(srcNodes);

  // { (chunks[domain], src[cluster], dst[cluster]) -> field[indexset] }
  auto items = relation.uncurry();
  mapping = RectangularMapping::createRectangualarHullMapping(items);


#if 0
  assert(!mapping);
  assert(countElts.isNull());

  auto range = this->relation.getRange(); /* { field[indexset] } */
  auto space = range.getSpace();
  auto nDims = range.getDimCount();

  auto aff = space.createZeroPwAff();
  auto count = space.createConstantAff(1).toPwAff();
  isl::PwAff prevLen;
  for (auto i = nDims-nDims; i<nDims; i+=1) {
    auto min = range.dimMin(i);
    auto max = range.dimMax(i);
    auto len = max - min;

    // row-major order
    if (prevLen.isValid())
      aff *= prevLen;
    aff += space.createVarAff(isl_dim_set, i) - min;

    count *= len;
    prevLen = len;
  }

  this->mapping = new AffineMapping(aff.move());
  this->countElts = count;
#endif
}


void CommunicationBuffer::codegenInit(MollyCodeGenerator &codegen, MollyPassManager *pm, MollyFunctionProcessor *funcCtx) {
  auto pass = funcCtx->asPass();
  auto dlp = &pass->getAnalysis<DataLayoutPass>();
  auto eltSizeQuery = dlp ? &dlp->getDataLayout() : NULL;
  auto &llvmContext = codegen.getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto &builder = codegen.getIRBuilder();
  auto clusterConf = pm->getClusterConfig();
  auto islctx = getIslContext();
  //auto intTy = Type::getInt64Ty(llvmContext);
  auto nClusterDims = clusterConf->getClusterDims();

  auto eltTy = fty->getEltType();
  auto eltSize = eltSizeQuery->getTypeAllocSize(eltTy);
  auto eltSizeVal = ConstantInt::get(intTy, eltSize);
  auto tagVal = ConstantInt::get(intTy, this->tag);

  auto srcRankSpace = getSrcNodeSpace();
  auto dstRankSpace = getDstNodeSpace();

  auto srcDstRelation = wrap(relation).reorderSubspaces(srcRankSpace, dstRankSpace);
  auto dstSrcRelation = srcDstRelation.reverse();

  auto selfCoord = funcCtx->getCurrentNodeCoordinate();
  auto srcSelfRank = selfCoord.castRange(srcRankSpace);
  auto dstSelfRank = selfCoord.castRange(dstRankSpace);

  auto coords = builder.CreateAlloca(intTy, ConstantInt::get(intTy, nClusterDims), "coords");
  auto nClusterDimsVal = ConstantInt::get(intTy, nClusterDims);

  {
    auto nDst = codegenNumDests(codegen, srcSelfRank); // Max over all chunks; TODO: Skip everything if nDst is zero (at runtime)
    auto sendobj = codegen.callRuntimeCombufSendAlloc(nDst, eltSizeVal, tagVal);
    builder.CreateStore(sendobj, getVariableSend());


    auto instrAfterScop = &*builder.GetInsertPoint();
    auto scopEd = ScopEditor::newScop(islctx, instrAfterScop, pass);
    auto scop = scopEd.getScop();
    auto scopInfo = new polly::ScopInfo(islctx->keep()); // Create it ourselfves; no need to actually run it, but need to give it out islctx
    scopInfo->setScop(scop); // Force to take this scop without trying to detect it
    pm->registerEvaluatedAnalysis(scopInfo, &scop->getRegion());
    auto scopCtx = pm->getScopContext(scop);


    auto sendSelfCoord = scopCtx->getCurrentNodeCoordinate(); // Required; will add coordinates of the current node to the context
    auto sendSrcSelfRank = sendSelfCoord.castRange(srcRankSpace);
    auto sendDstSelfRank = sendSelfCoord.castRange(dstRankSpace);


    auto scatterId = scopEd.getScatterTupleId();
    auto voidSpace = islctx->createSetSpace(0, 0);
    auto scatterSpace = voidSpace.setSetTupleId(scatterId);

    {
      auto sendDomainSpace = dstRankSpace;
      auto sendDomain = sendSrcSelfRank.getRange().apply(srcDstRelation); // { dstNode[cluster] }
      auto sendScattering = sendDomain.getSpace().mapsTo(scatterSpace).createZeroMultiAff(); // { dstNode[cluster] -> scattering[] }
      auto sendWhere = dstSrcRelation; // { dstNode[cluster] -> srcNode[cluster] }
      auto sendStmtEd = scopEd.createStmt(sendDomain, sendScattering, sendWhere, "send_dst_init");
      auto sendStmt = sendStmtEd.getStmt();
      auto sendBB = sendStmt->getBasicBlock();
      auto sendStmtCtx = pm->getScopStmtContext(sendStmt);
      auto sendCodegen = sendStmtCtx->makeCodegen();
      auto &sendBuilder = sendCodegen.getIRBuilder();

      auto sendSrcAff = sendStmtCtx->getClusterPwMultiAff().castRange(srcRankSpace);
      auto combufSend = sendCodegen.createScalarLoad(getVariableSend(), "combuf_send");
      //sendCodegen.addScalarLoadAccess()
      //auto combufSend = sendCodegen.materialize();
      auto curIteration = sendStmtEd.getCurrentIteration().intersectDomain_consume(sendStmtCtx->getDomain());
      auto sendDstAff = curIteration.castRange(dstRankSpace); /* { [domain] -> dstNode[cluster] } */
      //auto sendDstRank = clusterConf->codegenRank(sendCodegen, sendDstAff); //TODO: this is a global rank, but we actually need one indexed from (0..maxdst]
      //auto sendDstRank = sendbufMapping->codegenIndex(sendCodegen, sendSrcAff, sendDstAff);
      auto sendDstRank = codegenSendbufDstIndex(sendCodegen, sendSrcAff, sendDstAff);
      auto sendWhat = rangeProduct(sendSrcAff, sendDstAff);
      auto sendSize = mapping->codegenMaxSize(sendCodegen, sendWhat); // Max over all chunks

      // Write the dst node coordinates to the stack such that MollyRT knows who is the target node
      // sendDstRank just provides a meaningless number
      for (auto i = nClusterDims - nClusterDims; i < nClusterDims; i += 1) {
        sendCodegen.createArrayStore(sendStmtCtx->getDomainValue(i), coords, i);
      }

      auto tagVal = ConstantInt::get(intTy, tag);
      sendCodegen.callRuntimeCombufSendDstInit(combufSend, sendDstRank, nClusterDimsVal, coords, sendSize, tagVal);
    }

    // Now we constructed a SCoP, so tell Polly to generate the code for it...
    // Do not forget that this command will change a lot of BasicBlocks
    scopCtx->pollyCodegen();
    builder.SetInsertPoint(instrAfterScop);
  }




  {
    //auto nSrc = recvbufMapping->codegenMaxSize(codegen, dstSelfRank); // Max over all chunks; TODO: Skip at runtime if zero
    auto nSrc = codegenNumSrcs(codegen, dstSelfRank);
    auto recvobj = codegen.callRuntimeCombufRecvAlloc(nSrc, eltSizeVal, tagVal);
    builder.CreateStore(recvobj, getVariableRecv());

    auto instrAfterScop = &*builder.GetInsertPoint();
    auto scopEd = ScopEditor::newScop(islctx, instrAfterScop, pass);
    auto scop = scopEd.getScop();
    auto scopInfo = new polly::ScopInfo(islctx->keep()); // Create it ourselfves; no need to actually run it, but need to give it out islctx
    scopInfo->setScop(scop); // Force to take this scop without trying to detect it
    pm->registerEvaluatedAnalysis(scopInfo, &scop->getRegion());
    auto scopCtx = pm->getScopContext(scop);


    auto recvSelfCoord = scopCtx->getCurrentNodeCoordinate(); // Required; will add coordinates of the current node to the context
    auto recvSrcSelfRank = recvSelfCoord.castRange(srcRankSpace);
    auto recvDstSelfRank = recvSelfCoord.castRange(dstRankSpace);


    auto scatterId = scopEd.getScatterTupleId();
    auto voidSpace = islctx->createSetSpace(0, 0);
    auto scatterSpace = voidSpace.setSetTupleId(scatterId);


    {
      auto recvDomainSpace = srcRankSpace;
      auto recvDomain = recvDstSelfRank.getRange().apply(dstSrcRelation); // { srcNode[cluster] }
      auto recvScattering = recvDomain.getSpace().mapsTo(scatterSpace).createZeroMultiAff(); // { srcNode[cluster] -> scattering[] }
      auto recvWhere = srcDstRelation; // { dstNode[cluster] -> srcNode[cluster] }
      auto recvStmtEd = scopEd.createStmt(recvDomain, recvScattering, recvWhere, "recv_src_init");
      auto recvStmt = recvStmtEd.getStmt();
      auto recvBB = recvStmt->getBasicBlock();
      auto recvStmtCtx = pm->getScopStmtContext(recvStmt);
      auto recvCodegen = recvStmtCtx->makeCodegen();
      auto &recvBuilder = recvCodegen.getIRBuilder();

      auto recvDstAff = recvStmtCtx->getClusterMultiAff().castRange(dstRankSpace);
      auto combufRecv = recvCodegen.createScalarLoad(getVariableRecv(), "combuf_recv");
      //auto combufRecv = recvCodegen.materialize(getVariableRecv());
      auto recvSrcAff = recvStmtEd.getCurrentIteration().castRange(srcRankSpace); /* { [domain] -> srcNode[cluster] } */
      //auto recvDstRank = clusterConf->codegenRank(recvCodegen, recvSrcAff);
      // auto recvSrcRank = recvbufMapping->codegenIndex(recvCodegen, recvDstAff, recvSrcAff);
      auto recvSrcRank = codegenRecvbufSrcIndex(recvCodegen, recvDstAff, recvSrcAff);

      auto recvWhat = rangeProduct(recvDstAff, recvSrcAff);
      auto recvSize = mapping->codegenMaxSize(recvCodegen, recvWhat); // Max over all chunks

      for (auto i = nClusterDims - nClusterDims; i < nClusterDims; i += 1) {
        recvCodegen.createArrayStore(recvStmtCtx->getDomainValue(i), coords, i);
      }

      auto tagVal = ConstantInt::get(intTy, tag);
      recvCodegen.callRuntimeCombufRecvSrcInit(combufRecv, recvSrcRank, nClusterDimsVal, coords, recvSize, tagVal);
    }

    // Now we constructed a SCoP, so tell Polly to generate the code for it...
    // Do not forget that this command will change a lot of BasicBlocks
    scopCtx->pollyCodegen();
    builder.SetInsertPoint(instrAfterScop);
  }

}


/// dstCoord: { (chunk[domain], dstNode[cluster]) -> [] }
/// index: { (chunk[domain], srcNode[cluster], dstNode[cluster]) -> [] }
llvm::Value *CommunicationBuffer::codegenPtrToSendBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index) {
  auto &irBuilder = codegen.getIRBuilder();

  //auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto buftranslator = srcCoord;
  auto sendbufIdx = sendbufMapping->codegenIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtr = codegen.callCombufSendbufPtr(this, sendbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  return irBuilder.CreateGEP(sendbufPtr, indexIdx, "sendbufelt");
}


void CommunicationBuffer::codegenStoreInSendbuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index, llvm::Value *val) {
  auto &irBuilder = codegen.getIRBuilder();

  //auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto buftranslator = srcCoord;
  auto sendbufIdx = codegenSendbufDstIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtr = codegen.callCombufSendbufPtr(this, sendbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(sendbufPtr, indexIdx, "sendbufelt");


  auto myval = codegen.materialize(val);
  auto store = irBuilder.CreateStore(myval, ptr);
  // We symbolically write to the buffer object at the given coordinate
  codegen.addStoreAccess(sendbufPtr, index, store);
}


llvm::Value *CommunicationBuffer::codegenSendbufPtrPtr(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = srcCoord;
  auto sendbufIdx = codegenSendbufDstIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtrPtr = codegen.getSendbufPtrPtr(this, sendbufIdx);
  return sendbufPtrPtr;
}


AnnotatedPtr CommunicationBuffer::codegenSendbufPtr(MollyCodeGenerator &codegen, const isl::MultiPwAff chunk, const isl::MultiPwAff srcCoord, const isl::MultiPwAff dstCoord, const isl::MultiPwAff index) {
  auto &irBuilder = codegen.getIRBuilder();

  //auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff();
  //auto buftranslator = srcCoord.toPwMultiAff();
  //auto sendbufIdx = codegenSendbufDstIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtrPtr = codegenSendbufPtrPtr(codegen, chunk, srcCoord, dstCoord);
  auto sendbufPtr = irBuilder.CreateLoad(sendbufPtrPtr); //codegen.callCombufSendbufPtr(this, sendbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(sendbufPtr, indexIdx, "sendbufelt");

  // return AnnotatedPtr::createArrayPtr(ptr, sendbufPtr, index); // FIXME: sendbufPtr as base might be a problem because it is the return value of __molly_combuf_send_wait
  return AnnotatedPtr::createUnannotated(ptr); // Combufs are not user-accessible, any dependency must be handled by the caller; In Molly's case, accesses never overlap, there only must be deps from send_wait and to send
  //FIXME: When Polly reorder optimization is activated, some dependendencies MUST be added
}


llvm::Value *CommunicationBuffer::codegenRecvbufPtrPtr(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = dstCoord;
  auto recvbufIdx = codegenRecvbufSrcIndex(codegen, buftranslator, srcCoord); //recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto recvbufPtrPtr = codegen.getRecvbufPtrPtr(this, recvbufIdx);
  return recvbufPtrPtr;
}


AnnotatedPtr CommunicationBuffer::codegenRecvbufPtr(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord, isl::MultiPwAff index) {
  auto &irBuilder = codegen.getIRBuilder();

  auto recvbufPtrPtr = codegenRecvbufPtrPtr(codegen, chunk, srcCoord, dstCoord);
  auto recvbufPtr = irBuilder.CreateLoad(recvbufPtrPtr);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(recvbufPtr, indexIdx, "recvbufelt");

  return AnnotatedPtr::createUnannotated(ptr);
}


llvm::Value *CommunicationBuffer::codegenPtrToRecvBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index) {
  auto &irBuilder = codegen.getIRBuilder();

  //auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto buftranslator = dstCoord;
  auto recvbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto recvbufPtr = codegen.callCombufRecvbufPtr(this, recvbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  return irBuilder.CreateGEP(recvbufPtr, indexIdx, "recvbufelt");
}


llvm::Value *CommunicationBuffer::codegenLoadFromRecvBuf(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord, isl::MultiPwAff index) {
  auto &irBuilder = codegen.getIRBuilder();

  //auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto buftranslator = dstCoord;
  auto recvbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto recvbufPtr = codegen.callCombufRecvbufPtr(this, recvbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord));
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(recvbufPtr, indexIdx, "recvbufelt");

  auto result = irBuilder.CreateLoad(ptr, "recvelt");
  //auto allIndices = getIndexsetSpace().universeBasicSet();
  codegen.addLoadAccess(recvbufPtr, index/*symbolic, not the real offset*/, result);
  return result;
}


llvm::Value *CommunicationBuffer::codegenSendWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  //auto buftranslator = rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto buftranslator = srcCoord;
  auto sendbufIdx = codegenSendbufDstIndex(codegen, buftranslator, dstCoord);
  return codegen.callCombufSendWait(this, sendbufIdx);
}


void CommunicationBuffer::codegenSend(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  // auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff(); // { [domain] -> (chunk[domain], srcNode[cluster]) }
  auto buftranslator = srcCoord;
  auto sendbufIdx = codegenSendbufDstIndex(codegen, buftranslator, dstCoord/* { [domain] -> dstNode[cluster] } */);
  auto call = codegen.callCombufSend(this, sendbufIdx);

  // Add artificial accesses to the buffer, the runtime will access it. This is to inhibit other accesses from moving cross it
  auto allIndices = getIndexsetSpace().universeBasicSet(); // Could also be exact, but there is not point in doing it
  codegen.addLoadAccess(this->getVariableSend(), rangeProduct(dstCoord, alltoall(buftranslator.getDomain(), allIndices)), call);
}


llvm::Value *CommunicationBuffer::codegenRecvWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  //auto buftranslator = rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto buftranslator = dstCoord;
  auto sendbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  return codegen.callCombufRecvWait(this, sendbufIdx);
}


void CommunicationBuffer::codegenRecv(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  //auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto buftranslator = dstCoord;
  auto sendbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto call = codegen.callCombufRecv(this, sendbufIdx);

  auto allIndices = relation.getSpace().findNthSubspace(isl_dim_out, 2).universeBasicSet();
  codegen.addStoreAccess(this->getVariableRecv(), rangeProduct(srcCoord, alltoall(buftranslator.getDomain(), allIndices)), call);
}


llvm::Type * molly::CommunicationBuffer::getEltType() const {
  return getFieldType()->getEltType();
}


llvm::PointerType *molly::CommunicationBuffer::getEltPtrType() const {
  return getFieldType()->getEltPtrType();
}


isl::Space molly::CommunicationBuffer::getDstNamedDims() {
  auto result = getDstNodeSpace();
  auto islctx = result.getCtx();
  auto nDims = result.getSetDimCount();
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto id = islctx->createId("dstdim" + Twine(i));
    result.setDimId_inplace(isl_dim_set, i, id);
  }
  return result;
}


isl::Space molly::CommunicationBuffer::getSrcNamedDims() {
  auto result = getDstNodeSpace();
  auto islctx = result.getCtx();
  auto nDims = result.getSetDimCount();
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto id = islctx->createId("srcdim" + Twine(i));
    result.setDimId_inplace(isl_dim_set, i, id);
  }
  return result;
}


llvm::Value *molly::CommunicationBuffer::codegenNumDests(MollyCodeGenerator &codegen, isl::PwMultiAff srcSelfRank) {
  return sendbufMapping->codegenMaxSize(codegen, srcSelfRank.castRange(getSrcNodeSpace()));
}


llvm::Value *molly::CommunicationBuffer::codegenNumSrcs(MollyCodeGenerator &codegen, isl::PwMultiAff dstSelfRank) {
  return recvbufMapping->codegenMaxSize(codegen, dstSelfRank.castRange(getDstNodeSpace()));
}


llvm::Value *CommunicationBuffer::codegenPtrToSendbufObj(MollyCodeGenerator &codegen) {
  auto var = getVariableSend();
  auto func = codegen.getParentFunction();
  auto entry = &func->getEntryBlock();
  auto ptr = new LoadInst(var, "sendbufobj", entry->getFirstInsertionPt());

  return codegen.materialize(ptr);
  //auto result = codegen.getIRBuilder().CreateLoad(ptr, "sendbufobj");
  //codegen.addScalarLoadAccess(var, result);
  //return result;
}


llvm::Value *CommunicationBuffer::codegenPtrToRecvbufObj(MollyCodeGenerator &codegen) {
  auto var = getVariableRecv();
  auto func = codegen.getParentFunction();
  auto entry = &func->getEntryBlock();
  auto ptr = new LoadInst(var, "recvbufobj", entry->getFirstInsertionPt());

  return codegen.materialize(ptr);
  //auto result = codegen.getIRBuilder().CreateLoad(var, "recvbufobj");
  //codegen.addScalarLoadAccess(var, result);
  //return result;
}


llvm::Value *CommunicationBuffer::codegenSendbufDstIndex(MollyCodeGenerator &codegen, isl::MultiPwAff domain, isl::MultiPwAff dstCoord) {
  auto result = sendbufMapping->codegenIndex(codegen, domain, dstCoord);
  return result;
}


llvm::Value *CommunicationBuffer::codegenRecvbufSrcIndex(MollyCodeGenerator &codegen, isl::MultiPwAff domain, isl::MultiPwAff srcCoord) {
  auto result = recvbufMapping->codegenIndex(codegen, domain, srcCoord);
  return result;
}
