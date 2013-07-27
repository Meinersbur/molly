#include "MollyIntrinsics.h"

#include <llvm/IR/Intrinsics.h>
//#include <llvm/IR/IntrinsicInst.h>
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>

using namespace molly;
using namespace llvm;


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


void molly::codegenWriteLocal(IRBuilder<> &builder, FieldVariable *fvar, ArrayRef<Value*> indices, Value *val) {
  auto fty = fvar->getFieldType();
  auto funcPtrLocal = fty->getFuncPtrLocal();

  SmallVector<Value*,8> args;
  args.push_back(fvar->getVariable());
  args.append(indices.begin(), indices.end());
  auto ptr = builder.CreateCall(funcPtrLocal, args, "field_local_ptr");
  builder.CreateStore(val, ptr);
}
