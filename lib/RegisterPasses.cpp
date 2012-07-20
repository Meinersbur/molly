
#include "molly/RegisterPasses.h"
#include "molly/LinkAllPasses.h"

#include "llvm\Support\CommandLine.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

static cl::opt<bool> MollyEnabled("molly", cl::desc("Enable Nolly by default in -O3"), cl::init(false),cl::ZeroOrMore);


static void initializeMollyPasses(PassRegistry &Registry) {
	initializeFieldDetectionAnalysisPass(Registry);
}

namespace {
	class StaticInitializer {
	public:
		StaticInitializer() {
			PassRegistry &Registry = *PassRegistry::getPassRegistry();
		}
	};
}

static StaticInitializer InitializeErverything;

static void registerMollyPasses(llvm::PassManagerBase &PM) {
	PM.add(molly::createFieldDetectionAnalysisPass());
}


static
	void registerMollyEarlyAsPossiblePasses(const llvm::PassManagerBuilder &Builder, llvm::PassManagerBase &PM) {

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


static llvm::RegisterStandardPasses
	PassRegister(llvm::PassManagerBuilder::EP_EarlyAsPossible,  registerMollyEarlyAsPossiblePasses);

