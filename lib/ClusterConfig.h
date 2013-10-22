//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Stores the geometry of the nodes in the cluster
///
//===----------------------------------------------------------------------===//

#ifndef MOLLY_CLUSTERCONFIG_H
#define MOLLY_CLUSTERCONFIG_H

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ArrayRef.h>
#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include "islpp/Id.h"
#include "islpp/BasicSet.h"
#include "molly/Mollyfwd.h"


namespace molly {
  class ClusterConfig {
  private:
    isl::Ctx *islctx;
    isl::Id nodecoord;

    llvm::SmallVector<unsigned,4> clusterLengths;
    isl::BasicSet clusterShape;
    llvm::Function *funcCoordToRank;

  protected:
    isl::Ctx *getIslContext() const { assert(islctx); return islctx; }

  public:
    ClusterConfig(isl::Ctx *islctx) : islctx(islctx), funcCoordToRank(nullptr) {
      assert(islctx);
      nodecoord = islctx->createId("rank");
    }

    /// TupleId for node coordinates in this cluster 
    const isl::Id &getClusterTuple() const { return nodecoord; }
    isl::Space getClusterSpace() const;
    isl::Space getClusterParamSpace() const { return getClusterSpace().moveDims(isl_dim_param, clusterShape.getParamDimCount(), isl_dim_set, 0,clusterShape.getDimCount()).params(); }
    
    void setClusterLengths(llvm::ArrayRef<unsigned> lengths) {
      clusterLengths.clear();
      clusterLengths.append(lengths.begin(), lengths.end());
       auto nDims = lengths.size();

      auto space = islctx->createSetSpace(0, nDims);
      space.setSetTupleId_inplace(getClusterTuple());
      for (auto d=nDims-nDims;d<nDims;d+=1) {
        auto id = getClusterDimId(d);
        space.setSetDimId_inplace(d, id.move());
      }

     this-> clusterShape = islctx->createRectangularSet(clusterLengths);
      clusterShape.cast_inplace(space);
    }

    llvm::ArrayRef<unsigned> getClusterLengths() const { return clusterLengths; }

    isl::BasicSet getClusterShape() const { return clusterShape; }
    isl::BasicSet getClusterParamShape() const { return clusterShape.moveDims(isl_dim_param, clusterShape.getParamDimCount(), isl_dim_set, 0,clusterShape.getDimCount()); }

    unsigned getClusterDims() const { return clusterLengths.size(); }
    unsigned getClusterSize() const;
    unsigned getClusterLength(size_t d) const { assert(0 <= d && d < clusterLengths.size()); return clusterLengths[d]; }

    // { [] -> len[cluster] } 
    isl::MultiAff getClusterLengthsAff() const;

    isl::Id getClusterDimId(isl::pos_t d);

    isl::MultiAff getMasterRank() const;

    llvm::Value *codegenComputeRank(DefaultIRBuilder &builder, llvm::ArrayRef<llvm::Value*> coords);
    llvm::Value *codegenRank(MollyCodeGenerator &codegen, isl::PwMultiAff coords);
  }; // class ClusterConfig
} // namespace molly
#endif /* MOLLY_CLUSTERCONFIG_H */
