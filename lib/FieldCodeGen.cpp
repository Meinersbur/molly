#define DEBUG_TYPE "molly"
#include "FieldCodeGen.h"

#include <polly/ScopPass.h>
#include "FieldDistribution.h"
#include "FieldDetection.h"
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
#include <llvm/Support/Debug.h>

using namespace llvm;
using namespace molly;

using polly::ScopPass;
using polly::Scop;


namespace molly {
  class ModuleFieldGen : public ModulePass {
  private:
    bool changed;
  public:
    static char ID;
    ModuleFieldGen() : ModulePass(ID) {
    }


    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MollyContextPass>();
      AU.addRequired<FieldDetectionAnalysis>();

      AU.addPreserved<MollyContextPass>();
      AU.addPreserved<FieldDetectionAnalysis>();
      //AU.addPreserved<Fie
    }

#if 0
    // Not possible because we cannot determine the element type
    Function *emitPtr(Module &M, FieldType *ftype) {
    // Resolve to nothing; loads and stores should have been resolved
    auto &context = M.getContext();
    auto llvmTy = ftype->getType();
    auto nDims = ftype->getNumDimensions();
    auto intTy = Type::getInt32Ty(context);
    auto rtnTy = ftype->getEltPtrType();

    // Built function type
    SmallVector<Type*, 5> argTys;
    argTys.push_back(PointerType::getUnqual(llvmTy));
    for (auto i = nDims - nDims; i < nDims; i += 1) {
      argTys.push_back(intTy);
    }
    auto ptrFuncTy = FunctionType::get(rtnTy, argTys, false);

    // Create function
    auto ptrFunc = Function::Create(ptrFuncTy, GlobalValue::InternalLinkage, "molly_ptr", &M);
    auto entryBB = BasicBlock::Create(context, "Entry", ptrFunc);

    // Build body
    IRBuilder<> builder(entryBB);
    builder.SetInsertPoint(entryBB);
    builder.CreateRet(llvm::UndefValue::get(rtnTy));

    changed = true;
    return ptrFunc;
  }
#endif

    Function *emitLocalLength(Module &M, FieldType *ftype) {
      auto &context = M.getContext();
      auto llvmTy = ftype->getType();
      auto nDims = ftype->getNumDimensions();
      auto intTy = Type::getInt32Ty(context);
      auto lengths = ftype->getLengths();
      auto molly = &getAnalysis<MollyContextPass>();

      SmallVector<Type*, 5> argTys;
      argTys.push_back(PointerType::getUnqual(llvmTy));
      argTys.append(nDims, intTy);
      auto localLengthFuncTy = FunctionType::get(intTy, argTys, false);

      auto localLengthFunc = Function::Create(localLengthFuncTy, GlobalValue::InternalLinkage, "molly_locallength", &M);
      auto it = localLengthFunc->getArgumentList().begin();
      auto fieldArg = &*it;
      ++it;
      auto dimArg = &*it;
      ++it;
      assert(localLengthFunc->getArgumentList().end() == it);

      auto entryBB = BasicBlock::Create(context, "Entry", localLengthFunc); 
      auto defaultBB = BasicBlock::Create(context, "Default", localLengthFunc); 
      new UnreachableInst(context, defaultBB);

      IRBuilder<> builder(entryBB);
      builder.SetInsertPoint(entryBB);

      DEBUG(llvm::dbgs() << nDims << " Cases\n");
      auto sw = builder.CreateSwitch(dimArg, defaultBB, nDims);
      for (auto d = nDims-nDims; d<nDims; d+=1) {
        DEBUG(llvm::dbgs() << "Case " << d << "\n");
        auto caseBB = BasicBlock::Create(context, "Case_dim" + Twine(d), localLengthFunc);
        ReturnInst::Create(context, ConstantInt::get(intTy, lengths[d] / molly->getClusterLength(d)/*FIXME: wrong if nondivisible*/), caseBB);

        sw->addCase(ConstantInt::get(intTy, d), caseBB);
      }

      changed = true;
      ftype->setLocalLengthFunc(localLengthFunc);
      return localLengthFunc;
    }


