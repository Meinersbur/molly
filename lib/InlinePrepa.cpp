#include "InlinePrepa.h"

#include <llvm/Support/Compiler.h>
#include <llvm/Pass.h> 
#include <llvm/Transforms/IPO/InlinerPass.h>
#include <llvm/Analysis/InlineCost.h>

using namespace molly;
using namespace llvm;


namespace molly {
  // Mostly copied from InlineAlwys.cpp
  class MollyInlinePrepa : public llvm::Inliner {
 //InlineCostAnalysis *ICA;

public:
  // Use extremely low threshold.
  MollyInlinePrepa() : Inliner(ID, -2000000000, /*InsertLifetime*/ true)/*, ICA(0)*/ {
    initializeAlwaysInlinerPass(*PassRegistry::getPassRegistry());
  }

  MollyInlinePrepa(bool InsertLifetime)
      : Inliner(ID, -2000000000, InsertLifetime)/*, ICA(0)*/ {
    initializeAlwaysInlinerPass(*PassRegistry::getPassRegistry());
  }

  static char ID; // Pass identification, replacement for typeid

  virtual InlineCost getInlineCost(CallSite CS);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnSCC(CallGraphSCC &SCC);

#if 0
  using llvm::Pass::doFinalization;
  virtual bool doFinalization(CallGraph &CG) {
    return removeDeadFunctions(CG, /*AlwaysInlineOnly=*/ true);
  }
#endif
  }; // class MollyInlinePrepa
} // namespace molly


InlineCost MollyInlinePrepa::getInlineCost(CallSite CS) {
  Function *Callee = CS.getCalledFunction();

  // Only inline direct calls to functions with always-inline attributes
  // that are viable for inlining. FIXME: We shouldn't even get here for
  // declarations.
  if (Callee && !Callee->isDeclaration() &&
      Callee->hasFnAttribute( "molly_inline") /*&&
      ICA->isInlineViable(*Callee)*/)
    return InlineCost::getAlways();

  return InlineCost::getNever();
}

bool MollyInlinePrepa::runOnSCC(CallGraphSCC &SCC) {
  auto front = *SCC.begin();
  auto frontFunc = front->getFunction();
  if (frontFunc && frontFunc->getName() == "test") {
    int a = 0;
  }
  //ICA = &getAnalysis<InlineCostAnalysis>();
  return Inliner::runOnSCC(SCC);
}

void MollyInlinePrepa::getAnalysisUsage(AnalysisUsage &AU) const {
  //AU.addRequired<InlineCostAnalysis>();
  Inliner::getAnalysisUsage(AU);
}




char MollyInlinePrepa::ID = 0;
char &molly::MollyInlinePassID = MollyInlinePrepa::ID;
static RegisterPass<MollyInlinePrepa> ScopStmtSplitPassRegistration("molly-inline", "Molly - Inliner");

llvm::Pass *molly::createMollyInlinePass() {
  return new MollyInlinePrepa(false);
}
