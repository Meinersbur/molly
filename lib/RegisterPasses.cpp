#define DEBUG_TYPE "molly"
#include "molly/RegisterPasses.h"

#include "molly/LinkAllPasses.h"
#include "InlinePrepa.h"
#include "MollyUtils.h"
#include "MollyPassManager.h"

#include <polly/ScopPass.h>
#include <polly/RegisterPasses.h>
#include <polly/ScopInfo.h>
#include <polly/LinkAllPasses.h>
#include <polly/CodeGen/BlockGenerators.h>

#include <llvm/PassManager.h>
#include <llvm/PassRegistry.h>
#include <llvm/InitializePasses.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>

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

  //PM.add(llvm::createStripSymbolsPass(true));

  std::string infoDummy;
  auto OSorig = new raw_fd_ostream("0_orig.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OSorig, "Before any Molly-specific passes\n\n"));

  // Unconditional inlining for field member function to make llvm.molly intrinsics visible
  PM.add(molly::createMollyInlinePass()); 

  auto OSafterinline = new raw_fd_ostream("1_inlined.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OSafterinline, "After MollyInline pass\n\n"));

  // Cleanup after inlining
  PM.add(llvm::createCFGSimplificationPass()); // calls to __builtin_molly_ptr and load/store instructions must be in same BB
  PM.add(llvm::createPromoteMemoryToRegisterPass()); // Canonical inductionvar must be a register

  auto OStidy = new raw_fd_ostream("2_tidied.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OStidy, "After general cleanup necessary to recognize scop\n\n"));

  // Canonicalization
  // Partially copied from Polly's registerCanonicalicationPasses
  PM.add(llvm::createPromoteMemoryToRegisterPass());
  PM.add(llvm::createSROAPass());// Enhanced version of mem2reg; clang::CodeGen buts some temporaries into AllocaInsts
  //PM.add(llvm::createInstructionCombiningPass()); // FIXME: converts ZExt to ashr, which is not recognized by ScalarEvolution
  PM.add(llvm::createInstructionSimplifierPass());
  PM.add(llvm::createCFGSimplificationPass());
  PM.add(llvm::createTailCallEliminationPass());
  PM.add(llvm::createCFGSimplificationPass());
  PM.add(llvm::createReassociatePass());
  PM.add(llvm::createLoopRotatePass());
  //PM.add(llvm::createInstructionCombiningPass());
  PM.add(llvm::createInstructionSimplifierPass());
  if (!SCEVCodegen)
    PM.add(polly::createIndVarSimplifyPass());
  PM.add(polly::createCodePreparationPass());

  auto OScanonicalized = new raw_fd_ostream("3_canonicalized.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OScanonicalized, "After canonicalization\n\n"));
  PM.add(llvm::createVerifierPass());

  // Do the Molly thing
  // TODO: Configure to optLevel
  PM.add(createMollyPassManager());

  auto OSaftermolly = new raw_fd_ostream("6_mollied.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OSaftermolly, "After Molly did its work\n\n"));
  PM.add(llvm::createVerifierPass());

#ifndef NDEBUG
  // cleanup function
  PM.add(llvm::createCFGSimplificationPass());
  PM.add(llvm::createEarlyCSEPass());
  PM.add(llvm::createPromoteMemoryToRegisterPass());
  PM.add(llvm::createCorrelatedValuePropagationPass());
  PM.add(llvm::createInstructionCombiningPass());
  PM.add(llvm::createReassociatePass());
  PM.add(llvm::createLICMPass());
  PM.add(llvm::createIndVarSimplifyPass());  
  PM.add(llvm::createLoopDeletionPass());
  PM.add(llvm::createAggressiveDCEPass());
  PM.add(llvm::createCFGSimplificationPass());

  // cleanup module
  PM.add(llvm::createGlobalOptimizerPass());     
  PM.add(llvm::createIPSCCPPass());              
  //PM.add(llvm::createDeadArgEliminationPass());  
  PM.add(llvm::createGVNPass());
  PM.add(llvm::createGlobalDCEPass());        
  PM.add(llvm::createConstantMergePass());     

  auto OSaftercleanup = new raw_fd_ostream("7_cleaned.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OSaftercleanup, "After cleanup\n\n"));
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


static void registerMollyLastPasses(const llvm::PassManagerBuilder &Builder, llvm::PassManagerBase &PM) {
  std::string infoDummy;
  auto OSlast = new raw_fd_ostream("6_last.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OSlast, "Last\n\n"));
}


static llvm::RegisterStandardPasses OptPassRegister(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, registerMollyEarlyAsPossiblePasses);
static llvm::RegisterStandardPasses NoOptPassRegister(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0, registerMollyNoOptPasses);
static llvm::RegisterStandardPasses LastPassRegister(llvm::PassManagerBuilder::EP_OptimizerLast, registerMollyLastPasses);

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
