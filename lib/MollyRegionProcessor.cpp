#include "MollyRegionProcessor.h"

#include "llvm\Support\Compiler.h"
#include "llvm\PassAnalysisSupport.h"
#include "MollyPassManager.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
//using isl::enwrap;


class MollyRegionProcessorImpl : public MollyRegionProcessor, private AnalysisResolver  {
  MollyPassManager *pm;
  llvm::Region *region;

public:
  MollyRegionProcessorImpl(MollyPassManager *pm, llvm::Region *region) : pm(pm), region(region), AnalysisResolver(*static_cast<PMDataManager*>(nullptr)) {}


#pragma region llvm::AnalysisResolver
  Pass * findImplPass( AnalysisID PI ) LLVM_OVERRIDE {
    return pm->findOrRunAnalysis(PI, nullptr, region);
  }

  Pass * findImplPass( Pass *P, AnalysisID PI, Function &F ) LLVM_OVERRIDE {
    return pm->findOrRunAnalysis(PI, &F, region);
  }

  Pass * getAnalysisIfAvailable(AnalysisID ID, bool Direction) const LLVM_OVERRIDE {
    return pm->findAnalysis(ID, nullptr, region);
  }
#pragma endregion


#pragma region polly::MollyRegionProcessor
  llvm::AnalysisResolver *asResolver() LLVM_OVERRIDE {
    return this;
  }
#pragma endregion
};


MollyRegionProcessor *MollyRegionProcessor::create(MollyPassManager *pm, llvm::Region *region) {
  return new MollyRegionProcessorImpl(pm, region);
}
