#ifndef MOLLY_FIELD_DETECTION_H
#define MOLLY_FIELD_DETECTION_H

#include <llvm/Pass.h>
#include "llvm/IR/Module.h"
#include <llvm/ADT/DenseMap.h>

namespace llvm {
  class GlobalVariable;
  class StructType;
  class Function;
  class CallInst;
  class Instruction;
  class Module;
}

namespace molly {
  class FieldVariable;
  class FieldType;
  class MollyContext;
  class FieldAccess;
}



namespace molly {
  class MollyContextPass;

  class FieldDetectionAnalysis : public llvm::ModulePass {
  private:
    llvm::DenseMap<llvm::GlobalVariable*, FieldVariable*> fieldVars;
    llvm::DenseMap<llvm::StructType*, FieldType*> fieldTypes;

    MollyContext *mollyContext;
    llvm::Module *module;

  public:
    static char ID;
    FieldDetectionAnalysis() : ModulePass(ID) {
    }

    virtual const char *getPassName() const {
      return ModulePass::getPassName();
    }

    virtual void FieldDetectionAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequiredTransitive<molly::MollyContextPass>();
      AU.setPreservesAll();
    }

    virtual bool runOnModule(llvm::Module &M);

    llvm::DenseMap<llvm::GlobalVariable*, FieldVariable*> &getFieldVariables() { return fieldVars; }
    llvm::DenseMap<llvm::StructType*, FieldType*> &getFieldTypes() { return fieldTypes; } // TODO: return a list of FieldType*, not a map
  private:
    Pass *createFieldDetectionAnalysisPass();


  public:
    FieldType *lookupFieldType(llvm::StructType *ty) ;
    FieldType *getFieldType(llvm::StructType *ty);

    FieldVariable *lookupFieldVariable(llvm::GlobalVariable *gvar);
    FieldVariable *getFieldVariable(llvm::GlobalVariable *gvar);

     FieldType *getFromFunction(llvm::Function *func);
     FieldVariable *getFromCall(llvm::CallInst *inst);
     FieldVariable *getFromAccess(llvm::Instruction *inst);

     FieldAccess getFieldAccess(llvm::Instruction *inst);

  };

}

namespace llvm {
  class PassRegistry;
  void initializeScopDetectionPass(llvm::PassRegistry&);
}


#endif /* MOLLY_FIELD_DETECTION_H */