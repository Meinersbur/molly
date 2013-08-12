#define DEBUG_TYPE "molly"
#include "molly/RegisterPasses.h"

#include "molly/LinkAllPasses.h"
#include "MollyInlinePrepa.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/RegionPass.h"
#include <polly/LinkAllPasses.h>
#include <llvm/Transforms/Scalar.h>
#include <polly/ScopPass.h>
#include <polly/RegisterPasses.h>
#include <polly/ScopInfo.h>
#include "MollyUtils.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "MollyPassManager.h"
#include <polly/CodeGen/BlockGenerators.h>

using namespace llvm;
using namespace std;


cl::OptionCategory MollyCategory("Molly Options", "Configure memory optimizations");

static cl::opt<bool> MollyEnabled("molly", cl::desc("Molly - Enable by default in -O3"), cl::init(false), cl::Optional, cl::cat(MollyCategory));
static cl::opt<int> dbranch("malt", cl::desc("Debug Branch"), cl::init(0), cl::Optional, cl::cat(MollyCategory));


static void initializeMollyPasses(PassRegistry &Registry) {
  //initializeFieldDetectionAnalysisPass(Registry);
}


static void registerMollyPasses(llvm::PassManagerBase &PM, bool mollyEnabled, int optLevel) {
  if (!mollyEnabled) {
    llvm::errs() << "Molly: Loaded but disabled\n";
    //FIXME: Even if molly is disabled, we need to remove the molly intrinsics by some local access
    return;
  }

  llvm::errs() << "Molly: Enabled lvl " << optLevel << "\n";


  std::string infoDummy;
  auto  OSorig = new raw_fd_ostream("0_orig.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OSorig, false, "Before ani Molly-specific passes\n\n"));

  // Unconditional inlining for field member function to make llvm.molly intrinsics visisble
  PM.add(molly::createMollyInlinePass()); 

  auto OSafterinline = new raw_fd_ostream("1_inlined.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OSafterinline, false, "After MollyInline pass\n\n"));

  // Cleanup after inlining
  PM.add(llvm::createCFGSimplificationPass()); // calls to __builtin_molly_ptr and load/store instructions must be in same BB
  PM.add(llvm::createPromoteMemoryToRegisterPass()); // Canonical inductionvar must be a register

  auto OStidy = new raw_fd_ostream("2_tidied.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OStidy, false, "After general cleanup necessary to recognize scop\n\n"));

  // Canonicalization
  // Copied from Polly's registerCanonicalicationPasses
  PM.add(llvm::createPromoteMemoryToRegisterPass());
  PM.add(llvm::createInstructionCombiningPass());
  PM.add(llvm::createCFGSimplificationPass());
  PM.add(llvm::createTailCallEliminationPass());
  PM.add(llvm::createCFGSimplificationPass());
  PM.add(llvm::createReassociatePass());
  PM.add(llvm::createLoopRotatePass());
  PM.add(llvm::createInstructionCombiningPass());
  if (!SCEVCodegen)
    PM.add(polly::createIndVarSimplifyPass());
  PM.add(polly::createCodePreparationPass());

  auto OScanonicalized = new raw_fd_ostream("3_canonicalized.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OScanonicalized, false, "After canonicalization\n\n"));

  // Do the Molly thing
  // TODO: Configure to optLevel
  PM.add(createMollyPassManager());

  auto OSaftermolly = new raw_fd_ostream("4_mollied.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OSaftermolly, false, "After Molly did its work\n\n"));

#ifndef NDEBUG
  // Cleanup
  //PM.add(llvm::createCFGSimplificationPass());
  //PM.add(llvm::createPromoteMemoryToRegisterPass());
#endif
}


static void registerMollyNoOptPasses(const llvm::PassManagerBuilder &Builder, llvm::PassManagerBase &PM) {
  //REMINDER: Usually we'd disable molly without optimization, but for debugging, enabled it
  //registerMollyPasses(PM, false, 0);
  registerMollyPasses(PM, MollyEnabled, 0);
}


static void registerMollyEarlyAsPossiblePasses(const llvm::PassManagerBuilder &Builder, llvm::PassManagerBase &PM) {
  registerMollyPasses(PM, MollyEnabled, Builder.OptLevel);
}


static llvm::RegisterStandardPasses OptPassRegister(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, registerMollyEarlyAsPossiblePasses);
static llvm::RegisterStandardPasses NoOptPassRegister(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, registerMollyNoOptPasses);

namespace molly {
  void forceLinkPassRegistration() {
    // We must reference the passes in such a way that compilers will not
    // delete it all as dead code, even with whole program optimization,
    // yet is effectively a NO-OP. As the compiler isn't smart enough
    // to know that getenv() never returns -1, this will do the job.
    if (std::getenv("bar") != (char*) -1)
      return;

    //molly::createFieldDetectionAnalysisPass();

#define USE(val) \
  srand( *((unsigned int*)&(val)) )

    // Do something with the static variable to ensure it is linked into the library
    USE(OptPassRegister);
    USE(NoOptPassRegister);
    USE(MollyEnabled);
  }
} // namespace molly
