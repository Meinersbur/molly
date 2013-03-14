
#include "molly/RegisterPasses.h"
#include "molly/LinkAllPasses.h"
#include "molly/FieldCodeGen.h"
#include "ScopStmtSplit.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
using namespace std;

static cl::opt<bool> MollyEnabled("molly", cl::desc("Molly - Enable by default in -O3"), cl::init(false), cl::Optional);


static void initializeMollyPasses(PassRegistry &Registry) {
	initializeFieldDetectionAnalysisPass(Registry);
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
static void registerMollyPasses(llvm::PassManagerBase &PM) {
	//PM.add(molly::createFieldDetectionAnalysisPass());

  PM.add(molly::createScopStmtSplitPass());
	PM.add(molly::createFieldDistributionPass());
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

