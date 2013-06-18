#ifndef MOLLY_MOLLYCONTEXTPASS_H
#define MOLLY_MOLLYCONTEXTPASS_H 1

#include <llvm/Pass.h>
#include <llvm/ADT/SmallString.h>
#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Multi.h"
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/ArrayRef.h>

namespace llvm {
  class Module;
  class LLVMContext;
} // namespace llvm

namespace isl {
} // namespace isl

namespace molly {
  class ClusterConfig;
  class FieldDetectionAnalysis;
} // namespace molly


namespace molly {

  /// This class preserves information necessary between Molly passes
  /// The pass manager may not discard it between passes, otherwise applied transformations are lost
  /// Currently the llvm PassManaget doesn't support this, therefore all passes in between need to explicitle preserve this pass
  class MollyContextPass LLVM_FINAL : public llvm::ModulePass {
  private:
    //MollyContext *context; // deprecated

    isl::Ctx *islctx;
    llvm::LLVMContext *llvmContext;
    llvm::OwningPtr<ClusterConfig> clusterConf;
    FieldDetectionAnalysis *fieldDetection;
    //TODO: Preserve SCoPs

    void initClusterConf();

  public:
    static char ID;
    MollyContextPass();
    ~MollyContextPass();

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE;
    void releaseMemory() LLVM_OVERRIDE;
    bool runOnModule(llvm::Module &M) LLVM_OVERRIDE;
    //const char *getPassName() const LLVM_OVERRIDE { return "MollyContextPass"; }

    //MollyContext *getMollyContext() { return context; }

    /// Returns the space the node coordinates are in
    isl::Space getClusterSpace();
    /// Get the set of all nodes in the cluster
    isl::BasicSet getClusterShape();
    llvm::ArrayRef<unsigned> getClusterLengths();
    int getClusterDims() const;
    int getClusterSize() const;
    int getClusterLength(int d) const;

    isl::MultiAff getMasterRank(); /* { -> 0 } */

    isl::Ctx *getIslContext() { assert(islctx); return islctx; }
    llvm::LLVMContext *getLLVMContext() { assert(llvmContext); return llvmContext; }
    ClusterConfig *getClusterConfig() { assert(clusterConf); return clusterConf.get(); }
    FieldDetectionAnalysis *getFieldDetection() { assert(fieldDetection); return fieldDetection; }
  }; // class MollyContextPass

  extern char &MollyContextPassID;

} // namespace molly
#endif /* MOLLY_MOLLYCONTEXTPASS_H */
