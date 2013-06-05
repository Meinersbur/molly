#ifndef MOLLY_MOLLYCONTEXTPASS_H
#define MOLLY_MOLLYCONTEXTPASS_H 1

#include <llvm/Pass.h>
#include <llvm/ADT/SmallString.h>
#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Multi.h"

namespace llvm {
  class Module;
}

namespace isl {
} // namespace isl


namespace molly {
  class MollyContext;
  class IslCtx;

  class MollyContextPass : public llvm::ModulePass {
  private:
    MollyContext *context;
    llvm::SmallVector<unsigned,4> clusterLengths;
    isl::BasicSet clusterShape;

  public:
    static char ID;
    MollyContextPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

    virtual bool runOnModule(llvm::Module &M);

    MollyContext *getMollyContext() { return context; }
    isl::Ctx *getIslContext();

    /// Returns the space the node coordinates are in
    isl::Space getClusterSpace();
    /// Get the set of all nodes in the cluster
    isl::BasicSet &getClusterShape() { return clusterShape; }
    const llvm::SmallVectorImpl<unsigned> &getClusterLengths() { return clusterLengths; }
    int getClusterDims() const;
    int getClusterSize() const;
    int getClusterLength(int d) const;

    isl::MultiAff getMasterRank(); /* { -> 0 } */
  };
}


#endif /* MOLLY_MOLLYCONTEXTPASS_H */