    void runOnFieldType(Module &M, FieldType *ftype) {
      emitLocalLength(M, ftype);
      //emitPtr(M, ftype);
    }

    llvm::Module *module;


    void wrapMain() {
      auto &context = module->getContext();

      // Search main function
      auto origMainFunc = module->getFunction("main");
      if (!origMainFunc) {
        //FIXME: This means that either we are compiling modules independently (instead of whole program as intended), or this program as already been optimized 
        // The driver should resolve this
        return;
        llvm_unreachable("No main function found");
      }

      // Rename old main function
      const char *replMainName = "__molly_orig_main";
      auto replMainFunc = module->getFunction(replMainName);
      if (replMainFunc) {
        llvm_unreachable("main already replaced?");
      }
      
      origMainFunc->setName(replMainName);

      // Find the wrapper function from MollyRT
      auto rtMain = module->getOrInsertFunction("__molly_main", Type::getInt32Ty(context), Type::getInt32Ty(context)/*argc*/, PointerType::get(Type::getInt8PtrTy(context), 0)/*argv*/, NULL);
      assert(rtMain);

      // Create new main function
      Type *parmTys[] = {Type::getInt32Ty(context)/*argc*/, PointerType::get(Type::getInt8PtrTy(context), 0)/*argv*/ };
      auto mainFuncTy = FunctionType::get(Type::getInt32Ty(context), parmTys, false);
      auto wrapFunc = Function::Create(mainFuncTy, GlobalValue::ExternalLinkage, "main", module);

      auto entry = BasicBlock::Create(context, "entry", wrapFunc);
      IRBuilder<> builder(entry);

      // Create a call to the wrapper main function
      SmallVector<Value *, 2> args;
      collect(args, wrapFunc->getArgumentList());
      //args.append(wrapFunc->arg_begin(), wrapFunc->arg_end());
      auto ret = builder.CreateCall(rtMain, args, "call_to_rtMain");
      DEBUG(llvm::dbgs() << ">>>Wrapped main\n");
      changed = true;
      builder.CreateRet(ret);
    }


    void emitMollyGlobalVars() {
      auto &context = module->getContext();
      auto intTy = Type::getInt32Ty(context);
      auto intPtrTy = Type::getInt32PtrTy(context);
      auto molly = &getAnalysis<MollyContextPass>();

      new GlobalVariable(*module, intTy, true, GlobalValue::ExternalLinkage, ConstantInt::get(intTy, molly->getClusterDims()), "__molly_cluster_dims");
      new GlobalVariable(*module, intTy, true, GlobalValue::ExternalLinkage, ConstantInt::get(intTy, molly->getClusterSize()), "__molly_cluster_size");

      auto &lengths = molly->getClusterLengths();
      auto nDims = lengths.size();

      SmallVector<Constant*, 4> lengthConsts;
      SmallVector<Constant*, 4> periodicConsts;
      for (auto d=nDims-nDims; d<nDims; d+=1) {
        lengthConsts.push_back(ConstantInt::get(intTy, lengths[d]));
        periodicConsts.push_back(ConstantInt::get(intTy, true));//FIXME: Determine whether periodicity is really needed
      }
      lengthConsts.push_back(ConstantInt::get(intTy, 0));
      periodicConsts.push_back(ConstantInt::get(intTy, -1));
      auto clusterLengthsConstsTy = ArrayType::get(intTy, nDims+1);
      auto glengths = ConstantArray::get(clusterLengthsConstsTy, lengthConsts);
      auto clusterLengthsGlobArr = new GlobalVariable(*module, clusterLengthsConstsTy, true, GlobalValue::InternalLinkage, glengths);
      new GlobalVariable(*module, PointerType::getUnqual(clusterLengthsConstsTy), true, GlobalValue::ExternalLinkage, clusterLengthsGlobArr, "__molly_cluster_lengths");

      auto gperiodic = ConstantArray::get(clusterLengthsConstsTy, periodicConsts);
      auto clustePeriodicGlobArr = new GlobalVariable(*module, clusterLengthsConstsTy, true, GlobalValue::InternalLinkage, gperiodic);
      new GlobalVariable(*module, PointerType::getUnqual(clusterLengthsConstsTy), true, GlobalValue::ExternalLinkage, clustePeriodicGlobArr, "__molly_cluster_periodic");

      DEBUG(llvm::dbgs() << "Emitted __molly_cluster_dims\n");
    }


