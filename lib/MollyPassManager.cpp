#include "MollyPassManager.h"

#include <llvm/Pass.h>
#include <polly/PollyContextPass.h>
//#include <llvm/IR/Module.h>
#include "MollyUtils.h"
#include <llvm/ADT/DenseSet.h>
#include "MollyContextPass.h"
#include <polly/ScopInfo.h>
#include <polly/LinkAllPasses.h>
#include "FieldDistribution.h"
#include "FieldDetection.h"
#include "FieldCodeGen.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;

namespace molly {

#define MollyPassManager MollyPassManager LLVM_FINAL // Visual Studio source browser does not preprocess
  class MollyPassManager : public llvm::ModulePass {
#undef MollyPassManager
  private:
    bool changedIR;
    Module *module;

    llvm::DenseSet<AnalysisID> alwaysPreserve;
    llvm::DenseMap<llvm::AnalysisID, llvm::ModulePass*> currentModuleAnalyses; 
    llvm::DenseMap<std::pair< llvm::AnalysisID, llvm::Function*>, llvm::FunctionPass*> currentFunctionAnalyses; 
    llvm::DenseMap<std::pair< llvm::AnalysisID,llvm::Region*>, llvm::RegionPass*> currentRegionAnalyses; 

    void removeUnpreservedAnalyses(const AnalysisUsage &AU) {
      if (AU.getPreservesAll()) 
        return;

      auto &preserved = AU.getPreservedSet();
      DenseSet<AnalysisID> preservedSet;
      for (auto it = preserved.begin(), end = preserved.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }
      for (auto it = alwaysPreserve.begin(), end = alwaysPreserve.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }

      DenseSet<Pass*> donotFree; 
      decltype(currentModuleAnalyses) rescuedModuleAnalyses;
      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto moduleAnalysisID = it->first;
        auto moduleAnalysisPass = it->second;

        if (preservedSet.find(moduleAnalysisID) != preservedSet.end()) {
          donotFree.insert(moduleAnalysisPass);
          rescuedModuleAnalyses[moduleAnalysisID] = moduleAnalysisPass;
        }
      }

      decltype(currentFunctionAnalyses) rescuedFunctionAnalyses;
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto function = it->first.second;
        auto functionAnalysisID = it->first.first;
        auto functionAnalysisPass = it->second;

        if (preservedSet.find(functionAnalysisID) != preservedSet.end()) {
          donotFree.insert(functionAnalysisPass);
          rescuedFunctionAnalyses[std::make_pair(functionAnalysisID, function)] = functionAnalysisPass;
        }
      }

      decltype(currentRegionAnalyses) rescuedRegionAnalyses;
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto region = it->first.second;
        auto regionAnalysisID = it->first.first;
        auto regionAnalysisPass = it->second;

