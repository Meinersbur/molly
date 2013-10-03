#include "MollyIntrinsics.h"

#include <llvm/IR/Intrinsics.h>
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include "CommunicationBuffer.h"
#include <llvm/IR/IRBuilder.h>
#include "MollyUtils.h"
#include <llvm/IR/Module.h>
//#include "ScopEditor.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


llvm::CallInst *molly::callLocalPtrIntrinsic(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, llvm::Instruction *insertBefore) {
  auto fty = fvar->getFieldType();
  assert(fty->getNumDimensions() == indices.size());
  auto nDims = indices.size();
  auto gvar = fvar->getVariable();
  auto module = gvar->getParent();

  SmallVector<Type*, 8> tys;
  tys.reserve(1+nDims);
  SmallVector<Value*, 8> args;
  args.reserve(nDims);

  // Return type
  tys.push_back(fty->getEltPtrType());

  // target field
  tys.push_back(fty->getType()->getPointerTo()); 
  args.push_back(fvar->getVariable());

  // coordinates
   for (auto i = nDims-nDims; i < nDims; i+=1) {
     auto idx = indices[i];
     assert(idx);
     tys.push_back(idx->getType());
     args.push_back(idx);
   }

  auto intrinsic = Intrinsic::getDeclaration(module, Intrinsic::molly_ptr_local, tys);
  auto result = CallInst::Create(intrinsic, args, "ptr_local", insertBefore);
  return result;
}


Value *molly::codegenReadLocal(IRBuilder<> &builder, FieldVariable *fvar, ArrayRef<Value*> indices) {
  auto fty = fvar->getFieldType();
  auto funcPtrLocal = fty->getFuncPtrLocal();

  //TODO: Care for if the field elements are structs, and we are setting just one element of it
  SmallVector<Value*,8> args;
  args.push_back(fvar->getVariable());
  args.append(indices.begin(), indices.end());
  auto ptr = builder.CreateCall(funcPtrLocal, args, "field_local_ptr");
  return builder.CreateLoad(ptr, "field_local_val");
} 


void molly::codegenWriteLocal(DefaultIRBuilder &builder, FieldVariable *fvar, ArrayRef<Value*> indices, Value *val) {
  auto fty = fvar->getFieldType();
  auto funcPtrLocal = fty->getFuncPtrLocal();

  SmallVector<Value*,8> args;
  args.push_back(fvar->getVariable());
  args.append(indices.begin(), indices.end());
  auto ptr = builder.CreateCall(funcPtrLocal, args, "field_local_ptr");
  builder.CreateStore(val, ptr);
}


void molly::codegenSendBuf(DefaultIRBuilder &builder, molly::CommunicationBuffer *buf, llvm::Value *dstCoord) {
  auto module = getModuleOf(builder.GetInsertBlock());
   auto sendFunc = module->getFunction("__molly_buf_send");

   auto var = buf->getVariableSend();
   auto fty = buf->getFieldType();
   auto rel = buf->getRelation(); /* { (src[coord] -> dst[coord]) -> field[indexset] } */

   builder.CreateCall(sendFunc, var);
}