    virtual bool runOnModule(Module &M) {
      changed = false;
      this->module = &M;
      auto FD = &getAnalysis<FieldDetectionAnalysis>();


      auto &ftypes = FD->getFieldTypes();
      for (auto it = ftypes.begin(), end = ftypes.end(); it!=end; ++it) {
        auto ftype = it->second;
        runOnFieldType(M, ftype);
      }

      wrapMain();
      emitMollyGlobalVars();

      module = NULL;
      return changed;
    }
  }; // class ModuleFieldGen
  } // namespace molly


  llvm::ModulePass *molly::createModuleFieldGenPass() {
    return new ModuleFieldGen();
  }

  char ModuleFieldGen::ID = 0;
  const char &molly::ModuleFieldGenPassID = ModuleFieldGen::ID;
  static RegisterPass<ModuleFieldGen> ModuleFieldGenRegistration("molly-modulegen", "Molly - ModuleFieldGen", false, false);


  namespace molly {
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

    void emitIslocalCall(CallInst *callInstr) {
      auto &context = callInstr->getContext();
      auto selfArg = callInstr->getArgOperand(0);
      auto fty = fields->getFieldType(selfArg);

      // This is just a redirection to the implentation
      callInstr->setCalledFunction(fty->getIslocalFunc());

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

      AU.addPreserved<MollyContextPass>();
      AU.addPreserved<FieldDetectionAnalysis>();
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

  auto call =builder.CreateCall(access.getFieldType()->getFuncGetBroadcast(), args);
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
  } else if (access.isWrite()) {
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
  if (F.getName() == "test") {
    int a = 0;
    DEBUG(llvm::dbgs() << "### before FieldCodeGen ########\n");
    DEBUG(llvm::dbgs() << F);
    DEBUG(llvm::dbgs() << "################################\n");
  }

  MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
  this->fields = &getAnalysis<FieldDetectionAnalysis>();
  this->func = &F;

  changed = false;
  SmallVector<Instruction*, 16> instrs;
  collectInstructionList(&F, instrs);

  // Replace intrinsics
  for (auto it = instrs.begin(), end = instrs.end(); it!=end; ++it) {
    auto instr = *it;
    if (auto callInstr = dyn_cast<CallInst>(instr)) {
      auto calledFunc = callInstr->getCalledFunction();
      if (calledFunc) {
        switch (calledFunc->getIntrinsicID()) {
        case Intrinsic::molly_locallength:
          emitLocalLengthCall(callInstr);
          break;
        case Intrinsic::molly_islocal:
          emitIslocalCall(callInstr);
          break;
        default:
          break;
        }
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

  //bool funcEmitted = emitFieldFunctions();

  if (F.getName() == "main" || F.getName() == "__molly_orig_main") {
    DEBUG(llvm::dbgs() << "### after FieldCodeGen ########\n");
    DEBUG(llvm::dbgs() << F);
    DEBUG(llvm::dbgs() << "###############################\n");
  }

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
const char &molly::FieldCodeGenPassID = FieldCodeGen::ID;
static RegisterPass<FieldCodeGen> FieldCodeGenRegistration("molly-fieldcodegen", "Molly - Code generation", false, false);


FunctionPass *molly::createFieldCodeGenPass() {
  return new FieldCodeGen();
}
