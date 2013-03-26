
#include <polly/ScopPass.h>
#include "molly/FieldCodeGen.h"
#include "molly/FieldDistribution.h"
#include "molly/FieldDetection.h"
#include "MollyContextPass.h"
#include "MollyFieldAccess.h"
#include "FieldType.h"
#include "FieldVariable.h"
#include "MollyUtils.h"
#include "MollyFieldAccess.h"

#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
//#include <polly/LinkAllPasses.h>
#include <polly/ScopPass.h>
//#include <llvm/Transforms/Utils/ModuleUtils.h> // appendToGlobalCtors


using namespace llvm;
using namespace molly;

using polly::ScopPass;
using polly::Scop;


namespace molly {
  class FieldScopCodeGen : public polly::ScopPass {

  public:
    static char ID;
    FieldScopCodeGen() : ScopPass(ID) {
    }

    virtual const char *getPassName() const {
      return "FieldScopCodeGen";
    }

    virtual bool runOnScop(Scop &S);
  }; // FieldCodeGen


  bool FieldScopCodeGen::runOnScop(Scop &S) {
    //auto pollyGen = createIslCodeGenerationPass();

    return false;
  }




  class FieldCodeGen : public FunctionPass {
  private:
    bool changed;

    FieldDetectionAnalysis *fields;

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

    void emitRead(MollyFieldAccess &access);
    void emitWrite(MollyFieldAccess &access);
    void emitAccess(MollyFieldAccess &access);

    virtual bool runOnFunction(Function &func);

  protected:
    bool emitFieldFunctions();

  }; // FieldCodeGen


} // namespace molly



void FieldCodeGen::emitRead(MollyFieldAccess &access){
  assert(access.isRead());
  auto accessor = access.getAccessor();
  auto bb = accessor->getParent();
  auto nDims = access.getNumDims();

  IRBuilder<> builder(bb);
  builder.SetInsertPoint(accessor->getNextNode()); // Behind the old load instr

  auto buf = builder.CreateAlloca(access.getElementType(), NULL, "getbuf");

  SmallVector<Value*,6> args;
  args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
  args.push_back(buf);
  for (auto d = nDims-nDims; d < nDims; d+=1) {
    auto coord = access.getCoordinate(d);
    args.push_back(coord);
  }

  builder.CreateCall(access.getFieldType()->getFuncGetBroadcast(), args, "getcall");
  auto load = builder.CreateLoad(buf, "loadget");
  accessor->replaceAllUsesWith(load);
}


void FieldCodeGen::emitWrite(MollyFieldAccess &access) {
  //TODO: Support structs
  assert(access.isWrite());
  auto accessor = cast<StoreInst>(access.getAccessor());
  auto bb = accessor->getParent();
  auto nDims = access.getNumDims();

  IRBuilder<> builder(bb);
  builder.SetInsertPoint(accessor->getNextNode()); // Behind the old store instr

  SmallVector<Value*,6> args;
  args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
  args.push_back(accessor);
  for (auto d = nDims-nDims; d < nDims; d+=1) {
    auto coord = access.getCoordinate(d);
    args.push_back(coord);
  }

  builder.CreateCall(access.getFieldType()->getFuncSetBroadcast(), args, "setcall");
}


void FieldCodeGen::emitAccess(MollyFieldAccess &access) {
  auto accessor = access.getAccessor();
  auto bb = accessor->getParent();

  if (access.isRead()) {
    emitRead(access);
  } else if  (access.isWrite()) {
    emitWrite(access);
  } else {
    llvm_unreachable("What is it?");
  }

  // Remove old access
  auto call = access.getFieldCall();
  if (call != accessor)
    call->removeFromParent();
  accessor->removeFromParent();
}


bool FieldCodeGen::runOnFunction(Function &func) {
  if (func.getName() == "main") {
    int a = 0;
  }

  MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
  fields = &getAnalysis<FieldDetectionAnalysis>();


  changed = false;
  SmallVector<Instruction*, 16> instrs;
  collectInstructionList(&func, instrs);

  for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
    auto instr = *it;

    auto access = fields->getFieldAccess(instr);
    if (!access.isValid())
      continue;  // Not an access to a field

    emitAccess(access);

#if 0
    auto fieldTy = access.getFieldType();
    auto fieldVar = access.getFieldVariable();
    auto islocalFunc = fieldTy->getIsLocalFunc();

    auto accessor = access.getAccessor();
    auto ref = access.getFieldCall();
    auto subscript = access.getSubscript();
    auto islocalfunc = fieldTy->getIsLocalFunc();

    if (access.isRead()) {

    } else {
      assert(access.isWrite());
      auto bbBefore = instr->getParent();
      auto bbConditional = SplitBlock(bbBefore, instr, this);
      auto bbAfter = SplitBlock(bbConditional, instr->getNextNode(), this); //TODO: What if there is no next node

      bbBefore->getTerminator()->eraseFromParent();
      IRBuilder<> builder(bbBefore);
      auto isLocalResult = builder.CreateCall2(islocalfunc, fieldVar->getVariable(), subscript);
      builder.CreateCondBr(isLocalResult, bbConditional, bbAfter);
      changed = true;
    }
#endif
  }

  bool funcEmitted = emitFieldFunctions();

  return changed;
}


bool FieldCodeGen::emitFieldFunctions() {
  auto &fields = getAnalysis<FieldDetectionAnalysis>();
  auto &fieldTypes = fields.getFieldTypes();
  bool changed = false;

  for (auto it = fieldTypes.begin(), end = fieldTypes.end(); it!=end; ++it) {
    auto fieldTy = it->second;

#if 0
    auto *isLocalFunc = fieldTy->getIsLocalFunc();
    if (isLocalFunc->empty()) {
      fieldTy->emitIsLocalFunc();
      return true;
    }
#endif
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
