#include "RectangularMapping.h"

#include <llvm/IR/IRBuilder.h>

using namespace molly;
using namespace llvm;


int RectangularMapping:: map(ArrayRef<unsigned> coords) const {
  assert(coords.size() == lengths.size());

  // Row-major
  auto nDims = lengths.size();
  int result = coords[0]- offsets[0];
  for (auto i = 1; i<nDims;i+=1) {
    assert(coords[i] >= offsets[i]);
    result = result * lengths[i-1] + (coords[i] - offsets[i]);
  }
  return result;
}


Value *RectangularMapping::codegen(llvm::IRBuilder<> &builder, llvm::ArrayRef<llvm::Value*> coords) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);
  auto nDims = getInputDims();
  assert(coords.size() == nDims);

 auto result = builder.CreateSub(coords[0], ConstantInt::get(intTy, offsets[0] ));
  for (auto i = 1; i<nDims; i+=1) {
    auto horn = builder.CreateMul(result, ConstantInt::get(intTy, lengths[i-1]));
    auto idx = builder.CreateSub(coords[i], ConstantInt::get(intTy, offsets[i]));
    result = builder.CreateAdd(horn, idx);
  }
  return result;
}
