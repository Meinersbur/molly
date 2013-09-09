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
#include "islpp/BasicSet.h"
#include "LLVMfwd.h"

namespace isl {
class Space;
} // namespace isl


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
    
    void setClusterLengths(llvm::ArrayRef<unsigned> lengths) {
      clusterLengths.clear();
      clusterLengths.append(lengths.begin(), lengths.end());

      clusterShape = islctx->createRectangularSet(clusterLengths);
      clusterShape.setTupleId_inplace(getClusterTuple());
    }

    llvm::ArrayRef<unsigned> getClusterLengths() const { return clusterLengths; }
    isl::BasicSet getClusterShape() const { return clusterShape; }

    unsigned getClusterDims() const { return clusterLengths.size(); }
    unsigned getClusterSize() const;
    unsigned getClusterLength(size_t d) const { assert(0 <= d && d < clusterLengths.size()); return clusterLengths[d]; }

    isl::MultiAff getMasterRank() const;

    llvm::Value *codegenComputeRank(DefaultIRBuilder &builder, llvm::ArrayRef<llvm::Value*> coords);
  }; // class ClusterConfig
} // namespace molly
#endif /* MOLLY_CLUSTERCONFIG_H */
