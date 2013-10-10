#include "MollyRegionProcessor.h"

#include <llvm/Support/Compiler.h>
#include <llvm/PassAnalysisSupport.h>
#include "MollyPassManager.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
//using isl::enwrap;


namespace {

  class MollyRegionResolver : public AnalysisResolver {
  private:
    MollyPassManager *pm;
    Region *region;

  public:
    MollyRegionResolver(MollyPassManager *pm, Region *region) 
      :  AnalysisResolver(*static_cast<PMDataManager*>(nullptr)), pm(pm), region(region) {}

    Pass * findImplPass(AnalysisID PI) LLVM_OVERRIDE {
      return pm->findOrRunAnalysis(PI, nullptr, region);
    }

    Pass * findImplPass(Pass *P, AnalysisID PI, Function &F) LLVM_OVERRIDE {
      return pm->findOrRunAnalysis(PI, &F, region);
    }

    Pass * getAnalysisIfAvailable(AnalysisID ID, bool Direction) const LLVM_OVERRIDE {
      return pm->findAnalysis(ID, nullptr, region);
    }
  }; // class MollyRegionResolver


  class MollyRegionProcessorImpl : public MollyRegionProcessor  {
    MollyPassManager *pm;
    llvm::Region *region;

  public:
    MollyRegionProcessorImpl(MollyPassManager *pm, llvm::Region *region) : pm(pm), region(region) {}
  }; // class MollyRegionProcessorImpl

} // namespace 


MollyRegionProcessor *MollyRegionProcessor::create(MollyPassManager *pm, llvm::Region *region) {
  return new MollyRegionProcessorImpl(pm, region);
}
AnalysisResolver *MollyRegionProcessor::createResolver(MollyPassManager *pm, llvm::Region *region) {
  return new MollyRegionResolver(pm, region);
}
