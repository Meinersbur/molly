#include "PatternSearchAnalysis.h"

#include <llvm/ADT/SmallVector.h>
#include <polly/ScopInfo.h>
#include <polly/Dependences.h>
#include "islpp/UnionMap.h"

using namespace molly;
using namespace polly;
using namespace llvm;



void PatternSearchAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<RegionInfo>();
  AU.addRequired<ScopInfo>();
  AU.addRequired<Dependences>();

  AU.setPreservesAll();
}


void PatternSearchAnalysis::runOnScop(polly::Scop *scop) {
  //auto deps = &getAnalysis<Dependences>(scop->getRegion());

  //auto flow = isl::wrap(deps->getDependences(Dependences::TYPE_RAW)); // Flow dependences
  //flow.dump();
}


static void collectRegions(Region *R, SmallVectorImpl<Region*> &list) {
  list.push_back(R);
  for (Region::iterator I = R->begin(), E = R->end(); I != E; ++I)
    collectRegions(*I, list);
}

bool PatternSearchAnalysis::runOnFunction(llvm::Function &F) {
  auto RI = &getAnalysis<RegionInfo>();

  SmallVector<Region*, 8> regions;
  collectRegions(RI->getTopLevelRegion(), regions);

  for (auto itRegion = regions.begin(), endRegion = regions.end(); itRegion!=endRegion; ++itRegion) {
    auto region = *itRegion;
    //auto SI = &getAnalysis<ScopInfo>(*region);
    //auto scop = SI->getScop();
    //if (!scop)
    //  continue;

    //runOnScop(scop);
  }
  return false;
}


char PatternSearchAnalysis::ID = 0;
static RegisterPass<PatternSearchAnalysis> ScopDistributionRegistration("molly-patterns", "Molly - Search for access patterns", false, true);
