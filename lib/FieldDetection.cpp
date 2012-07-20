
#define DEBUG_TYPE "molly"
#include "llvm\Support\Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "molly/FieldDetection.h"
#include "molly\LinkAllPasses.h"

using namespace llvm;
using namespace molly;

static cl::opt<bool> EnableFieldDetection("molly-field", cl::desc("Enabke field detection"), cl::Hidden, cl::init(true));

STATISTIC(NumGlobalFields, "Number of detected global fields");

char FieldDetectionAnalysis::ID = 0;



INITIALIZE_PASS_BEGIN(FieldDetectionAnalysis, "molly-detect",
					  "Molly - Detect fields", false, false)
INITIALIZE_PASS_END(FieldDetectionAnalysis, "molly-detect",
					"Molly - Find fields", false, false)


ModulePass *molly::createFieldDetectionAnalysisPass() {
	return new FieldDetectionAnalysis();
}

