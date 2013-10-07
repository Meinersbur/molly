#include "ClusterConfig.h"

#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/ArrayRef.h>
#include "MollyUtils.h"
#include "llvm/IR/Function.h"
#include <llvm/ADT/Twine.h>

using namespace molly;
using namespace llvm;


isl::Space ClusterConfig::getClusterSpace() const {
  return clusterShape.getSpace();
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
    auto module = getModuleOf(builder);

    SmallVector<Type*,4> argTys(getClusterDims(), intTy);
    auto funcTy = FunctionType::get(intTy,argTys, false);
    funcCoordToRank = Function::Create(funcTy, GlobalValue::PrivateLinkage, "Coord2Rank", module);
    //FIXME: Call MPI_Cart_rank
  }

  auto result = builder.CreateCall(funcCoordToRank, coords);
  return result;
}


isl::Id molly::ClusterConfig::getClusterDimId( isl::pos_t d ) {
  return islctx->createId("rankdim" + Twine(d), this);
}


isl::MultiAff molly::ClusterConfig::getClusterLengthsAff() const {
  auto nDims = getClusterDims();
  auto space = islctx->createMapSpace(0, 0, nDims);
  auto result = space.createZeroMultiAff();
  auto domspace = space.getDomainSpace();
  for (auto d = nDims-nDims; d<nDims; d+=1) {
    result.setAff_inplace(d, domspace.createConstantAff(getClusterLength(d)));
  }
  return result;
}
