#define DEBUG_TYPE "molly"
#include "molly/RegisterPasses.h"

#include "molly/LinkAllPasses.h"
#include "FieldCodeGen.h"
#include "ScopStmtSplit.h"
#include "MollyInlinePrepa.h"
#include "ScopDistribution.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/RegionPass.h"
#include "PatternSearchAnalysis.h"
#include <polly/LinkAllPasses.h>
#include <llvm/Transforms/Scalar.h>
#include <polly/ScopPass.h>
#include "ScopFieldCodeGen.h"
#include "GlobalPassManager.h"
#include <polly/RegisterPasses.h>
#include <polly/ScopInfo.h>
#include "MollyUtils.h"
#include "MollyContextPass.h"
#include "FieldDetection.h"
#include "llvm/Assembly/PrintModulePass.h"
#include <polly/PollyContextPass.h>
#include "InsertInOut.h"
#include "MollyPassManager.h"

using namespace llvm;
using namespace std;


cl::OptionCategory MollyCategory("Molly Options", "Configure memory optimizations");

static cl::opt<bool> MollyEnabled("molly", cl::desc("Molly - Enable by default in -O3"), cl::init(false), cl::Optional, cl::cat(MollyCategory));
static cl::opt<int> dbranch("malt", cl::desc("Debug Branch"), cl::init(0), cl::Optional, cl::cat(MollyCategory));


static void initializeMollyPasses(PassRegistry &Registry) {
  //initializeFieldDetectionAnalysisPass(Registry);
}
#if 0
namespace {
  class StaticInitializer {
  public:
    StaticInitializer() {
      PassRegistry &Registry = *PassRegistry::getPassRegistry();
    }
  };
}

static StaticInitializer InitializeErverything;
#endif

static int EmptyRegionPassInsts = 0;
class EmptyRegionPass : public llvm::RegionPass {
public:
  static char ID;
  bool hasRun;

  EmptyRegionPass() : RegionPass(ID), hasRun(false) {
    EmptyRegionPassInsts+=1;
  }
  ~EmptyRegionPass() {
    int a= 0;
  }
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }
  virtual bool runOnRegion(Region *R, RGPassManager &RGM) override {
    hasRun = true;
    return false;
  }

  virtual void assignPassManager(PMStack &a,  PassManagerType b) {
    RegionPass::assignPassManager(a,b);
  }
  virtual void preparePassManager(PMStack &a) {
    RegionPass::preparePassManager(a);
  }
};
static RegisterPass<EmptyRegionPass> EmptyRegionPassRegistration("emptyregion", "EmptyRegionPass", false, true);
char EmptyRegionPass::ID;


class Empty2FunctionPass : public llvm::FunctionPass {
public:
  static char ID;
  Empty2FunctionPass() : FunctionPass(ID) {}
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }
  virtual bool runOnFunction(Function &F) override {
    return false;
  }
  virtual void assignPassManager(PMStack &a,  PassManagerType b) {
    FunctionPass::assignPassManager(a,b);
  }
  virtual void preparePassManager(PMStack &a) {
    FunctionPass::preparePassManager(a);
  }
};
static RegisterPass<Empty2FunctionPass> Empty2FunctionPassRegistration("empty2function", "EmptyFunctionPass", false, true);
char Empty2FunctionPass::ID;



static int EmptyFunctionPassInsts = 0;
class EmptyFunctionPass : public llvm::FunctionPass {
public:
  static char ID;
  EmptyFunctionPass() : FunctionPass(ID) {
    EmptyFunctionPassInsts+=1;
  }
  ~EmptyFunctionPass() {
    int a= 0;
  }
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    if (dbranch == 0) {
    } else if (dbranch == 1) {
      AU.addRequired<RegionInfo>();
      AU.addRequired<EmptyRegionPass>();
    } else {
      AU.addRequired<Empty2FunctionPass>();
    }
    AU.setPreservesAll();
  }
  virtual bool runOnFunction(Function &F) override {
    auto RI = &getAnalysis<RegionInfo>();
    auto TopLevel = RI->getTopLevelRegion();
    //auto ER = &getAnalysis<EmptyRegionPass>(*TopLevel);
    return false;
  }

  virtual void assignPassManager(PMStack &a,  PassManagerType b) {
    FunctionPass::assignPassManager(a,b);
  }
  virtual void preparePassManager(PMStack &a) {
    FunctionPass::preparePassManager(a);
  }
};
static RegisterPass<EmptyFunctionPass> EmptyFunctionPassRegistration("emptyfunction", "EmptyFunctionPass", false, true);
char EmptyFunctionPass::ID;

