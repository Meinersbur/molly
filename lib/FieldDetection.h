#ifndef MOLLY_FIELD_DETECTION_H
#define MOLLY_FIELD_DETECTION_H

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/DenseMap.h>

namespace llvm {
  class GlobalVariable;
  class StructType;
  class CallInst;
  class Instruction;
  class Module;
  class ModulePass;
} // namespace llvm

namespace polly {
  class MemoryAccess;
} // namespace polly

namespace molly {
  class FieldVariable;
  class FieldType;
  class MollyContext;
  class MollyContextPass;
  class MollyFieldAccess;
} // namespace molly



namespace molly {
  class MollyContextPass;

  class FieldDetectionAnalysis : public llvm::ModulePass {
  private:
    llvm::DenseMap<llvm::GlobalVariable*, FieldVariable*> fieldVars; // obsolete; currently we do not collect instances of fields
    llvm::DenseMap<llvm::StructType*, FieldType*> fieldTypes;
    //llvm::DenseMap<isl::Id, FieldVariable*> tupleToVar;

    MollyContextPass *mollyContext;
    llvm::Module *module;

  public:
    static char ID;
    FieldDetectionAnalysis() : ModulePass(ID) {
    }

    const char *getPassName() const LLVM_OVERRIDE { return "FieldDetectionAnalysis"; }
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE;

    bool runOnModule(llvm::Module &M) LLVM_OVERRIDE;
    void releaseMemory() LLVM_OVERRIDE;

    llvm::DenseMap<llvm::GlobalVariable*, FieldVariable*> &getFieldVariables() { return fieldVars; }
    llvm::DenseMap<llvm::StructType*, FieldType*> &getFieldTypes() { return fieldTypes; } // TODO: return a list of FieldType*, not a map
  private:
    static Pass *createFieldDetectionAnalysisPass();

  public:
    FieldType *lookupFieldType(llvm::StructType *ty) ;
    FieldType *getFieldType(llvm::StructType *ty);
    FieldType *getFieldType(llvm::Value *val) ;

    FieldVariable *lookupFieldVariable(llvm::GlobalVariable *gvar);
    FieldVariable *getFieldVariable(llvm::GlobalVariable *gvar);

    FieldType *getFromFunction(llvm::Function *func);
    FieldVariable *getFromCall(const llvm::CallInst *inst);
    FieldVariable *getFromAccess(llvm::Instruction *inst);

    MollyFieldAccess getFieldAccess(const llvm::Instruction *inst);
    MollyFieldAccess getFieldAccess(polly::MemoryAccess *memacc);
  }; // class FieldDetectionAnalysis

  extern char &FieldDetectionAnalysisPassID;
} // namespace molly


namespace llvm {
  class PassRegistry;
  void initializeScopDetectionPass(llvm::PassRegistry&);
} // namespace llvm

#endif /* MOLLY_FIELD_DETECTION_H */
