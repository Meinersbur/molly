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

namespace isl {
class Space;
} // namespace isl


namespace molly {
  class ClusterConfig {
  private:
    isl::Ctx *islctx;

    llvm::SmallVector<unsigned,4> clusterLengths;
    isl::BasicSet clusterShape;

  protected:
    isl::Ctx *getIslContext() const { assert(islctx); return islctx; }

  public:
    ClusterConfig(isl::Ctx *islctx) : islctx(islctx) {
      assert(islctx);
    }
    
    void setClusterLengths(llvm::ArrayRef<unsigned> lengths) {
      clusterLengths.clear();
      clusterLengths.append(lengths.begin(), lengths.end());

      clusterShape = islctx->createRectangularSet(clusterLengths);
    }

    llvm::ArrayRef<unsigned> getClusterLengths() const { return clusterLengths; }
    isl::BasicSet getClusterShape() const { return clusterShape; }
    isl::Space getClusterSpace() const;
    unsigned getClusterDims() const { return clusterLengths.size(); }
    unsigned getClusterSize() const;
    unsigned getClusterLength(size_t d) const { assert(0 <= d && d < clusterLengths.size()); return clusterLengths[d]; }
  }; // class ClusterConfig
} // namespace molly
#endif /* MOLLY_CLUSTERCONFIG_H */