static int EmptyModulePassInsts = 0;
class EmptyModulePass : public llvm::ModulePass {
public:
  static char ID;
  EmptyModulePass() : ModulePass(ID) {
    EmptyModulePassInsts+=1;
  }
  ~EmptyModulePass() {
    int a= 0;
  }
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    if (dbranch == 0)
      AU.addRequired<EmptyFunctionPass>();

    //AU.addRequired<EmptyRegionPass>();

    AU.setPreservesAll();
  }
  virtual bool runOnModule(Module &M) {
    auto ER = &getAnalysis<EmptyRegionPass>();
    auto EF = &getAnalysis<EmptyFunctionPass>();
    assert(ER->hasRun);
    return false;
  }

  virtual void assignPassManager(PMStack &a,  PassManagerType b) {
    ModulePass::assignPassManager(a,b);
  }
  virtual void preparePassManager(PMStack &a) {
    ModulePass::preparePassManager(a);
  }
};
static RegisterPass<EmptyModulePass> EmptyModulePassRegistration("emptymodule", "EmptyModulePass");
char EmptyModulePass::ID;



class Empty3FunctionPass : public llvm::FunctionPass {
public:
  static char ID;
  Empty3FunctionPass() : FunctionPass(ID) {}
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<EmptyModulePass>();
    AU.setPreservesAll();
  }
  virtual bool runOnFunction(Function &F) override {
    return false;
  }
  virtual void assignPassManager(PMStack &a,  PassManagerType b) {
    FunctionPass::assignPassManager(a,b);
  }
  virtual void preparePassManager(PMStack &a) {
    FunctionPass::preparePassManager(a);
  }
};
static RegisterPass<Empty3FunctionPass> Empty3FunctionPassRegistration("empty3function", "Empty3FunctionPass", false, true);
char Empty3FunctionPass::ID;


