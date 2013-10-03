#include "CommunicationBuffer.h"

#include "islpp/Set.h"
#include <llvm/IR/IRBuilder.h>
#include "FieldType.h"
#include <llvm/IR/GlobalVariable.h>
#include <random>
#include "RectangularMapping.h"

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

  auto ptr = builder.CreateConstGEP1_64(varsend,0, "combufsend_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}


llvm::Value *CommunicationBuffer::getRecvBufferBase(DefaultIRBuilder &builder) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt64Ty(llvmContext);

  auto ptr = builder.CreateConstGEP1_64(varrecv,0, "combufrecv_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}   


isl::Space CommunicationBuffer::getDstNodeSpace() {
  return relation.getSpace().findNthSubspace(isl_dim_out, 1);
}


isl::Space CommunicationBuffer::getSrcNodeSpace() {
  return relation.getSpace().findNthSubspace(isl_dim_out, 0);
}


void CommunicationBuffer::doLayoutMapping() {
  assert(!mapping && "Call just once and then never change");
  assert(!sendbufMapping);
  assert(!recvbufMapping);
  auto paramsSpace = relation.getParamsSpace();
  auto nFieldDims = fty->getNumDimensions();

  // { (chunks[domain], src[cluster]) -> dst[cluster] }
  auto dstNodes = relation.wrap().reorganizeSubspaces(relation.getDomainSpace() >> getSrcNodeSpace(), getDstNodeSpace());
  sendbufMapping = RectangularMapping::createRectangualarHullMapping(dstNodes);

  // { (chunks[domain], dst[cluster]) -> src[cluster] }
  auto srcNodes = relation.wrap().reorganizeSubspaces(relation.getDomainSpace() >> getDstNodeSpace(), getSrcNodeSpace());
  recvbufMapping = RectangularMapping::createRectangualarHullMapping(srcNodes);

  // { (chunks[domain], src[cluster], dst[cluster]) -> field[indexset] }
  auto items = relation.uncurry();
  mapping = RectangularMapping:: createRectangualarHullMapping(items);


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


/// dstCoord: { (chunk[domain], dstNode[cluster]) -> [] }
/// index: { (chunk[domain], srcNode[cluster], dstNode[cluster]) -> [] }
llvm::Value *CommunicationBuffer::codegenPtrToSendBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index) {
  auto &irBuilder = codegen.getIRBuilder();

  auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto sendbufIdx = sendbufMapping->codegenIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtr = codegen.callCombufSendbufPtr(this, sendbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord)).toPwMultiAff();
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  return irBuilder.CreateGEP(sendbufPtr, indexIdx, "sendbufelt");
}


void CommunicationBuffer::codegenStoreInSendbuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index, llvm::Value *val) {
  auto &irBuilder = codegen.getIRBuilder();

  auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto sendbufIdx = sendbufMapping->codegenIndex(codegen, buftranslator, dstCoord);
  auto sendbufPtr = codegen.callCombufSendbufPtr(this, sendbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord)).toPwMultiAff();
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(sendbufPtr, indexIdx, "sendbufelt");

  auto myval = codegen.materialize(val);
  auto store = irBuilder.CreateStore(myval, ptr);

  // We symbolically write to the buffer object at the given coordinate
  codegen.addStoreAccess(this->getVariableSend(), rangeProduct(dstCoord, index), store);
}


llvm::Value *CommunicationBuffer::codegenPtrToRecvBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index) {
  auto &irBuilder = codegen.getIRBuilder();

  auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto recvbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto recvbufPtr = codegen.callCombufRecvbufPtr(this, recvbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord)).toPwMultiAff();
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  return irBuilder.CreateGEP(recvbufPtr, indexIdx, "recvbufelt");
}


llvm::Value *CommunicationBuffer::codegenLoadFromRecvBuf(MollyCodeGenerator &codegen,  isl::MultiPwAff chunk,  isl::MultiPwAff srcCoord,  isl::MultiPwAff dstCoord,  isl::MultiPwAff index) {
  auto &irBuilder = codegen.getIRBuilder();

  auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto recvbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  auto recvbufPtr = codegen.callCombufRecvbufPtr(this, recvbufIdx);

  auto idxtranslator = isl::rangeProduct(chunk, isl::rangeProduct(srcCoord, dstCoord)).toPwMultiAff();
  auto indexIdx = mapping->codegenIndex(codegen, idxtranslator, index);
  auto ptr = irBuilder.CreateGEP(recvbufPtr, indexIdx, "recvbufelt");

  auto result = irBuilder.CreateLoad(ptr, "recvelt");
  auto allIndices = getIndexsetSpace().universeBasicSet();
  codegen.addLoadAccess(this->getVariableSend(), rangeProduct(srcCoord, alltoall(buftranslator.getDomain(), allIndices)), result);
  return result;
}


llvm::Value *CommunicationBuffer::codegenSendWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = rangeProduct(chunk, srcCoord).toPwMultiAff();
  auto sendbufIdx = sendbufMapping->codegenIndex(codegen, buftranslator, dstCoord);
  return codegen.callCombufSendWait(this, sendbufIdx);
}


void CommunicationBuffer::codegenSend(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = isl::rangeProduct(chunk, srcCoord).toPwMultiAff(); // { [domain] -> (chunk[domain], srcNode[cluster]) }
  auto sendbufIdx = sendbufMapping->codegenIndex(codegen, buftranslator, dstCoord/* { [domain] -> dstNode[cluster] } */);
  auto call = codegen.callCombufSend(this, sendbufIdx);

  // Add artificial accesses to the buffer, the runtime will access it. This is to inhibit other accesses from moving cross it
  auto allIndices = getIndexsetSpace().universeBasicSet(); // Could also be exact, but there is not point in doing it
  codegen.addLoadAccess(this->getVariableSend(), rangeProduct(dstCoord, alltoall(buftranslator.getDomain(), allIndices)), call);
}


llvm::Value *CommunicationBuffer::codegenRecvWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = rangeProduct(chunk, dstCoord).toPwMultiAff();
  auto sendbufIdx = recvbufMapping->codegenIndex(codegen, buftranslator, srcCoord);
  return codegen.callCombufRecvWait(this, sendbufIdx);
}


void CommunicationBuffer::codegenRecv(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord) {
  auto buftranslator = isl::rangeProduct(chunk, dstCoord).toPwMultiAff();
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


llvm::Value *CommunicationBuffer::codegenPtrToSendbufObj(MollyCodeGenerator &codegen) {
  auto var = getVariableSend();
auto result = codegen.getIRBuilder().CreateLoad(var, "sendbufobj");
codegen.addScalarLoadAccess(var, result);
return result;
}


llvm::Value *CommunicationBuffer::codegenPtrToRecvbufObj(MollyCodeGenerator &codegen) {
   auto var = getVariableRecv();
  auto result = codegen.getIRBuilder().CreateLoad(var, "recvbufobj");
  codegen.addScalarLoadAccess(var, result);
  return result;
}
