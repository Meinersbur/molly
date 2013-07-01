#include "GlobalPassManager.h"

#include <polly/ScopPass.h>
#include <llvm/IR/Module.h>
#include "MollyUtils.h"

using namespace molly;
using namespace polly;
using namespace llvm;



namespace molly {
  class GlobalPassAnalysisResolver : public AnalysisResolver {
  protected:
    GlobalPassManager *GPM;

  protected:
    GlobalPassAnalysisResolver(GlobalPassManager *GPM) : AnalysisResolver(*((PMDataManager*)nullptr)) { this->GPM = GPM; }

  public:
    virtual Pass *findImplPass(AnalysisID PI) override = 0;
    virtual Pass *findImplPass(Pass *P, AnalysisID PI, Function &F) override { return GPM->findOrExecuteAnalysis(PI, &F); }
    virtual Pass *getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override = 0;
  } ; // class GlobalPassAnalysisResolver


  class GlobalPassModuleAnalysisResolver : public GlobalPassAnalysisResolver {
  private:
    Module *module;
  public:
    GlobalPassModuleAnalysisResolver(GlobalPassManager *GPM, Module *module) : GlobalPassAnalysisResolver(GPM) { this->module = module; }

    virtual Pass *findImplPass(AnalysisID PI) override {
      return GPM->findOrExecuteAnalysis(PI, module);
    }

    virtual Pass *getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override {
      return GPM->findAnalysis(ID, module);
    }
  } ; // class GlobalPassRegionAnalysisResolver


  class GlobalPassFunctionAnalysisResolver : public GlobalPassAnalysisResolver {
  private:
    Function *function;
  public:
    GlobalPassFunctionAnalysisResolver(GlobalPassManager *GPM, Function *function) : GlobalPassAnalysisResolver(GPM) { this->function = function; }

    virtual Pass *findImplPass(AnalysisID PI) override {
      return GPM->findOrExecuteAnalysis(PI, function);
    }

    virtual Pass *getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override {
      return GPM->findAnalysis(ID, function);
    }
  }; // class GlobalPassRegionAnalysisResolver


  class GlobalPassRegionAnalysisResolver : public GlobalPassAnalysisResolver {
  private:
    Region *region;
  public:
    GlobalPassRegionAnalysisResolver(GlobalPassManager *GPM, Region *region) : GlobalPassAnalysisResolver(GPM) { this->region = region; }

    virtual Pass *findImplPass(AnalysisID PI) override {
      return GPM->findOrExecuteAnalysis(PI, region);
    }

    virtual Pass *getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override {
      return GPM->findAnalysis(ID, region);
    }
  } ; // class GlobalPassRegionAnalysisResolver
} // namespace molly


void GlobalPassManager::pushModule(llvm::Module *module) {
  beginModule(module);
  for (auto it = module->begin(), end = module->end(); it!=end; ++it) {
    auto func = &*it;
    pushFunction(func);
  }
  endModule();
}


void GlobalPassManager::addPassID(AnalysisID id) {
  passes.push_back(id);
}


void GlobalPassManager::run(llvm::Module *module) {
  releaseMemory();
  auto passRegistry = PassRegistry::getPassRegistry();

  //FIXME: This is the simplest algorithm to do the job; for performance, some more stuff should be done:
  // - Execute passes of same level in a row for locality
  // - Do not store analyses that are only used at one level globally
  // - Free and reuse passes as soon as possible (i.e. there is no more pass downstream that requires this analysis)
  // - Execute multithreaded (WARNING: Not all passes might be prepared for this)
  // - Do dependence analysis before execution
  auto nPasses = passes.size();
  for (auto i =nPasses-nPasses;i<nPasses; i+=1) {
    auto passId = passes.at(i);
    auto passInfo = passRegistry->getPassInfo(passId);
    auto passIsAnalysis = passInfo->isAnalysis();
    auto pass = passInfo->createPass();
    auto passKind = pass->getPassKind();
    AnalysisUsage AU;
    pass->getAnalysisUsage(AU);
    delete pass; // Don't know any other method to get the type of pass

    switch (passKind) {
    case PT_Module:
      if (passIsAnalysis) {
        auto it = currentModuleAnalyses.find(std::make_pair(module,passId));
        if (it != currentModuleAnalyses.end())
          continue; // Analysis already done, no need to redo
      }

      runModulePass(passId, module);

      break;
    case PT_Function:

      for (auto itFunc = module->begin(), endFunc = module->end(); itFunc!=endFunc;++itFunc) {
        auto func = &*itFunc;

        if (passIsAnalysis) {
          auto it = currentFunctionAnalyses.find(std::make_pair(func,passId));
          if (it != currentFunctionAnalyses.end())
            continue; // Analysis already done, no need to redo
        }

        runFunctionPass(passId, func);

      }
      break;
    case PT_Region:
      for (auto itFunc = module->begin(), endFunc = module->end(); itFunc!=endFunc;++itFunc) {
        auto func = &*itFunc;
        auto regionAnalysis = findOrExecuteAnalysis<RegionInfo>(func);
        SmallVector<Region*,32> regions;
        collectAllRegions(regionAnalysis, regions);

        for (auto itRegion = regions.begin(), endRegion=regions.end(); itRegion!=endRegion;++itRegion) {
          auto region = *itRegion;

          if (passIsAnalysis) {
            auto analysis = findAnalysis(passId, region);
            if (analysis)
              continue;
          }

          runRegionPass(passId, region);
        }
      }

      break;
    default:
      llvm_unreachable("This type of pass not yet supported");
    }
  }
}


