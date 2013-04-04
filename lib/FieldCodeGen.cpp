#include "molly/FieldCodeGen.h"

#include <polly/ScopPass.h>
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
#include <llvm/IR/Intrinsics.h>
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



  class ModuleFieldGen : public ModulePass {
  private:
    bool changed;
  public:
    static char ID;
    ModuleFieldGen() : ModulePass(ID) {
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequiredTransitive<MollyContextPass>();
      AU.addRequiredTransitive <FieldDetectionAnalysis>();
    }



    Function *emitLocalLength(Module &M, FieldType *ftype) {
      auto &context = M.getContext();
      auto llvmTy = ftype->getType();
      auto nDims = ftype->getNumDimensions();
      auto intTy = Type::getInt32Ty(context);
      auto lengths = ftype->getLengths();

      SmallVector<Type*, 5> argTys;
      argTys.push_back( PointerType::getUnqual(llvmTy) );
      argTys.append(nDims, Type::getInt32Ty(context));
      auto localLengthFuncTy = FunctionType::get(Type::getInt32Ty(context), argTys, false);

      auto localLengthFunc = Function::Create(localLengthFuncTy, GlobalValue::InternalLinkage, "molly_localoffset", &M);
      auto dimArg = &localLengthFunc->getArgumentList().front();

      auto entryBB = BasicBlock::Create(context, "Entry", localLengthFunc); 
      auto defaultBB = BasicBlock::Create(context, "Default", localLengthFunc); 
      new UnreachableInst(context, defaultBB);

      //ReturnInst::Create(context, NULL, entryBB);
      IRBuilder<> builder(entryBB);
      builder.SetInsertPoint(entryBB);

      auto sw = builder.CreateSwitch(dimArg, defaultBB, nDims);
      auto d = 0;
      for (auto itCase = sw->case_begin(), endCase = sw->case_end(); itCase!=endCase; ++itCase) {
        itCase.setValue(ConstantInt::get(Type::getInt32Ty(context), d));

        auto caseBB = BasicBlock::Create(context, "Case_dim" + Twine(d), localLengthFunc); 
        ReturnInst::Create(context, ConstantInt::get(intTy, lengths[d]) , caseBB); 

        itCase.setSuccessor(caseBB);

        d+=1;
      }

      changed = true;
      ftype->setLocalLengthFunc(localLengthFunc);
      return localLengthFunc;
    }

    void runOnFieldType(Module &M, FieldType *ftype) {
      emitLocalLength(M, ftype);
    }

    virtual bool runOnModule(Module &M) {
      changed = false;
      auto FD = &getAnalysis<FieldDetectionAnalysis>();

      auto &ftypes = FD->getFieldTypes();
      for (auto it = ftypes.begin(), end = ftypes.end(); it!=end; ++it) {
        auto ftype = it->second;
        runOnFieldType(M, ftype);
      }

      return changed;
    }
  }; // class ModuleFieldGen


    llvm::ModulePass *createModuleFieldGenPass() {
      return new ModuleFieldGen();
    }

  char ModuleFieldGen::ID = 0;
  static RegisterPass<ModuleFieldGen> ModuleFieldGenRegistration("molly-modulegen", "Molly - Code generation per module", false, false);



  class FieldCodeGen : public FunctionPass {
  private:
    bool changed;

    FieldDetectionAnalysis *fields;
    Function *func;

    Value *createAlloca(Type *ty, const Twine &name = Twine()) {
      auto entry = &func->getEntryBlock();
      auto insertPos = entry->getFirstNonPHIOrDbgOrLifetime();
      auto result = new AllocaInst(ty, name, insertPos);
      return result;
    }

    typedef IRBuilder<> BuilderTy;

    Value *createPtrTo(BuilderTy &builder,  Value *val, const Twine &name = Twine()) {
      auto valueSpace = createAlloca(val->getType(), name);
      builder.CreateStore(val, valueSpace);
      return valueSpace;
    }

    void emitLocalLengthCall(CallInst *callInst) {
      auto &context = callInst->getContext();
      auto bb = callInst->getParent();

      auto selfArg = callInst->getArgOperand(0);
      auto dimArg = callInst->getArgOperand(1);

      auto fty = fields->getFieldType(selfArg);
      auto locallengthFunc = fty->getLocalLengthFunc();
      assert(locallengthFunc);

      //TODO: Optimize for common case where dim arg is constant, or will LLVM do proper inlining itself?

      // BuilderTy builder(bb);
      //builder.SetInsertPoint(callInst);
      callInst->setCalledFunction(locallengthFunc);
      changed = true;
    }

  public:
    static char ID;
    FieldCodeGen() : FunctionPass(ID) {
    }

    virtual const char *getPassName() const {
      return "FieldCodeGen";
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MollyContextPass>();
      //AU.addRequiredID(FieldDistributionPassID); 
      AU.addRequired<FieldDetectionAnalysis>();
    }

    void emitRead(MollyFieldAccess &access);
    void emitWrite(MollyFieldAccess &access);
    void emitAccess(MollyFieldAccess &access);

    virtual bool runOnFunction(Function &func);

    //virtual void releaseMemory() { }

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
  auto writtenValue = accessor->getOperand(0);

  IRBuilder<> builder(bb);
  builder.SetInsertPoint(accessor->getNextNode()); // Behind the old store instr


  SmallVector<Value*,6> args;
  args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
  auto stackPtr = createPtrTo(builder, writtenValue);
  args.push_back(stackPtr);
  for (auto d = nDims-nDims; d < nDims; d+=1) {
    auto coord = access.getCoordinate(d);
    args.push_back(coord);
  }

  builder.CreateCall(access.getFieldType()->getFuncSetBroadcast(), args);
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
  accessor->eraseFromParent();
  if (call != accessor) { //FIXME: Comparison after delete accessor
    call->eraseFromParent();
  }
}


bool FieldCodeGen::runOnFunction(Function &F) {
  if (F.getName() == "main") {
    int a = 0;
  }

  MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
  this->fields = &getAnalysis<FieldDetectionAnalysis>();
  this->func = &F;

  changed = false;
  SmallVector<Instruction*, 16> instrs;
  collectInstructionList(&F, instrs);

  for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
    auto instr = *it;
    if (auto callInstr = dyn_cast<CallInst>(instr)) {
      auto calledFunc = callInstr->getCalledFunction();
      if (calledFunc && calledFunc->getIntrinsicID() == Intrinsic::molly_locallength) {
        emitLocalLengthCall(callInstr);
      }
    }

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
