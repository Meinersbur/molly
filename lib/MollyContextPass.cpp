#include "MollyContextPass.h"

#include "MollyContext.h"

#include <llvm/IR/Module.h>

using namespace llvm;
using namespace molly;


bool MollyContextPass::runOnModule(Module &M) {
  if (!context)
    context = MollyContext::create(&M.getContext());

  return false;
}



static RegisterPass<MollyContextPass> FieldAnalysisRegistration("molly-context", "Molly - Context", false, true);