void GlobalPassManager::removeObsoletedAnalyses(llvm::Pass *pass) {
  AnalysisUsage AU;
  pass->getAnalysisUsage(AU);

  if (AU.getPreservesAll())
    return;

  auto &preserved = AU.getPreservedSet();
  DenseSet<AnalysisID> preservedSet;
  for (auto it = preserved.begin(), end = preserved.end(); it!=end; ++it) {
    auto preservedID = *it;
    preservedSet.insert(preservedID);
  }

  decltype(currentModuleAnalyses) rescuedModuleAnalyses;
  for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
    auto module = it->first.first;
    auto moduleAnalysisID = it->first.second;
    auto moduleAnalysisPass = it->second;

    if (preservedSet.find(moduleAnalysisID) == preservedSet.end()) {
      delete moduleAnalysisPass->getResolver();
      delete moduleAnalysisPass;
    } else {
      rescuedModuleAnalyses[std::make_pair(module, moduleAnalysisID)] = moduleAnalysisPass;
    }
  }
  currentModuleAnalyses = rescuedModuleAnalyses;

  decltype(currentFunctionAnalyses) rescuedFunctionAnalyses ;
  for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
    auto function = it->first.first;
    auto functionAnalysisID = it->first.second;
    auto functionAnalysisPass = it->second;

    if (preservedSet.find(functionAnalysisID) == preservedSet.end()) {
      delete functionAnalysisPass->getResolver();
      delete functionAnalysisPass;
    } else {
      rescuedFunctionAnalyses[std::make_pair(function, functionAnalysisID)] = functionAnalysisPass;
    }
  }
  currentFunctionAnalyses = rescuedFunctionAnalyses;

  decltype(currentRegionAnalyses) rescuedRegionAnalyses ;
  for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
    auto region = it->first.first;
    auto regionAnalysisID = it->first.second;
    auto regionAnalysisPass = it->second;

    if (preservedSet.find(regionAnalysisID) == preservedSet.end()) {
      delete regionAnalysisPass->getResolver();
      delete regionAnalysisPass;
    } else {
      rescuedRegionAnalyses[std::make_pair(region, regionAnalysisID)] = regionAnalysisPass;
    }
  }
  currentRegionAnalyses = rescuedRegionAnalyses;
}


llvm::ModulePass * GlobalPassManager::runModulePass(const void *passID, llvm::Module *module) {
  auto passRegistry = PassRegistry::getPassRegistry();
  auto passInfo = passRegistry->getPassInfo(passID);
  auto passIsAnalysis = passInfo->isAnalysis();
  auto pass = passInfo->createPass();
  pass->setResolver(new GlobalPassModuleAnalysisResolver(this, module));
  auto passKind = pass->getPassKind();
  AnalysisUsage AU;
  pass->getAnalysisUsage(AU);
#if 0
  auto &required = AU.getRequiredSet();
  for (auto it = required.begin(), end = required.end(); it!=end; ++it) {
    auto requiredID = *it;
    auto requiredPass = findOrExecuteAnalysis(requiredID, module);
    assert(requiredPass);
  }

  auto &requiredTransitive = AU.getRequiredTransitiveSet();
  for (auto it = requiredTransitive.begin(), end = requiredTransitive.end(); it!=end; ++it) {
    auto requiredID = *it;
    auto requiredPass = findOrExecuteAnalysis(requiredID, module);
    assert(requiredPass);
  }
#endif
  assert(pass->getPassKind() == PT_Module);
  auto modulePass = static_cast<ModulePass*>(pass);
  auto modified = modulePass->runOnModule(*module);
  if (modified)
    removeObsoletedAnalyses(modulePass);

  currentModuleAnalyses[std::make_pair(module, passID)] = modulePass;
  return modulePass;
}


