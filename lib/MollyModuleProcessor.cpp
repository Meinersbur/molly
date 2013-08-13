#include "MollyModuleProcessor.h"

#include "llvm\PassAnalysisSupport.h"
#include "MollyPassManager.h"

using namespace molly;
//using namespace polly;
using namespace llvm;
using namespace std;
//using isl::enwrap;


class MollyModuleProcessorImpl : public MollyModuleProcessor, private AnalysisResolver {
private:
  MollyPassManager *pm;
  llvm::Module *module;

public:
  MollyModuleProcessorImpl(MollyPassManager *pm, llvm::Module *module) : AnalysisResolver(*static_cast<PMDataManager*>(nullptr)) {}

#pragma region llvm::AnalysisResolver
   Pass * findImplPass( AnalysisID PI ) LLVM_OVERRIDE  {
    throw std::logic_error("The method or operation is not implemented.");
  }

   Pass * findImplPass(Pass *P, AnalysisID PI, Function &F ) LLVM_OVERRIDE  {
    return pm->findOrRunAnalysis(PI, &F, nullptr);
  }

   Pass * getAnalysisIfAvailable( AnalysisID ID, bool Direction ) const LLVM_OVERRIDE  {
    throw std::logic_error("The method or operation is not implemented.");
  }
#pragma endregion


#pragma region molly::MollyModuleProcessor
   llvm::AnalysisResolver * asResolver() LLVM_OVERRIDE {
   return this;
  }
#pragma endregion
}; // class MollyModuleProcessorImpl


MollyModuleProcessor *MollyModuleProcessor::create(MollyPassManager *pm, llvm::Module *module) {
  return new MollyModuleProcessorImpl(pm, module);
}
