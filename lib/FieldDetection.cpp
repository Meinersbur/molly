
#define DEBUG_TYPE "molly"
#include "llvm\Support\Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "molly/FieldDetection.h"
#include "molly/LinkAllPasses.h"

#include <llvm\IR\DerivedTypes.h>

#include "FieldVariable.h"
#include "FieldType.h"
#include "MollyContextPass.h"
#include "MollyContext.h"

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
  auto mollyContext = getAnalysis<MollyContextPass>().getMollyContext();

  std::string test("xyz");

  M.dump();

  auto &glist = M.getGlobalList();
  auto &flist = M.getFunctionList();
  auto &alist = M.getAliasList();
  auto &mlist = M.getNamedMDList();
  auto &vlist = M.getValueSymbolTable();

  auto fieldsMD = M.getNamedMetadata("molly.fields");
  auto numFields = fieldsMD->getNumOperands();

  for (unsigned i = 0; i < numFields; i+=1) {
    auto fieldMD = fieldsMD->getOperand(i);
    FieldType *field = FieldType::createFromMetadata(mollyContext, &M, fieldMD);
    fieldTypes[field->getType()] = field;
  }


#if 0
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
          auto &field = fields[&glob];
          if (!field) {
            // Field not known yet
           // field = Field::create (&glob);
          }

          auto useEnd = glob.use_end();
          for (auto useIt = glob.use_begin(); useIt != useEnd; ++useIt) {
            User *user = *useIt;
            //user->dump();
          }
        }
        }
      }
    }
  }
#endif

  return false;
}

ModulePass *molly::createFieldDetectionAnalysisPass() {
  return new FieldDetectionAnalysis();
}


