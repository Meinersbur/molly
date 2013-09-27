#include "MollyFunctionProcessor.h"

#include <llvm/Pass.h>
#include "MollyPassManager.h"
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "LLVMfwd.h"
#include <llvm/IR/IRBuilder.h>
#include "MollyFieldAccess.h"
#include "FieldVariable.h"
#include <llvm/IR/GlobalVariable.h>
#include "FieldType.h"
#include "MollyUtils.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "polly/LinkAllPasses.h"
#include "ClusterConfig.h"
#include "clang/CodeGen/MollyRuntimeMetadata.h"
#include "Codegen.h"
#include "llvm/IR/Module.h"
#include "FieldLayout.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
//using isl::enwrap;


namespace {

  class MollyFunctionResolver : public AnalysisResolver {
  private:
    MollyPassManager *pm;
    Function *func;

  public:
    MollyFunctionResolver(MollyPassManager *pm, Function *func) 
      :  AnalysisResolver(*static_cast<PMDataManager*>(nullptr)), pm(pm), func(func) {}

    Pass * findImplPass(AnalysisID PI) LLVM_OVERRIDE {
      return pm->findOrRunAnalysis(PI, func, nullptr);
    }

    Pass * findImplPass(Pass *P, AnalysisID PI, Function &F) LLVM_OVERRIDE {
      assert(&F == func);
      return pm->findOrRunAnalysis(PI, &F, nullptr);
    }

    Pass * getAnalysisIfAvailable(AnalysisID ID, bool Direction) const LLVM_OVERRIDE {
      return pm->findAnalysis(ID, func, nullptr);
    }
  }; // class MollyFunctionResolver


  // This is actually not a pass
  // It used internally for actions on functions and remembering function-specific data
  // Could be changed to a pass in later versions once we find out how to pass data between passes, without unrelated passes in between destroying them
  class MollyFunctionContext : public MollyFunctionProcessor, private FunctionPass {
  private:
    MollyPassManager *pm;
    Function *func;

    void modifiedIR() {
      pm->modifiedIR();
    }

  public:
    static char ID;
    MollyFunctionContext(MollyPassManager *pm, Function *func): FunctionPass(ID), pm(pm), func(func) {
      assert(pm);
      assert(func);

      //FIXME: Shouldn't the resolver assigned by the pass manager?
      this->setResolver(createResolver(pm, func));
    }

    bool runOnFunction(Function &F) LLVM_OVERRIDE {
      llvm_unreachable("This is not a pass");
    }

#pragma region Access CodeGen
  protected:
    Value *createAlloca(Type *ty, const Twine &name = Twine()) {
      auto entry = &func->getEntryBlock();
      auto insertPos = entry->getFirstNonPHIOrDbgOrLifetime();
      auto result = new AllocaInst(ty, name, insertPos);
      return result;
    }


    Value *createPtrTo(DefaultIRBuilder &builder,  Value *val, const Twine &name = Twine()) {
      auto valueSpace = createAlloca(val->getType(), name);
      builder.CreateStore(val, valueSpace);
      return valueSpace;
    }


    void emitRead(MollyFieldAccess &access){
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

      auto call = builder.CreateCall(access.getFieldType()->getFuncGetBroadcast(), args);
      auto load = builder.CreateLoad(buf, "loadget");
      accessor->replaceAllUsesWith(load);
    }


