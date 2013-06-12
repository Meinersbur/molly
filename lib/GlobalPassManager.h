#ifndef MOLLY_GLOBALPASSANAGER_H
#define MOLLY_GLOBALPASSANAGER_H
// This file is the result of the shortcomings of LLVM's pass manager,
// Namely:
// - Cannot access lower-level passes (Except on-the-fly FunctionPass from ModulePass)
// - Cannot force to preserve passes which contain information modified between mutliple passes (Here: Need to remember SCoPs and alter the schedule before CodeGen; In worst case, SCoPs will be recomputed form scratch with the trivial schedule otherwise)


#include <llvm/Support/Compiler.h>
#include <llvm/Pass.h> // ModulePass (baseclass of GlobalPassManager)
#include <vector>
#include <llvm/ADT/DenseSet.h>

namespace llvm {
  class Region;
  class RegionInfo;
  class RegionPass;
} // namespace llvm


namespace molly {

  class ModuleCallback {
  public:
    virtual void moduleFinished(llvm::Module *) = 0;
  }; // class ModuleCallback


  /// A pass manager that remembers the result of analyses of smaller units (e.g. ModulePass using a RegionPass)
  class GlobalPassManager : public llvm::ModulePass {
  private:
    llvm::Module *currentModule;

    // Order matters
    std::vector<llvm::AnalysisID> passes; 

#pragma region Required during execution
    llvm::DenseMap<std::pair<llvm::Module*, llvm::AnalysisID>, llvm::Pass*> currentModuleAnalyses; 
    llvm::DenseMap<std::pair<llvm::Function*, llvm::AnalysisID>, llvm::Pass*> currentFunctionAnalyses; 
    llvm::DenseMap<std::pair<llvm::Region*, llvm::AnalysisID>, llvm::Pass*> currentRegionAnalyses; 
#pragma endregion

    void removeObsoletedAnalyses(llvm::Pass *pass);
    void run(llvm::Module *module);
    llvm::ModulePass *runModulePass(llvm::AnalysisID passID, llvm::Module *module);
    llvm::FunctionPass *runFunctionPass(llvm::AnalysisID passID, llvm::Function *function);
    llvm::RegionPass *runRegionPass(llvm::AnalysisID passID, llvm::Region *region);
    void releaseMemory();

  public:
    ~GlobalPassManager() { releaseMemory(); }

    static char ID;
    GlobalPassManager() : llvm::ModulePass(ID) {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
      // Preserves nothing
    }

    virtual bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
      pushModule(&M);
      waitForCallbacks();
      return true;
    }

    void addPassID(llvm::AnalysisID id);
    void addPassID(const char &id) { addPassID(&id); }
    template<typename T>
    void addPass() { addPassID(&T::ID); }

    void pushModule(llvm::Module *module);

    void beginModule(llvm::Module *module) { currentModule = module; }
    void pushFunction(llvm::Function *function) {}
    void endModule() { 
      run(currentModule); 
      releaseMemory(); 
    }
    void waitForCallbacks() {}

    void addModuleCallback(ModuleCallback *callback);
    void doNotDiscard(llvm::Pass *);
    void canDiscardNow(llvm::Pass *);

        llvm::ModulePass *findAnalysis(llvm::AnalysisID, llvm::Module *);
    llvm::FunctionPass *findAnalysis(llvm::AnalysisID, llvm::Function *);
    template<typename T>
    T *findAnalysis( llvm::Function* function) { return static_cast<T*>( findAnalysis(&T::ID, function));  }
    llvm::RegionPass *findAnalysis(llvm::AnalysisID passID, llvm::Region* region);

        llvm::ModulePass *findOrExecuteAnalysis(llvm::AnalysisID passID, llvm::Module *);
    template<typename T>
    T *findOrExecuteAnalysis(llvm::Module *module) { return static_cast<T*>(findOrExecuteAnalysis(&T::ID, module)); }
    llvm::FunctionPass *findOrExecuteAnalysis(llvm::AnalysisID passID, llvm::Function *);
    template<typename T>
    T *findOrExecuteAnalysis(llvm::Function *func) { return static_cast<T*>(findOrExecuteAnalysis(&T::ID, func)); }
    llvm::RegionPass *findOrExecuteAnalysis(llvm::AnalysisID passID, llvm::Region *);
    template<typename T>
    T *findOrExecuteAnalysis(llvm::Region *region) { return static_cast<T*>(findOrExecuteAnalysis(&T::ID, region)); }
  }; // class GlobalPassManager


  molly::GlobalPassManager *createGlobalPassManager();

} // namespace molly
#endif /* MOLLY_GLOBALPASSANAGER_H */
