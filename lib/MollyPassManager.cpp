#include "MollyPassManager.h"



namespace llvm {
  class Module;
} // namespace llvm

using namespace llvm;
//using namespace polly;
using namespace molly;


namespace molly {

} // namespace molly


char molly::MollyPassManager::ID = 0;
molly::MollyPassManager::MollyPassManager() : ModulePass(ID)  {
}

void MollyPassManager::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
}

bool MollyPassManager::runOnModule(llvm::Module &M) {
  return false;
}

void MollyPassManager:: add(llvm::Pass *pass) {
}


MollyPassManager *MollyPassManager::create() {
  return new MollyPassManager();
}