static void registerMollyPasses(llvm::PassManagerBase &PM, bool mollyEnabled, int optLevel) {
  if (!mollyEnabled) {
    llvm::errs() << "Molly: Loaded but disabled\n";
    //FIXME: Even if molly is disabled, we need to remove the molly intrinsics by some local access
    return;
  }

  llvm::errs() << "Molly: Enabled lvl " << optLevel << "\n";

#if 0
  //PM.add(new EmptyRegionPass());
  if (dbranch == 0) {
    PM.add(new EmptyFunctionPass());
    PM.add(new EmptyModulePass());
  } else if (dbranch == 1) {
    PM.add(new EmptyRegionPass());
    PM.add(new EmptyFunctionPass());
  } else if (dbranch == 2) {
    PM.add(new Empty2FunctionPass());
    PM.add(new EmptyFunctionPass());
  } else if (dbranch == 3) {
    PM.add(new EmptyModulePass());
    PM.add(new Empty3FunctionPass());
  } else {
  }
#endif

  //PM.add(molly::createFieldDetectionAnalysisPass());

  std::string infoDummy;
  auto  OSorig = new raw_fd_ostream("0_orig.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OSorig, false, "Before ani Molly-specific passes\n\n"));

  // Unconditional inlining for field member function to make llvm.molly intrinsics visisble
  PM.add(molly::createMollyInlinePass()); 

  auto OSafterinline = new raw_fd_ostream("1_inlined.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OSafterinline, false, "After MollyInline pass\n\n"));

  PM.add(llvm::createCFGSimplificationPass()); // calls to __builtin_molly_ptr and load/store instructions must be in same BB
  PM.add(llvm::createPromoteMemoryToRegisterPass()); // Canonical inductionvar must be a register

  auto OStidy = new raw_fd_ostream("2_tidied.ll", infoDummy);
  PM.add(llvm::createPrintModulePass(OStidy, false, "After general cleanup necessary to recognize scop\n\n"));

  // Cleanup after inlining
  //FIXME: Which is the correct one?
  //PM.add(createConstantPropagationPass());
  //PM.add(createCorrelatedValuePropagationPass());
  //PM.add(createSCCPPass()); 
  //PM.add(createJumpThreadingPass());

  //PM.add(createDeadInstEliminationPass());
  //PM.add(createDeadCodeEliminationPass());
  //PM.add(createAggressiveDCEPass());  
  //PM.add(createCFGSimplificationPass());

  //PM.add(polly::createScopInfoPass());
  //PM.add(molly::createScopStmtSplitPass());
  //PM.add(polly::createDependencesPass());
  //PM.add(new PatternSearchAnalysis());

  // Decide where to natively store values (home node)
  //PM.add(molly::createFieldDistributionPass());

  // PM.add(molly::createScopDistributionPass());

  // Emit builtin functions
  //PM.add(molly::createModuleFieldGenPass());
  //PM.add(molly::createScopFieldCodeGenPass());
  //PM.add(molly::createFieldCodeGenPass());

  //TODO: Adjust to OptLevel
  //auto gpm = createGlobalPassManager();
  
  PM.add(createMollyPassManager());

#if 0
  PM.add(createPollyContextPass(true));

  //gpm->addPass<molly::MollyContextPass>(); // ModulePass (should be preserved until the end of the Molly chains)
  auto mollyContextPass = createPass<molly::MollyContextPass>();
  PM.add(mollyContextPass);
  //gpm->addPass<molly::FieldDetectionAnalysis>(); // ModulePass
  auto fieldDetectionPass = molly::createFieldDetectionAnalysisPass();
  PM.add(fieldDetectionPass);

  // Schedule before because IndependentBlocks invalidates all the Field passes
  //PM.add(createIndependentBlocksPass()); // FunctionPass

  // Decide where to natively store values (home node)
  //gpm->addPassID(FieldDistributionPassID); // ModulePass
  PM.add(createPassFromId(FieldDistributionPassID));

  // Emit builtin_molly_ functions and replace main
  //gpm->addPassID(ModuleFieldGenPassID); // ModulePass
  PM.add(createPassFromId(ModuleFieldGenPassID));

  // SCoPs are detected from here on; It must be preserved until the CodeGeneration passes, otherwise changes made to will be forgotten
  //gpm->addPass<polly::ScopInfo>(); // RegionPass
  PM.add(createPassFromId(ScopInfo::ID));

  //PM.add(createPassFromId(molly::InsertInOutPassID));

  // Decide where to execute statements
  //gpm->addPassID(ScopDistributionPassID); // ScopPass
  PM.add(createPassFromId(ScopDistributionPassID));

  // Insert communication code into SCoP and restrict instruction to their node where are are executed
  //gpm->addPassID(ScopFieldCodeGenPassID); // ScopPass
  PM.add(createPassFromId(ScopFieldCodeGenPassID));

#pragma region Polly
  // These are polly's optimization and code generation phases; They are fed using the modified SCoPs from Molly
  // FIXME: Improve interaction between Polly and Molly

  switch (Optimizer) {
  case OPTIMIZER_NONE:
    break; /* Do nothing */

#ifdef SCOPLIB_FOUND
  case OPTIMIZER_POCC:
    PM.add(polly::createPoccPass());
    break;
#endif

#ifdef PLUTO_FOUND
  case OPTIMIZER_PLUTO:
    PM.add(polly::createPlutoOptimizerPass());
    break;
#endif

  case OPTIMIZER_ISL:
    PM.add(polly::createIslScheduleOptimizerPass());
    break;
  }

  switch (CodeGenerator) {
#ifdef CLOOG_FOUND
  case CODEGEN_CLOOG:
    PM.add(polly::createCodeGenerationPass());
    if (PollyVectorizerChoice == VECTORIZER_BB) {
      VectorizeConfig C;
      C.FastDep = true;
      PM.add(createBBVectorizePass(C));
    }
    break;
#endif
  case CODEGEN_ISL:
    PM.add(polly::createIslCodeGenerationPass());
    break;
  case CODEGEN_NONE:
    break;
  }
#pragma endregion

  // Replace all remaining field accesses by calls to runtime
  //gpm->addPassID(FieldCodeGenPassID); // FunctionPass
  PM.add(createPassFromId(FieldCodeGenPassID));

  PM.add(createPollyContextRelease());

  //PM.unpreserve(mollyContextPass);
  //PM.unpreserve(fieldDetectionPass);

  //PM.add(gpm); //TODO: GlobalPassManager knows which ModuleAnalyses it could need and ask PM for it
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


namespace molly {
  ModulePass *createFieldDetectionAnalysisPass();

  //extern char &FieldDetectionAnalysisID;
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

    molly::createFieldDetectionAnalysisPass();

#define USE(val) \
  srand( *((unsigned int*)&(val)) )

    // Do something with the static variable to ensure it is linked into the library
    USE(OptPassRegister);
    USE(NoOptPassRegister);
    USE(MollyEnabled);
  }
} // namespace molly
