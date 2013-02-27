
#include "FieldType.h"

#include "MollyContext.h"
#include "islpp/Ctx.h"
#include "islpp/Set.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/BasicSet.h"
#include "islpp/Constraint.h"

#include <isl/space.h>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/ArrayRef.h>

#include <assert.h>

using namespace llvm;
using namespace molly;
using std::move;


/// NamedMDNode "molly.fields": List of MDNodes describing a FieldType
/// element MDNode:
/// [0] magic constant string "field"
/// [1] clang type name string
/// [2] llvm type name string
/// [3] int64 llvm unique typeid
/// [4] MDNode of dimension lengths
/// [5] ref function
/// [6] isLocal function
void FieldType::readMetadata() {
  assert(metadata);

  auto magic = dyn_cast<MDString>(metadata->getOperand(0));
  assert(magic->getString() == "field");

  auto clangNameMD = dyn_cast<MDString>(metadata->getOperand(1));
  auto clangName = clangNameMD->getString();

  auto llvmNameMD = dyn_cast<MDString>(metadata->getOperand(2));
  auto llvmName = llvmNameMD->getString();

  auto typeidMD = dyn_cast<llvm::ConstantInt>(metadata->getOperand(3));
  auto tid = typeidMD->getLimitedValue();

  auto lengthsMD = dyn_cast<llvm::MDNode>(metadata->getOperand(4));
  auto dims = lengthsMD->getNumOperands();

  lengths.clear();
  for (unsigned i = 0; i < dims; i+=1) {
    auto lenMD = dyn_cast<llvm::ConstantInt>(lengthsMD->getOperand(i));
    auto len = lenMD->getLimitedValue();
    lengths.push_back(len);
  }

  ty = module->getTypeByName(llvmName);


  this->reffunc = cast_or_null<Function>(metadata->getOperand(5));
  this->islocalfunc = cast_or_null<Function>(metadata->getOperand(6));
}


isl::Ctx *FieldType::getIslContext() {
  return mollyContext->getIslContext();
}

llvm::LLVMContext *FieldType::getLLVMContext() {
  return mollyContext->getLLVMContext();
}

llvm::Module *FieldType::getModule() {
  return module;
}


isl::Set FieldType::getIndexset() {
  auto ctx = getIslContext();

  auto dims = getDimensions();
  auto space = ctx->createSpace(0, dims);
  auto set = isl::BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = isl::Constraint::createInequality(space.copy());
    ge.setConstant(0);
    ge.setCoefficient(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = isl::Constraint::createInequality(move(space));
    lt.setConstant(lengths[d]);
    lt.setCoefficient(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }

  return set;
}


void FieldType::dump() {
  getIndexset().dump();
}


llvm::Function *FieldType::getRefFunc() {
  return reffunc;
}


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