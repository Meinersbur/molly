
#define DEBUG_TYPE "molly"
#include "llvm\Support\Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "molly/FieldDetection.h"
#include "molly/LinkAllPasses.h"

#include <llvm\IR\DerivedTypes.h>

using namespace llvm;
using namespace molly;

static cl::opt<bool> EnableFieldDetection("molly-field", cl::desc("Enable field detection"), cl::Hidden, cl::init(true));
static RegisterPass<FieldDetectionAnalysis> FieldAnalysisRegistration("molly-fieldanalysis", "Molly - Field detection", false, true);

STATISTIC(NumGlobalFields, "Number of detected global fields");

char FieldDetectionAnalysis::ID = 0;
char &molly::FieldDetectionAnalysisID = FieldDetectionAnalysis::ID;


INITIALIZE_PASS_BEGIN(FieldDetectionAnalysis, "molly-detect", "Molly - Detect fields", false, false)

INITIALIZE_PASS_END(FieldDetectionAnalysis, "molly-detect", "Molly - Find fields", false, false)

bool FieldDetectionAnalysis::runOnModule(Module &M) {
	std::string test("xyz");

	M.dump();

	auto &glist = M.getGlobalList();
	auto &flist = M.getFunctionList();
	auto &alist = M.getAliasList();
	auto &mlist = M.getNamedMDList();
	auto &vlist = M.getValueSymbolTable();

	{
		auto end = M.global_end();
		for (auto it = M.global_begin(); it != end; ++it) {
			auto &glob = *it;
			PointerType *pty = glob.getType();
			Type *ty = pty->getElementType();
			if (isa<llvm::StructType>(ty)) {
				StructType *sty = cast<StructType>(ty);
				if (!sty->isLiteral()) { sty->dump();
				auto name = sty->getName();
				if (name == "class.Array1D") {
					// Found a field!
					int a = 0;
					//glob.setM
				}
				}
			}
		}
	}

	{
	auto end = M.end();
	for (auto it = M.begin(); it != end; ++it) {
	}
	}

	{
	auto end = M.alias_end();
	for (auto it = M.alias_begin(); it != end; ++it) {
	}
	}

		{
	auto end = M.named_metadata_end();
	for (auto it = M.named_metadata_begin(); it != end; ++it) {
	}
	}



	return false;
}

ModulePass *molly::createFieldDetectionAnalysisPass() {
	return new FieldDetectionAnalysis();
}


