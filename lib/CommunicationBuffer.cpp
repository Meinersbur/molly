#include "CommunicationBuffer.h"

#include "islpp/Set.h"
#include <llvm/IR/IRBuilder.h>
#include "FieldType.h"
#include <llvm/IR/GlobalVariable.h>

using namespace molly;
//using namespace polly;
using namespace llvm;
using namespace std;


llvm::Value *CommunicationBuffer::getSendBufferBase(DefaultIRBuilder &builder) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);

  auto ptr = builder.CreateConstGEP1_32(varsend,0, "combufsend_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}


llvm::Value *CommunicationBuffer::getRecvBufferBase(DefaultIRBuilder &builder) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);

  auto ptr = builder.CreateConstGEP1_32(varrecv,0, "combufrecv_base");
  auto bufbase = builder.CreateLoad(ptr);
  return bufbase;
}


void CommunicationBuffer::doLayoutMapping() {
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
}


llvm::Value *CommunicationBuffer::codegenReadFromBuffer(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value *> indices) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);

  auto linear = mapping->codegen(builder, params, indices);
  auto base = getRecvBufferBase(builder);
  auto ptr = builder.CreateGEP(base, linear, "local_ptr");

  return builder.CreateLoad(ptr, "field_val");
}


void CommunicationBuffer::codegenWriteToBuffer(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::Value *value, llvm::ArrayRef<llvm::Value *> indices) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);

  auto linear = mapping->codegen(builder, params, indices);
  auto base = getSendBufferBase(builder);
  auto ptr = builder.CreateGEP(base, linear, "local_ptr");

  builder.CreateStore(value,ptr);
}
