
#include "molly/RegisterPasses.h"
#include "molly/LinkAllPasses.h"
#include "molly/FieldCodeGen.h"
#include "ScopStmtSplit.h"
#include "MollyInlinePrepa.h"

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

using namespace llvm;
using namespace std;


static cl::opt<bool> MollyEnabled("molly", cl::desc("Molly - Enable by default in -O3"), cl::init(false), cl::Optional);

static cl::opt<int> dbranch("malt", cl::desc("Debug Branch"), cl::init(0), cl::Optional);


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


static void registerMollyPasses(llvm::PassManagerBase &PM) {
  //PM.add(molly::createFieldDetectionAnalysisPass());


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

  // Unconditional inlining for field member function to make llvm.molly intrinsics visisble
  PM.add(molly::createMollyInlinePass()); 

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
  PM.add(molly::createFieldDistributionPass());
  PM.add(molly::createModuleFieldGenPass());
  PM.add(molly::createFieldCodeGenPass());
}


static void registerMollyEarlyAsPossiblePasses(const llvm::PassManagerBuilder &Builder, llvm::PassManagerBase &PM) {
  if (Builder.OptLevel == 0)
    return;

  if (!MollyEnabled) {
    return;
  }

  if (Builder.OptLevel != 3) {
    errs() << "Molly should only be run with -O3. Disabling Molly.\n";
    return;
  }

  registerMollyPasses(PM);
}


namespace molly {
  ModulePass *createFieldDetectionAnalysisPass();

  extern char &FieldDetectionAnalysisID;
}


static llvm::RegisterStandardPasses PassRegister(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, registerMollyEarlyAsPossiblePasses);

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
    USE(PassRegister);
    USE(MollyEnabled);
  }
}

