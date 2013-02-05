#ifndef MOLLY_FIELD_DETECTION_H
#define MOLLY_FIELD_DETECTION_H

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"

namespace llvm {
  class GlobalVariable;
  class StructType;
}

namespace molly {
  class FieldVariable;
  class FieldType;
}

using namespace llvm;

namespace molly {
  class MollyContextPass;

  class FieldDetectionAnalysis : public ModulePass {
  private:
    DenseMap<GlobalVariable*, FieldVariable*> fieldVars;
    DenseMap<StructType*, FieldType*> fieldTypes;

  public:
    static char ID;
    FieldDetectionAnalysis() : ModulePass(ID) {
    }

    virtual void FieldDetectionAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<molly::MollyContextPass>();
      AU.setPreservesAll();
    }

    virtual bool runOnModule(Module &M);

    DenseMap<GlobalVariable*, FieldVariable*> &getFieldVariables() { return fieldVars; }
    DenseMap<StructType*, FieldType*> &getFieldTypes() { return fieldTypes; }
  private:
    Pass *createFieldDetectionAnalysisPass();

  };

}

namespace llvm {
  class PassRegistry;
  void initializeScopDetectionPass(llvm::PassRegistry&);
}


#endif /* MOLLY_FIELD_DETECTION_H */
