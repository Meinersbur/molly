#include "ClusterConfig.h"

#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/ArrayRef.h>
#include "MollyUtils.h"
#include "llvm/IR/Function.h"

using namespace molly;
using namespace llvm;


isl::Space ClusterConfig::getClusterSpace() const {
  auto result = getIslContext()->createSetSpace(0, clusterLengths.size());
  result.setSetTupleId_inplace(getClusterTuple());
  return result;
}


unsigned ClusterConfig::getClusterSize() const {
  int result = 1;
  for (auto it = clusterLengths.begin(), end = clusterLengths.end(); it != end; ++it) {
    auto len = *it;
    result *= len;
  }
  assert(result >= 1);
  return result;
}


isl::MultiAff ClusterConfig::getMasterRank() const {
  auto islctx = getIslContext();
  auto nDims = getClusterDims();

  return isl::MultiAff::createZero(getClusterSpace());
}


llvm::Value *ClusterConfig::codegenComputeRank(DefaultIRBuilder &builder, ArrayRef<Value*> coords) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);

  if (!funcCoordToRank) {
    auto module = getParentModule(builder);

    SmallVector<Type*,4> argTys(getClusterDims(), intTy);
   auto funcTy = FunctionType::get(intTy,argTys, false);
    funcCoordToRank = Function::Create(funcTy, GlobalValue::PrivateLinkage, "Coord2Rank", module);
    //FIXME: Call MPI_Cart_rank
  }

auto result = builder.CreateCall(funcCoordToRank, coords);
return result;
}
