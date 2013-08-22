/// obsolete, replaced by GlobalPassManager
// This file is the result of the shortcomings of LLVM's pass manager,
// Namely:
// - Cannot access lower-level passes (Except on-the-fly FunctionPass from ModulePass)
// - Cannot force to preserve passes which contain information modified between mutliple passes (Here: Need to remember SCoPs and alter the schedule before CodeGen; In worst case, SCoPs will be recomputed form scratch with the trivial schedule otherwise)
#ifndef MOLLY_MOLLYPASSMANAGER_H
#define MOLLY_MOLLYPASSMANAGER_H

#include <llvm/Support/Compiler.h>
#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include "Clangfwd.h"
#include "islpp/Islfwd.h"


namespace molly {

  class MollyPassManager {
  protected:
    MollyPassManager() {};
    /* implicit */ MollyPassManager(const MollyPassManager &) LLVM_DELETED_FUNCTION;

  public:
    virtual llvm::LLVMContext &getLLVMContext() const = 0;
    virtual isl::Ctx *getIslContext() = 0;
    virtual ClusterConfig *getClusterConfig() = 0;
    virtual clang::CodeGen::MollyRuntimeMetadata *getRuntimeMetadata() = 0;

    virtual MollyFunctionProcessor *getFuncContext(llvm::Function * func) = 0;
    virtual MollyScopProcessor *getScopContext(polly::Scop *scop) = 0;
    virtual MollyScopStmtProcessor *getScopStmtContext(polly::ScopStmt *stmt) = 0;

    virtual MollyFieldAccess getFieldAccess(polly::ScopStmt *stmt) = 0;
    virtual MollyFieldAccess getFieldAccess(polly::MemoryAccess *) = 0;
    virtual MollyFieldAccess getFieldAccess(llvm::Instruction *) = 0;
    virtual FieldType *getFieldType(llvm::StructType *ty) = 0;
    virtual FieldType *getFieldType(llvm::Value *val) = 0;
    virtual FieldVariable *getFieldVariable(llvm::Value *val) = 0;

    virtual void runRegionPass(llvm::RegionPass *, llvm::Region *) = 0;
    virtual void runFunctionPass(llvm::FunctionPass *, llvm::Function *) = 0;
    virtual void runModulePass(llvm::ModulePass *) = 0;

    virtual llvm::Pass *findAnalysis(llvm::AnalysisID passID, llvm::Function *func) = 0;
    virtual llvm::Pass *findAnalysis(llvm::AnalysisID passID, llvm::Function *func, llvm::Region *region) = 0;

    virtual llvm::Pass *findOrRunAnalysis(llvm::AnalysisID passID, llvm::Function *func) = 0;
    virtual llvm::Pass *findOrRunAnalysis(llvm::AnalysisID passID, llvm::Function *func, llvm::Region *region) = 0;

    template<typename T>
    T *findOrRunAnalysis(llvm::Region *region) { 
      return (T*)findOrRunAnalysis(&T::ID, nullptr, region)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunAnalysis(llvm::Function *func) { 
      return (T*)findOrRunAnalysis(&T::ID, func, nullptr)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunAnalysis(llvm::Function *func, llvm::Region *region) { 
      return (T*)findOrRunAnalysis(&T::ID, func, region)->getAdjustedAnalysisPointer(&T::ID);
    }

    virtual void removePass(llvm::Pass *pass) = 0;

    virtual void modifiedIR() = 0;
    virtual void modifiedScop() = 0;

     virtual CommunicationBuffer *newCommunicationBuffer(FieldType *fty, isl::Map &&relation) = 0;
  }; // class MollyPassManager

  extern char &MollyPassManagerID;
  llvm::ModulePass *createMollyPassManager();

} // namespace molly
#endif /* MOLLY_MOLLYPASSMANAGER_H */
