
#include "FieldType.h"

#include "MollyContext.h"
#include "IslCtx.h"
#include "IslSet.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

#include <assert.h>

using namespace llvm;
using namespace molly;



void molly::FieldType::readMetadata() {
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

}

IslSet &molly::FieldType::getIndexset() {
  
  //mollyContext->getIslContext()->

}