        if (preservedSet.find(regionAnalysisID) != preservedSet.end()) {
          donotFree.insert(regionAnalysisPass);
          rescuedRegionAnalyses[std::make_pair(regionAnalysisID, region)] = regionAnalysisPass;
        }
      }

      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        delete pass;
      } 
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        delete pass;
      }
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        delete pass;
      }

      currentModuleAnalyses = std::move(rescuedModuleAnalyses);
      currentFunctionAnalyses = std::move(rescuedFunctionAnalyses);
      currentRegionAnalyses = std::move(rescuedRegionAnalyses);
    }

    void removeAllAnalyses() {
      DenseSet<Pass*> doFree; // To avoid double-free

      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      } 
      currentModuleAnalyses.clear();
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentFunctionAnalyses.clear();
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentRegionAnalyses.clear();

      for (auto it = doFree.begin(), end = doFree.end(); it!=end; ++it) {
        auto pass = *it;
        delete pass;
      }
    }

  public:
    static char ID;
    MollyPassManager() : ModulePass(ID) {
      //alwaysPreserve.insert(&polly::PollyContextPassID);
      //alwaysPreserve.insert(&molly::MollyContextPassID);
      //alwaysPreserve.insert(&polly::ScopInfo::ID);
      //alwaysPreserve.insert(&polly::IndependentBlocksID);
    }
    ~MollyPassManager() {
      removeAllAnalyses();
    }




    void releaseMemory() LLVM_OVERRIDE {
      removeAllAnalyses();
    }
    const char *getPassName() const LLVM_OVERRIDE { return "MollyPassManager"; }
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
      // Requires nothing, preserves nothing
    }


    void runModulePass(llvm::ModulePass *pass, bool permanent) {
      auto passID = pass->getPassID();
      currentModuleAnalyses[passID] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      }

      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentModuleAnalyses[intfPassID] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }

      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, false);
      }

      bool changed = pass->runOnModule(*module);
      if (changed)
        removeUnpreservedAnalyses(AU);
    }

    ModulePass *findOrRunAnalysis(AnalysisID passID, bool permanent) {
      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end()) {
        return it->second;
      }

      auto parentPass = getResolver()->getAnalysisIfAvailable(passID, true);
      if (parentPass) {
        assert(parentPass->getPassKind() == PT_Module);
        return static_cast<ModulePass*>(parentPass);
      }

      auto pass = createPassFromId(passID);
      assert(pass->getPassKind() == PT_Module);
      auto modulePass = static_cast<ModulePass*>(pass);
      runModulePass(modulePass, permanent);
      return modulePass;
    }
    ModulePass *findOrRunAnalysis(const char &passID, bool permanent) {
      return findOrRunAnalysis(&passID, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis(bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis() {
      return findOrRunAnalysis<T>(true);
    }



    void runFunctionPass(llvm::FunctionPass *pass, Function *func, bool permanent) {
      auto passID = pass->getPassID();
      currentFunctionAnalyses[std::make_pair(passID, func)] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      }

      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentFunctionAnalyses[std::make_pair(intfPassID, func)] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }

      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, func, false);
      }

      bool changed = pass->runOnFunction(*func);
      if (changed)
        removeUnpreservedAnalyses(AU);
    }

    Pass *findOrRunAnalysis(AnalysisID passID, Function *func, bool permanent) {
      {
        auto it = currentFunctionAnalyses.find(make_pair(passID, func));
        if (it != currentFunctionAnalyses.end()) {
          return it->second;
        }
      }{
        auto it = currentModuleAnalyses.find(passID);
        if (it != currentModuleAnalyses.end()) {
          return it->second;
        }}

      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass), permanent);
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func, permanent);
        break;
      default:
        llvm_unreachable("Wrong kinfd of pass");
      }
      runFunctionPass(static_cast<FunctionPass*>(pass), func, permanent);
      return pass;
    }
    Pass *findOrRunAnalysis(const char &passID,  Function *func,bool permanent ) {
      return findOrRunAnalysis(&passID,func, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis( Function *func,bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, func, permanent)->getAdjustedAnalysisPointer(T::PI);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis( Function *func) {
      return (T*)findOrRunAnalysis(&T::ID, func, false)->getAdjustedAnalysisPointer(T::PI); 
    }


    llvm::Pass *runRegionPass(llvm::RegionPass *pass, Region *region, bool permanent) {
      auto passID = pass->getPassID();
      currentRegionAnalyses[std::make_pair(passID, region)] = pass;
      if (permanent) {
        alwaysPreserve.insert(passID);
      }

      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentRegionAnalyses[std::make_pair(intfPassID, region)] = pass;
        if (permanent) 
          alwaysPreserve.insert(intfPassID);
      }

      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, region, false);
      }

      bool changed = pass->runOnRegion(region, *((RGPassManager*)nullptr));
      if (changed)
        removeUnpreservedAnalyses(AU);
    }



    Pass *findOrRunAnalysis(AnalysisID passID, Region *region, bool permanent) {
      {auto it = currentRegionAnalyses.find(make_pair(passID, region));
      if (it != currentRegionAnalyses.end()) {
        return it->second;
      }}

      auto func = region->getEnteringBlock()->getParent();
      {
        auto it = currentFunctionAnalyses.find(make_pair(passID, func));
        if (it != currentFunctionAnalyses.end()) {
          return it->second;
        }}

      {
        auto it = currentModuleAnalyses.find(passID);
        if (it != currentModuleAnalyses.end()) {
          return it->second;
        }}

      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass), permanent);
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func, permanent);
        break;
      case PT_Region:
        runRegionPass(static_cast<RegionPass*>(pass), region, permanent);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }
      return pass;
    }
    Pass *findOrRunAnalysis(const char &passID,  Region *region,bool permanent ) {
      return findOrRunAnalysis(&passID,region, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis( Region *region,bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, region, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis( Region *region) {
      return (T*)findOrRunAnalysis(&T::ID, region, false)->getAdjustedAnalysisPointer(&T::ID); 
    }


  private:
    void runOnFunction(Function *func) {
    }

  public:
    bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
      this->changedIR = false;
      this->module = &M;

      findOrRunPemanentAnalysis<polly::PollyContextPass>();
      findOrRunPemanentAnalysis<molly::MollyContextPass>();
      findOrRunAnalysis(molly::FieldDetectionAnalysisPassID, true);
      findOrRunAnalysis(molly::FieldDistributionPassID, false);
      findOrRunAnalysis(molly::ModuleFieldGenPassID, false);

      for (auto it = M.begin(), end = M.end(); it!=end; ++it) {
        auto func = &*it;
        runOnFunction(func);
      }

      return changedIR;
    }

  }; // class MollyPassManager
} // namespace molly


char molly::MollyPassManager::ID = 0;
extern char &molly::MollyPassManagerID = molly::MollyPassManager::ID;
ModulePass *molly::createMollyPassManager() {
  return new MollyPassManager();
}