    void emitWrite(MollyFieldAccess &access) {
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


    void emitAccess(MollyFieldAccess &access) {
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


    void emitLocalLengthCall(CallInst *callInst) {
      auto &context = callInst->getContext();
      auto bb = callInst->getParent();

      auto selfArg = callInst->getArgOperand(0);
      auto dimArg = callInst->getArgOperand(1);

      auto fty = pm->getFieldType(selfArg);
      auto locallengthFunc = fty->getLocalLengthFunc();
      assert(locallengthFunc);

      //TODO: Optimize for common case where dim arg is constant, or will LLVM do proper inlining itself?

      // BuilderTy builder(bb);
      //builder.SetInsertPoint(callInst);
      callInst->setCalledFunction(locallengthFunc);
      modifiedIR();
    }


    void emitIslocalCall(CallInst *callInstr) {
      auto &context = callInstr->getContext();
      auto selfArg = callInstr->getArgOperand(0);
      auto fty = pm->getFieldType(selfArg);

      // This is just a redirection to the implementation
      callInstr->setCalledFunction(fty->getIslocalFunc());

      modifiedIR();
    }



  private:
    isl::MultiAff currentNodeCoord; /* { -> node[cluster] } */
    std::map<isl_id *, llvm::Value *> idtovalue;

    isl::MultiAff getCurrentNodeCoordinate() {
      if (currentNodeCoord.isValid())
        return currentNodeCoord;

      auto clusterConf = pm->getClusterConfig();
      auto islctx = pm->getIslContext();
      auto clusterSpace = clusterConf->getClusterSpace();
      auto nDims = clusterSpace.getSetDimCount();
      auto currentNodeCoordSpace = isl::Space::createMapFromDomainAndRange(nDims, clusterSpace);


      for (auto i = nDims-nDims; i<nDims; i+=1) {
        auto value = getClusterCoordinate(i);
        auto id = clusterConf->getClusterDimId(i);
        currentNodeCoordSpace.setDimId_inplace(isl_dim_in, i, id);
        idtovalue[id.keep()] = value;
      }

      currentNodeCoord = currentNodeCoordSpace.createZeroMultiAff();
      for (auto i = nDims-nDims; i<nDims; i+=1) {
        currentNodeCoord.setAff_inplace(i, currentNodeCoord.getDomainSpace().createAffOnVar(i));
      }
      return currentNodeCoord;
    }


    Function *getRuntimeFunc(StringRef name, Type *retTy, ArrayRef<Type*> tys) {//TODO: Moved to MollyCodegen
      auto module = func->getParent();
      auto &llvmContext = func->getContext();

      auto initFunc = module->getFunction(name);
      if (!initFunc) {
        auto intTy = Type::getInt64Ty(llvmContext);
        auto initFuncTy = FunctionType::get(retTy, tys, false);
        initFunc = Function::Create(initFuncTy, GlobalValue::ExternalLinkage, name, module);
      }

      // Check if the function matches
      assert(initFunc->getReturnType() == retTy);
      assert(initFunc->getFunctionType()->getNumParams() == tys.size());
      auto nParams = tys.size();
      for (auto i = nParams-nParams; i<nParams; i+=1) {
        assert(initFunc->getFunctionType()->getParamType(i) == tys[i]);
      }
      return initFunc;
    }


    void replaceGlobalInit(CallInst *call, Function *called) {
      auto combufs = pm->getCommunicationBuffers();

      for (auto combuf : combufs) {

        assert(!"TODO");

      }

      call->eraseFromParent();
    }


    void replaceGlobalFree(CallInst *call, Function *called) {
      call->eraseFromParent();
    }



    void replaceClusterCurrentCoordinate(CallInst *call, Function *called) {
      MollyCodeGenerator codegen(call->getParent(), call, asPass());
      auto dArg = call->getOperand(0);
      auto result = codegen.callRuntimeClusterCurrentCoord(dArg);

      call->replaceAllUsesWith(result);
      call->eraseFromParent();
    }


    void replaceFieldInit(CallInst *call, Function *called) {
      auto &llvmContext = func->getContext();
      auto module = func->getParent();
      auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
      auto clusterconf = pm->getClusterConfig();
      auto clusterContext = clusterconf->getClusterParamShape();

      MollyCodeGenerator codegen(call->getParent(), call, clusterContext, idtovalue);
      auto &builder = codegen.getIRBuilder();

      auto fval = call->getArgOperand(0);
      auto fvar = pm->getFieldVariable(fval);
      assert(fvar && "Field variable not registered?!?");
      auto fty = fvar->getFieldType();

      auto initFunc = module->getFunction("__molly_local_init");
      if (!initFunc) {
        auto voidTy = Type::getVoidTy(llvmContext);

        auto intTy = Type::getInt64Ty(llvmContext);
        Type *tys[] = { voidPtrTy, intTy };
        auto initFuncTy = FunctionType::get(voidTy, tys, false);
        initFunc = Function::Create(initFuncTy, GlobalValue::ExternalLinkage, "__molly_local_init", module);
      }

      auto layout = fvar->getLayout(); //TODO: implement FieldLayout
      auto sizeVal = layout->codegenLocalSize(codegen, getCurrentNodeCoordinate()/* {  -> node[cluster] } */);

      auto fvalptr = builder.CreatePointerCast(fval, voidPtrTy);

      Value *args[] = { fvalptr, sizeVal };
      builder.CreateCall(initFunc, args);

      call->eraseFromParent();
    }


    void replaceFieldFree(CallInst *call, Function *called) {
      auto &llvmContext = func->getContext();
      auto module = func->getParent();
      auto voidPtrTy = Type::getInt8PtrTy(llvmContext);

      auto fval = call->getArgOperand(0);
      auto fvar = pm->getFieldVariable(fval);

      DefaultIRBuilder builder(call);

      auto freeFunc = module->getFunction("__molly_local_free");
      if (!freeFunc) {
        auto voidTy = Type::getVoidTy(llvmContext);

        auto intTy = Type::getInt64Ty(llvmContext);
        Type *tys[] = { voidPtrTy };
        auto freeFuncTy = FunctionType::get(voidTy, tys, false);
        freeFunc = Function::Create(freeFuncTy, GlobalValue::ExternalLinkage, "__molly_local_free", module);
      }

      auto fvalptr = builder.CreatePointerCast(fval, voidPtrTy);
      Value *args[] = { fvalptr };
      builder.CreateCall(freeFunc, args);

      call->eraseFromParent();
    }


    bool isMollyIntrinsics(unsigned intID) {
      return Intrinsic::molly_1d_islocal <= intID && intID <= Intrinsic::molly_set_local;
    }


  public:
    void replaceIntrinsics() LLVM_OVERRIDE {

      // Make copy of all potentially to-be-replaced instructions
      SmallVector<CallInst*, 16> instrs;
      for (auto it = func->begin(), end = func->end(); it!=end; ++it) {
        auto bb = &*it;
        for (auto itInstr = bb->begin(), endInstr = bb->end(); itInstr!=endInstr; ++itInstr) {
          auto instr = &*itInstr;
          if (!isa<CallInst>(instr))
            continue;
          auto callInstr = cast<CallInst>(instr);
          auto func = callInstr->getCalledFunction();
          if (!func)
            continue;
          if(!func->isIntrinsic())
            continue;
          if (!isMollyIntrinsics(func->getIntrinsicID()))
            continue;
          instrs.push_back(callInstr);
        }
      }


      for (auto instr : instrs) {
        auto called = instr->getCalledFunction();
        if (!called)
          continue;

        auto intID = called->getIntrinsicID();
        switch(intID) {
        case Intrinsic::molly_cluster_current_coordinate:
          replaceClusterCurrentCoordinate(instr, called);
          break;
        case Intrinsic::molly_global_init:
          replaceFieldInit(instr, called);
          break;
        case Intrinsic::molly_global_free:
          replaceFieldFree(instr, called);
          break;
        case Intrinsic::molly_field_init:
          replaceFieldInit(instr, called);
          break;
        case Intrinsic::molly_field_free:
          replaceFieldFree(instr, called);
          break;

        default:
          continue;
        }
      }
    }

    void replaceRemainaingIntrinsics() {
      if (func->getName() == "test") {
        int a = 0;
        DEBUG(llvm::dbgs() << "### before FieldCodeGen ########\n");
        DEBUG(llvm::dbgs() << *func);
        DEBUG(llvm::dbgs() << "################################\n");
      }

      //MollyContextPass &MollyContext = getAnalysis<MollyContextPass>();
      //this->fields = &getAnalysis<FieldDetectionAnalysis>();
      this->func = func;

      modifiedIR();
      SmallVector<Instruction*, 16> instrs;
      collectInstructionList(func, instrs);

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

        auto access = pm->getFieldAccess(instr);
        if (!access.isValid())
          continue;  // Not an access to a field

        emitAccess(access);
      }

      //bool funcEmitted = emitFieldFunctions();

      if (func->getName() == "main" || func->getName() == "__molly_orig_main") {
        DEBUG(llvm::dbgs() << "### after FieldCodeGen ########\n");
        DEBUG(llvm::dbgs() << *func);
        DEBUG(llvm::dbgs() << "###############################\n");
      }
    }

#pragma endregion


    Pass *findOrRunAnalysis(AnalysisID passID) {
      return pm->findOrRunAnalysis(passID, func, nullptr);
    }

    template<typename T>
    T *findOrRunAnalysis() {
      return pm->findOrRunAnalysis<T>(func, nullptr);
    }

    void removePass(Pass *pass) {
      pm->removePass(pass);
    }


    MollyFieldAccess getFieldAccess(Instruction* instr) {
      return pm->getFieldAccess(instr);
    }


#pragma region Isolate field accesses

    void isolateInBB(MollyFieldAccess &facc) {
      auto accessInstr = facc.getAccessor();
      auto callInstr = facc.getFieldCall();
      auto bb = accessInstr->getParent();

      //SmallVector<Instruction*, 8> isolate;

      //if (callInstr)
      //  isolate.push_back(accessInstr);



      // FIXME: This creates trivial BasicBlocks, only containing PHI instrunctions and/or the callInstr, and arithmetic instructions to compute the coordinate; Those should be moved into the isolated BB
      if (bb->getFirstNonPHI() != accessInstr) {
        bb = SplitBlock(bb, accessInstr, this);
      }
      auto isolatedBB = bb;

      auto followInstr = accessInstr->getNextNode();
      if (followInstr && !isa<BranchInst>(followInstr)) {
        bb = SplitBlock(bb, followInstr, this);
      }

      auto oldName = isolatedBB->getName();
      if (oldName.endswith(".split")) 
        oldName = oldName.drop_back(6);
      isolatedBB->setName(oldName + ".isolated");

      if (callInstr) {
        assert(callInstr->hasOneUse());
        callInstr->moveBefore(accessInstr);
      }

#if 0
      // Move operands into the isolated block
      // Required to detect the affine access relations
      SmallVector<Instruction*,8> worklist;
      worklist.push_back(accessInstr);

      while(!worklist.empty()) {
        auto instr = worklist.pop_back_val();    // instr is already in isolated block

        for (auto it = instr->op_begin(), end = instr->op_end(); it!=end; ++it) {
          assert( it->getUser() == instr);
          auto val = it->get();
          if (!isa<Instruction>(val))
            continue;  // No need to move a constant
          auto usedInstr = cast<Instruction>(val);

          if (isa<PHINode>(usedInstr)) 
            continue; // No need to move

          if (usedInstr->)

        }

      }
#endif

#ifndef NDEBUG
      auto RI = this->getAnalysisIfAvailable<RegionInfo>();
      if (RI) {
        RI->verifyAnalysis();
      }
#endif
    }

    void isolateFieldAccesses() {
      // "Normalize" existing BasicBlocks
      auto ib = findOrRunAnalysis(&polly::IndependentBlocksID);
      removePass(ib);

      //for (auto &b : *func) {
      // auto bb = &b;

      bool changed =false;
      SmallVector<Instruction*, 16> instrs;
      collectInstructionList(func, instrs);
      for (auto instr : instrs){
        auto facc = getFieldAccess(instr);
        if (!facc.isValid())
          continue;
        // Create a distinct BB for this access
        // polly::IndependentBlocks will then it independent from the other BBs and
        // polly::ScopInfo create a ScopStmt for it
        isolateInBB(facc);
        changed = true;
      }
      //}

      if (changed) {
        // Also make new BBs independent
        ib = findOrRunAnalysis(&polly::IndependentBlocksID);
      }


      //auto sci = findOrRunAnalysis<TempScopInfo>();
      //auto sd = findOrRunAnalysis<ScopDetection>();
      //for (auto region : *sd) {
      //}

      // Check consistency
#ifndef NDEBUG
      for (auto &b : *func) {
        auto bb = &b;

        for (auto &i : *bb){
          auto instr = &i;
          auto facc = getFieldAccess(instr);
          if (facc.isValid()) {
            //assert(isFieldAccessBasicBlock(bb));
            break;
          }
        }
      }
#endif
    }

#pragma endregion


  private:
    SmallVector<Value*, 4> currentClusterCoord;

    Value* getClusterCoordinate(unsigned i) LLVM_OVERRIDE {
      auto clusterConf = pm->getClusterConfig();

      currentClusterCoord.reserve(clusterConf->getClusterDims());
      while (i >= currentClusterCoord.size() )
        currentClusterCoord.push_back(nullptr);

      auto &curCoord = currentClusterCoord[i];
      if (curCoord) {
        assert(cast<CallInst>(curCoord)->getCalledFunction()->getIntrinsicID() == Intrinsic::molly_cluster_current_coordinate);
        return curCoord;
      }

      auto bb = &func->getEntryBlock();
      MollyCodeGenerator codegen(bb, bb->getFirstInsertionPt(), asPass());
      curCoord = codegen.callClusterCurrentCoord(i);
      return curCoord;
    }


    std::vector<Value*> getClusterCoordinates() {
      auto clusterConf = pm->getClusterConfig();

      vector<Value *>result;
      auto nClusterDims = clusterConf->getClusterDims();
      result.reserve(nClusterDims);

      for (auto i = nClusterDims-nClusterDims; i < nClusterDims;i+=1) {
        result.push_back(getClusterCoordinate(i));
      } 

      return result;
    }

#pragma region molly::MollyFunctionProcessor
    llvm::FunctionPass *asPass() LLVM_OVERRIDE {
      return this;
    }
#pragma endregion

  }; // class MollyFunctionContext
} // namespace


char MollyFunctionContext::ID = '\0';
MollyFunctionProcessor *MollyFunctionProcessor::create(MollyPassManager *pm, Function *func) {
  return new MollyFunctionContext(pm, func);
}
llvm::AnalysisResolver *MollyFunctionProcessor::createResolver(MollyPassManager *pm, llvm::Function *func) {
  return new MollyFunctionResolver(pm, func);
}
