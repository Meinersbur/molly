#ifndef MOLLY_LINKALLPASSES_H
#define MOLLY_LINKALLPASSES_H

#include <cstdlib>

namespace llvm {
  class Pass;
  class PassInfo;
  class PassRegistry;
  class RegionPass;
  class ModulePass;
}

using namespace llvm;

namespace molly {
  //ModulePass *createFieldDetectionAnalysisPass();

//extern const char &FieldDistributionPassID;
  //ModulePass *createFieldDistributionPass();

  //extern char &FieldDetectionAnalysisID;
}

using namespace molly;

namespace {
  struct MollyForcePassLinking {
  MollyForcePassLinking() {
    // We must reference the passes in such a way that compilers will not
    // delete it all as dead code, even with whole program optimization,
    // yet is effectively a NO-OP. As the compiler isn't smart enough
    // to know that getenv() never returns -1, this will do the job.
    if (std::getenv("bar") != (char*) -1)
    return;

     //createAffSCEVItTesterPass();
    //molly::createFieldDetectionAnalysisPass();
  }
  } MollyForcePassLinking; // Force link by creating a global definition.
} // namespace

namespace llvm {
  class PassRegistry;
  //void initializeFieldDetectionAnalysisPass(llvm::PassRegistry &);
}

#endif /* MOLLY_LINKALLPASSES_H */
