#ifndef MOLLY_MOLLYCONTEXTPASS_H
#define MOLLY_MOLLYCONTEXTPASS_H 1

#include <llvm/Pass.h>

namespace llvm {
  class Module;
}

namespace molly {
  class MollyContext;

    class MollyContextPass : public llvm::ModulePass {
  private:
    MollyContext *context;

  public:
    static char ID;
    MollyContextPass() : ModulePass(ID) {
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

    virtual bool runOnModule(llvm::Module &M);

    MollyContext *getMollyContext() { return context; }
  };


}


#endif /* MOLLY_MOLLYCONTEXTPASS_H */