llvm::FunctionPass *GlobalPassManager::runFunctionPass(const void *passID, llvm::Function *function) {
  auto passRegistry = PassRegistry::getPassRegistry();
  auto passInfo = passRegistry->getPassInfo(passID);
  auto passIsAnalysis = passInfo->isAnalysis();
  auto pass = passInfo->createPass();
  pass->setResolver(new GlobalPassFunctionAnalysisResolver(this, function));
  assert(pass->getPassKind() == PT_Function);
  auto functionPass = static_cast<FunctionPass*>(pass);
  auto modified = functionPass->runOnFunction(*function);
  if (modified)
    removeObsoletedAnalyses(functionPass);

  currentFunctionAnalyses[std::make_pair(function, passID)] = functionPass;
  return functionPass;
}


llvm::RegionPass * GlobalPassManager::runRegionPass(const void *passID, llvm::Region *region) {
  auto passRegistry = PassRegistry::getPassRegistry();
  auto passInfo = passRegistry->getPassInfo(passID);
  auto passIsAnalysis = passInfo->isAnalysis();
  auto pass = passInfo->createPass();
  pass->setResolver(new GlobalPassRegionAnalysisResolver(this, region));
  assert(pass->getPassKind() == PT_Region);
  auto regionPass = static_cast<RegionPass*>(pass);
  RGPassManager dummy;
  auto modified = regionPass->runOnRegion(region, dummy);
  if (modified)
    removeObsoletedAnalyses(regionPass);

  currentRegionAnalyses[std::make_pair(region, passID)] = regionPass;
  return regionPass;
}


void GlobalPassManager::releaseMemory() {
  for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
    auto pass = it->second;
    delete pass->getResolver();
    delete pass;
  }
  currentModuleAnalyses.clear();

  for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
    auto pass = it->second;
    delete pass->getResolver();
    delete pass;
  }
  currentFunctionAnalyses.clear();

  for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
    auto pass = it->second;
    delete pass->getResolver();
    delete pass;
  }
  currentRegionAnalyses.clear();
}


llvm::ModulePass *GlobalPassManager::findAnalysis( const void *passID, llvm::Module *module) {
  auto it = currentModuleAnalyses.find(std::make_pair(module, passID));
  if (it==currentModuleAnalyses.end())
    return nullptr;
  assert(it->second->getPassKind() == PT_Module);
  return static_cast<ModulePass*>( it->second);
}


llvm::FunctionPass *GlobalPassManager::findAnalysis(const void *passID, llvm::Function *func) {
  auto it = currentFunctionAnalyses.find(std::make_pair(func, passID));
  if (it==currentFunctionAnalyses.end())
    return nullptr;
  assert(it->second->getPassKind() == PT_Function);
  return static_cast<FunctionPass*>( it->second);
}


llvm::RegionPass *GlobalPassManager::findAnalysis(AnalysisID passID, llvm::Region *region) {
  auto it = currentRegionAnalyses.find(std::make_pair(region, passID));
  if (it==currentRegionAnalyses.end())
    return nullptr;
  assert(it->second->getPassKind() == PT_Region);
  return static_cast<RegionPass*>(it->second);
}


llvm::ModulePass *GlobalPassManager::findOrExecuteAnalysis(AnalysisID passID, llvm::Module *module) {
  auto pass = findAnalysis(passID, module);
  if (pass)
    return pass;

  return runModulePass(passID, module);
}


llvm::FunctionPass *GlobalPassManager::findOrExecuteAnalysis(AnalysisID passID, llvm::Function *function) {
  auto pass = findAnalysis(passID, function);
  if (pass)
    return cast<FunctionPass>( pass);

  return runFunctionPass(passID, function);
}


llvm::RegionPass *GlobalPassManager::findOrExecuteAnalysis(AnalysisID passID, llvm::Region *region) {
  auto pass = findAnalysis(passID, region);
  if (pass)
    return cast<RegionPass>( pass);

  return runRegionPass(passID, region);
}


char GlobalPassManager::ID;
static RegisterPass<GlobalPassManager> GlobalPassManagerRegistration("globalpass", "Molly - Global Pass", false, false);

molly::GlobalPassManager *molly::createGlobalPassManager() {
  return new GlobalPassManager();
}
