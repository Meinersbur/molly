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

static int printerval = 8;
static void addPrintModulePass(llvm::PassManagerBase &PM, StringRef name) {
  std::string infoDummy;
  auto OS = new raw_fd_ostream((Twine(printerval) + Twine('_') + name + ".ll").str().c_str(), infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OS, (Twine("After ") + name + "\n\n").str()));
  printerval += 1;
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
  PM.add(llvm::createInstructionCombiningPass());
  //PM.add(llvm::createInstructionSimplifierPass());
  if (!SCEVCodegen)
    PM.add(polly::createIndVarSimplifyPass());
  PM.add(polly::createCodePreparationPass());

  //PM.add(llvm::createInstructionCombiningPass());
  PM.add(llvm::createInstructionSimplifierPass());

  auto OScanonicalized = new raw_fd_ostream("3_canonicalized.ll", infoDummy, sys::fs::F_Text);
  PM.add(llvm::createPrintModulePass(*OScanonicalized, "After canonicalization\n\n"));
  PM.add(llvm::createVerifierPass());



  // Do the Molly thing
  // TODO: Configure to optLevel
  PM.add(createMollyPassManager());
  addPrintModulePass(PM, "Molly");

  PM.add(llvm::createVerifierPass());

#ifndef NDEBUG
  // cleanup function
  PM.add(llvm::createCFGSimplificationPass());
  addPrintModulePass(PM, "ControlFlowGraphSimplification");

  PM.add(llvm::createEarlyCSEPass());
  addPrintModulePass(PM, "EarlyCSE");

  PM.add(llvm::createPromoteMemoryToRegisterPass());
  addPrintModulePass(PM, "Mem2Reg");

  PM.add(llvm::createCorrelatedValuePropagationPass());
  addPrintModulePass(PM, "CorrelatedValuePropagation");

  PM.add(llvm::createInstructionCombiningPass());
  addPrintModulePass(PM, "InstructionCombining");

  PM.add(llvm::createReassociatePass());
  addPrintModulePass(PM, "Reassociate");

  PM.add(llvm::createLICMPass());
  addPrintModulePass(PM, "LoopInvariantCodeMotion");

  PM.add(llvm::createIndVarSimplifyPass());
  addPrintModulePass(PM, "IndVarSimplify");

  PM.add(llvm::createLoopDeletionPass());
  addPrintModulePass(PM, "LoopDeletion");

  PM.add(llvm::createAggressiveDCEPass());
  addPrintModulePass(PM, "AggressiveDeadCodeElimination");

  PM.add(llvm::createCFGSimplificationPass());
  addPrintModulePass(PM, "ControlFlowGrapSimplification");

  // cleanup module
  PM.add(llvm::createGlobalOptimizerPass());
  addPrintModulePass(PM, "GlobalOptimizer");

  PM.add(llvm::createIPSCCPPass());
  addPrintModulePass(PM, "IPSCC");

  //PM.add(llvm::createDeadArgEliminationPass());  
  PM.add(llvm::createGVNPass());
  addPrintModulePass(PM, "GlobalValueNumbering");

  PM.add(llvm::createGlobalDCEPass());
  addPrintModulePass(PM, "GlobalDeadCodeElimination");

  PM.add(llvm::createConstantMergePass());
  addPrintModulePass(PM, "ConstantMerge");

  addPrintModulePass(PM, "Cleaned");
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
    if (std::getenv("bar") != (char*)-1)
      return;

    //molly::createFieldDetectionAnalysisPass();

#define USE(val) \
  srand( *((unsigned int*)&(val)) )

    // Do something with the static variable to ensure it is linked into the library
    USE(OptPassRegister);
    USE(NoOptPassRegister);
    USE(MollyEnabled);
  }


  // Polly moved its static initializer to polly.cpp in LLVMPolly, which we do not load
  // Therefore we have to initialize Polly by ourselves
  //FIXME: static initialization is a chaos, this could should be cleaned up!
  struct StaticInitializer {
    StaticInitializer() {
      llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
      polly::initializePollyPasses(Registry);
    }
  } _initializer;

} // namespace molly
