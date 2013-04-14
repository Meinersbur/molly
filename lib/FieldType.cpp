
#include "FieldType.h"

#include "MollyContext.h"
#include "islpp/Ctx.h"
#include "islpp/Set.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/BasicSet.h"
#include "islpp/Constraint.h"

#include <isl/space.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>


#include <assert.h>

using namespace llvm;
using namespace molly;
using std::move;






isl::Ctx *FieldType::getIslContext() {
  return mollyContext->getIslContext();
}


llvm::LLVMContext *FieldType::getLLVMContext() {
  return mollyContext->getLLVMContext();
}


llvm::Module *FieldType::getModule() {
  return module;
}


FieldType::~FieldType() {
  int a = 0;
}


isl::Set FieldType::getLogicalIndexset() {
  auto ctx = getIslContext();

  auto dims = getNumDimensions();
  auto space = ctx->createSpace(0, dims);
  auto set = isl::BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = isl::Constraint::createInequality(space.copy());
    ge.setConstant(0);
    ge.setCoefficient(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = isl::Constraint::createInequality(move(space));
    lt.setConstant(getLengths()[d]);
    lt.setCoefficient(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }

  return set;
}


void FieldType::dump() {
  getLogicalIndexset().dump();
}



#if 0
llvm::Function *FieldType::getIsLocalFunc() {
  if (!islocalfunc) {
    // function has been optimized away; We simply create a new one
    auto llvmContext = getLLVMContext();

    // Prototype
    Type *params[] = {
      Type::getInt32Ty(*llvmContext) //TODO: Clang may have "int" int to some other type
    };
    auto functy = FunctionType::get(Type::getInt1Ty(*llvmContext), params, false);

    islocalfunc = Function::Create(functy, GlobalValue::ExternalLinkage, "isLocal", getModule());
  }
  return islocalfunc;
}
#endif

static Value *emit(IRBuilderBase &builder, Value *value) {
  return value;
}


static Value *emit(IRBuilderBase &builder, uint32_t value) {
  return Constant::getIntegerValue(Type::getInt32Ty(builder.getContext()), APInt(32, (uint64_t)value, false)) ;
}


static Value *emit(IRBuilderBase &builder, bool value) {
  return Constant::getIntegerValue(Type::getInt32Ty(builder.getContext()), APInt(1, (uint64_t)value, false)) ;
}

template<typename T>
SmallVector<T*,4> iplistToSmallVector(iplist<T> &list) {
  SmallVector<T*,4> result;
  for (auto it = list.begin(), end = list.end(); it!=end;++it) {
    result.push_back(&*it);
  }
  return result;
}


#if 0
void FieldType::emitIsLocalFunc() {
  auto &llvmContext = module->getContext();
  auto func = getIsLocalFunc();
  func->getBasicBlockList().clear(); // Remove any existing implementation; Alternative: Delete old one, create new one

  auto entryBB = BasicBlock::Create(llvmContext, "Entry", func);
  IRBuilder<> builder(entryBB);

  if (!isdistributed) {
    auto trueConst = emit(builder, true);
    builder.CreateRet(trueConst);
    return;
  }

  auto args = iplistToSmallVector(func->getArgumentList());
  GlobalVariable *coordVar = module->getGlobalVariable("_cart_local_coord");
  assert(coordVar);
  //auto coordAddr = builder.CreateInBoundsGEP(coordVar, emit(builder, (uint32_t)0));
  auto coordAddr = builder.CreateConstInBoundsGEP2_32(coordVar, 0, 0);
  auto coord = builder.CreateLoad(coordAddr, "coord");
  auto localLength = emit(builder, localLengths[0]);
  auto origin = builder.CreateNSWMul(coord, localLength);
  auto end = builder.CreateAdd(origin, localLength);

  auto lower = builder.CreateICmpUGE(origin, args[1]);
  auto higher = builder.CreateICmpUGT(args[1], end);
  auto inregion = builder.CreateAnd(lower, higher);

  builder.CreateRet(inregion);
}
#endif
