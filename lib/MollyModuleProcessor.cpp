#include "MollyModuleProcessor.h"

#include <llvm/PassAnalysisSupport.h>
#include "MollyPassManager.h"

using namespace molly;
//using namespace polly;
using namespace llvm;
using namespace std;
//using isl::enwrap;

namespace {

  class MollyModuleResolver : public AnalysisResolver {
  private:
    MollyPassManager *pm;
    Module *module;

  public:
    MollyModuleResolver(MollyPassManager *pm, Module *module) 
      :  AnalysisResolver(*static_cast<PMDataManager*>(nullptr)), pm(pm), module(module) {}

    Pass * findImplPass(AnalysisID PI) override {
      return pm->findOrRunAnalysis(PI, nullptr, nullptr);
    }

    Pass * findImplPass(Pass *P, AnalysisID PI, Function &F) override {
      return pm->findOrRunAnalysis(PI, &F, nullptr);
    }

    Pass * getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override {
      return pm->findAnalysis(ID, nullptr, nullptr);
    }
  }; // class MollyModuleResolver

  class MollyModuleProcessorImpl : public MollyModuleProcessor, private AnalysisResolver {
  private:
    MollyPassManager *pm;
    llvm::Module *module;

  public:
    MollyModuleProcessorImpl(MollyPassManager *pm, llvm::Module *module) : AnalysisResolver(*static_cast<PMDataManager*>(nullptr)) {}
  }; // class MollyModuleProcessorImpl
} // namespace

MollyModuleProcessor *MollyModuleProcessor::create(MollyPassManager *pm, llvm::Module *module) {
  return new MollyModuleProcessorImpl(pm, module);
}
AnalysisResolver *MollyModuleProcessor::createResolver(MollyPassManager *pm, llvm::Module *module) {
  return new MollyModuleResolver(pm, module);
}
