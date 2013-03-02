
#include <polly/ScopPass.h>
#include "molly/FieldCodeGen.h"
#include "molly/FieldDistribution.h"
#include "molly/FieldDetection.h"
#include "MollyContextPass.h"
#include "FieldAccess.h"
#include "FieldType.h"
#include "FieldVariable.h"

#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <polly/MollyMeta.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
//#include <llvm/Transforms/Utils/ModuleUtils.h> // appendToGlobalCtors


using namespace llvm;
using namespace polly;
using namespace molly;


namespace molly {
  class FieldScopCodeGen : public ScopPass {

  public:
    static char ID;
    FieldScopCodeGen() : ScopPass(ID) {
    }

    virtual const char *getPassName() const {
      return "FieldScopCodeGen";
    }

    virtual bool runOnScop(Scop &S) {
      return false;
    }
  }; // FieldCodeGen






  class FieldCodeGen : public FunctionPass {
  public:
    static char ID;
    FieldCodeGen() : FunctionPass(ID) {
    }

    virtual const char *getPassName() const {
      return "FieldCodeGen";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MollyContextPass>();
      AU.addRequiredID(FieldDistributionPassID); 
      AU.addRequired<FieldDetectionAnalysis>();
    }




    virtual bool runOnFunction(Function &func);

  protected:
    bool emitFieldFunctions();

  }; // FieldCodeGen


} // namespace molly


/// Convenience function to get a list if instructions so we can modify the function without invalidating iterators
/// Should be more effective if we filter useful instructions first (lambda?)
static void collectInstructionList(Function *func, SmallVectorImpl<Instruction*> &list) {
  auto bbList = &func->getBasicBlockList();
  for (auto it = bbList->begin(), end = bbList->end(); it!=end; ++it) {
    auto bb = &*it;
    auto instList = &bb->getInstList();
    for (auto it = instList->begin(), end = instList->end(); it!=end; ++it) {
      auto *inst = &*it;
      list.push_back(inst);
    }
  }
}


bool FieldCodeGen::runOnFunction(Function &func) {
  if (func.getName() == "main") {
    int a = 0;
  }

  MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
  auto &fields = getAnalysis<FieldDetectionAnalysis>();

  
  //getAnalysis<FieldDetectionAnalysis>();
  bool changed = false;
  SmallVector<Instruction*, 16> instrs;
  collectInstructionList(&func, instrs);

  for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
    auto instr = *it;

    auto access = fields.getFieldAccess(instr);
    if (!access.isValid())
      continue;  // Not an access to a field

    auto fieldTy = access.getFieldType();
    auto fieldVar = access.getFieldVariable();
    auto islocalFunc = fieldTy->getIsLocalFunc();

    auto accessor = access.getAccessor();
    auto ref = access.getFieldCall();
    auto subscript = access.getSubscript();
    auto islocalfunc = fieldTy->getIsLocalFunc();

    auto bbBefore = instr->getParent();
    auto bbConditional = SplitBlock(bbBefore, instr, this);
    auto bbAfter = SplitBlock(bbConditional, instr->getNextNode(), this); //TODO: What if there is no next node

    bbBefore->getTerminator()->eraseFromParent();
    IRBuilder<> builder(bbBefore);
    auto isLocalResult = builder.CreateCall2(islocalfunc, fieldVar->getVariable(), subscript);
    builder.CreateCondBr(isLocalResult, bbConditional, bbAfter);
    changed = true;
  }

  bool funcEmitted = emitFieldFunctions();

  return changed || funcEmitted;
}



bool FieldCodeGen::emitFieldFunctions() {
  auto &fields = getAnalysis<FieldDetectionAnalysis>();
 auto &fieldTypes = fields.getFieldTypes();
 bool changed = false;

 for (auto it = fieldTypes.begin(), end = fieldTypes.end(); it!=end; ++it) {
   auto fieldTy = it->second;

   auto *isLocalFunc = fieldTy->getIsLocalFunc();
   if (isLocalFunc->empty()) {
     fieldTy->emitIsLocalFunc();
     return true;
   }
 }

 return changed;
}





char FieldCodeGen::ID = 0;
static RegisterPass<FieldCodeGen> FieldCodeGenRegistration("molly-fieldcodegen", "Molly - Code generation", false, false);

char FieldScopCodeGen::ID = 0;
static RegisterPass<FieldScopCodeGen> FieldScopCodeGenRegistration("molly-fieldscopcodegen", "Molly - Modify SCoPs", false, false);


ScopPass *molly::createFieldScopCodeGenPass() {
  return new FieldScopCodeGen();
}

FunctionPass *molly::createFieldCodeGenPass() {
  return new FieldCodeGen();
}
