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
#include "RectangularMapping.h"

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
      : AnalysisResolver(*static_cast<PMDataManager*>(nullptr)), pm(pm), func(func) {}

    Pass * findImplPass(AnalysisID PI) override {
      return pm->findOrRunAnalysis(PI, func, nullptr);
    }

    Pass * findImplPass(Pass *P, AnalysisID PI, Function &F) override {
      assert(&F == func);
      return pm->findOrRunAnalysis(PI, &F, nullptr);
    }

    Pass * getAnalysisIfAvailable(AnalysisID ID, bool Direction) const override {
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
    MollyFunctionContext(MollyPassManager *pm, Function *func) : FunctionPass(ID), pm(pm), func(func) {
      assert(pm);
      assert(func);

      //FIXME: Shouldn't the resolver assigned by the pass manager?
      this->setResolver(createResolver(pm, func));
    }

    bool runOnFunction(Function &F) override {
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


    Value *createPtrTo(DefaultIRBuilder &builder, Value *val, const Twine &name = Twine()) {
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

      SmallVector<Value*, 6> args;
      args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
      args.push_back(buf);
      for (auto d = nDims - nDims; d < nDims; d += 1) {
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


      SmallVector<Value*, 6> args;
      args.push_back(access.getFieldVariable()->getVariable()); // "this" implicit argument
      auto stackPtr = createPtrTo(builder, writtenValue);
      args.push_back(stackPtr);
      for (auto d = nDims - nDims; d < nDims; d += 1) {
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
      }
      else if (access.isWrite()) {
        emitWrite(access);
      }
      else {
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
    isl::MultiAff currentNodeCoord; /* { [] -> node[cluster] } */
    std::map<isl_id *, llvm::Value *> idtovalue;

    isl::MultiAff getCurrentNodeCoordinate() override {
      if (currentNodeCoord.isValid())
        return currentNodeCoord;

      auto clusterConf = pm->getClusterConfig();
      auto islctx = pm->getIslContext();
      auto clusterSpace = clusterConf->getClusterSpace();
      auto nDims = clusterSpace.getSetDimCount();

      auto paramSpace = clusterSpace.moveDims(isl_dim_param, 0, isl_dim_set, 0, nDims);
      auto currentNodeCoordSpace = isl::Space::createMapFromDomainAndRange(0, clusterSpace).alignParams(paramSpace);
      //auto currentNodeCoordSpace = clusterSpace.alignParams(paramSpace);

      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto value = getClusterCoordinate(i);
        auto id = clusterConf->getClusterDimId(i);
        //currentNodeCoordSpace.setDimId_inplace(isl_dim_param, i, id);
        idtovalue[id.keep()] = value;
      }

      currentNodeCoord = currentNodeCoordSpace.createZeroMultiAff();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto id = clusterConf->getClusterDimId(i);
        currentNodeCoord.setAff_inplace(i, currentNodeCoordSpace.getDomainSpace().createAffOnParam(id));
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
      for (auto i = nParams - nParams; i < nParams; i += 1) {
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
      assert(called->getIntrinsicID() == Intrinsic::molly_cluster_current_coordinate);
      assert(call->getNumArgOperands() == 1);
      auto dArg = call->getOperand(0);

      auto codegen = makeCodegen(call);
      auto result = codegen.callRuntimeClusterCurrentCoord(dArg);

      // We replaced the intrinsics by the runtime function, let that code generator know is
      // Otherwise, it would continue the freed value
      // Alternatively, we could clear this cache
      for (auto &currentnodefunc : currentClusterCoord) {
        if (currentnodefunc == call)
          currentnodefunc = result;
      }
      for (auto &vtoid : idtovalue) {
        if (vtoid.second == call)
          vtoid.second = result;
      }

      call->replaceAllUsesWith(result);
      call->eraseFromParent();
    }


    void replaceFieldInit(CallInst *call, Function *called) {
      auto &llvmContext = func->getContext();
      auto module = func->getParent();
      auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
      auto clusterconf = pm->getClusterConfig();
      auto clusterContext = clusterconf->getClusterParamShape();

      auto codegen = makeCodegen(call);
      auto &builder = codegen.getIRBuilder();

      auto fval = call->getArgOperand(0);
      auto fvar = pm->getFieldVariable(fval);
      //if (fvar) {
      //auto fty = fvar->getFieldType();
      //auto fty = pm->getFieldType(cast<StructType>(fval->getType()->getPointerElementType()));

      auto layout = fvar->getLayout();
      auto sizeVal = layout->codegenLocalSize(codegen, getCurrentNodeCoordinate()/* {  -> node[cluster] } */);

      auto rankFunc = pm->emitFieldRankofFunc(layout);
      auto indexFunc = pm->emitLocalIndexofFunc(layout);

      codegen.callRuntimeLocalInit(fval, sizeVal, rankFunc, indexFunc);
      //} else {
      // Constructor should have been inlined; let's assume this is the non-inlined version that will never be called since we have no way to find out to which 
      //}

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


    MollyCodeGenerator makeCodegen(Instruction *insertBefore) override {
      // ensure idtovalue is up to date
      getCurrentNodeCoordinate();

      MollyCodeGenerator codegen(insertBefore->getParent(), insertBefore, asPass(), pm->getClusterConfig()->getClusterParamShape(), idtovalue);
      return codegen; // NRVO
    }


    void replaceCombufSendPtr(CallInst *call, Function *called) {
      auto combuf = call->getOperand(0);
      auto dst = call->getOperand(1);

      auto codegen = makeCodegen(call);
      auto ptr = codegen.callRuntimeCombufSendPtr(combuf, dst);
      auto casted = codegen.getIRBuilder().CreatePointerCast(ptr, call->getType());
      call->replaceAllUsesWith(casted);
      call->eraseFromParent();
    }


    void replaceCombufRecvPtr(CallInst *call, Function *called) {
      auto combuf = call->getOperand(0);
      auto src = call->getOperand(1);

      auto codegen = makeCodegen(call);
      auto ptr = codegen.callRuntimeCombufRecvPtr(combuf, src);
      auto casted = codegen.getIRBuilder().CreatePointerCast(ptr, call->getType());
      call->replaceAllUsesWith(casted);
      call->eraseFromParent();
    }


    void replaceCombufSend(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_combuf_send);
      assert(call->getNumArgOperands() == 2);
      auto combuf = call->getOperand(0);
      auto dst = call->getOperand(1);

      auto codegen = makeCodegen(call);
      codegen.callRuntimeCombufSend(combuf, dst);

      call->eraseFromParent();
    }


    void replaceCombufSendWait(CallInst *call) {
      auto called = call->getCalledFunction();
      assert(called->getIntrinsicID() == Intrinsic::molly_combuf_send_wait);
      assert(call->getNumArgOperands() == 2);
      auto sendbuf = call->getOperand(0);
      auto dst = call->getOperand(1);

      auto codegen = makeCodegen(call);
      auto replacement = codegen.callRuntimeCombufSendWait(sendbuf, dst);

      auto replacementCasted = codegen.getIRBuilder().CreatePointerCast(replacement, call->getType());
      call->replaceAllUsesWith(replacementCasted);
      call->eraseFromParent();
    }


    void replaceCombufRecv(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_combuf_recv);
      assert(call->getNumArgOperands() == 2);
      auto combuf = call->getOperand(0);
      auto src = call->getOperand(1);

      auto codegen = makeCodegen(call);
      codegen.callRuntimeCombufRecv(combuf, src);

      call->eraseFromParent();
    }


    void replaceCombufRecvWait(CallInst *call) {
      auto called = call->getCalledFunction();
      assert(called->getIntrinsicID() == Intrinsic::molly_combuf_recv_wait);
      assert(call->getNumArgOperands() == 2);
      auto recvbuf = call->getOperand(0);
      auto src = call->getOperand(1);

      auto codegen = makeCodegen(call);
      auto replacement = codegen.callRuntimeCombufRecvWait(recvbuf, src);

      auto replacementCasted = codegen.getIRBuilder().CreatePointerCast(replacement, call->getType());
      call->replaceAllUsesWith(replacementCasted);
      call->eraseFromParent();
    }


    void replaceLocalPtr(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_local_ptr);
      assert(call->getNumArgOperands() == 1);
      auto localobj = call->getOperand(0);

      auto codegen = makeCodegen(call);
      auto ptr = codegen.callRuntimeLocalPtr(localobj);
      auto casted = codegen.getIRBuilder().CreatePointerCast(ptr, call->getType(), ptr->getName() + Twine("_casted"));

      call->replaceAllUsesWith(casted);
      call->eraseFromParent();
    }


    void replaceValueAccess(llvm::Instruction *accInstr) {
      auto acc = FieldAccess::fromAccessInstruction(accInstr);
      assert(acc.isValid());
      auto codegen = makeCodegen(accInstr);

      auto coords = acc.getCoordinates();
      auto field = acc.getBaseField();
      auto fvar = pm->getFieldVariable(field);

      auto rank = codegen.callFieldRankof(fvar, coords);
      auto idx = codegen.callLocalIndexof(fvar, coords);

      if (acc.isRead()) {
        auto loadInst = acc.getLoadInst();
        auto mollyLoad = codegen.codegenValueLoad(fvar, rank, idx);
        loadInst->replaceAllUsesWith(mollyLoad);
        loadInst->eraseFromParent();
      }
      else if (acc.isWrite()) {
        auto storeInst = acc.getStoreInst();
        codegen.codegenValueStore(fvar, storeInst->getValueOperand(), rank, idx);
        storeInst->eraseFromParent();
      }
      else
        llvm_unreachable("Strange access");
    }


    void replacePtr(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_ptr);
      assert(call->getNumArgOperands() >= 2);
      //auto fieldobj = call->getOperand(0);
      //auto nCoords = call->getNumArgOperands() -1;

      //for (auto i=nCoords-nCoords; i<nCoords; i+=1) {
      //  auto coord = call->getOperand(i+1);
      //}

      SmallVector<Instruction *, 4> uses;
      for (auto useIt = call->use_begin(), end = call->use_end(); useIt != end; ++useIt) {
        auto opno = useIt.getOperandNo();
        auto use = &useIt.getUse();
        auto user = cast<Instruction>(*useIt);

        // Make copy because replaceValueAccess() will erease the instr from the list
        uses.push_back(user);
      }

      for (auto use : uses) {
        replaceValueAccess(use);
      }
      call->eraseFromParent();
    }

    isl::Ctx *getIslContext() {
      return pm->getIslContext();
    }




    void replaceFieldRankof(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_field_rankof);
      assert(call->getNumArgOperands() >= 2);
      auto fieldobj = call->getOperand(0);
      SmallVector<Value*, 4> coords;
      auto nDims = call->getNumArgOperands() - 1;
      coords.reserve(nDims);
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        coords.push_back(call->getOperand(i + 1));
      }
      auto clusterConf = pm->getClusterConfig();

      auto fvar = pm->getFieldVariable(fieldobj);
      auto fty = fvar->getFieldType();
      auto layout = fvar->getLayout();
      auto dist = layout->getHomeAff(); // { field[indexset] -> node[cluster] } 

      auto islctx = getIslContext();
      auto codegen = makeCodegen(call);
      auto paramsSpace = islctx->createParamsSpace(nDims);
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        auto coord = call->getOperand(i + 1);
        auto id = islctx->createId("idx" + Twine(i), coord);
        paramsSpace.setDimId_inplace(isl_dim_param, i, id);
        codegen.addParam(id, coord);
      }

      auto coordsAffSpace = paramsSpace.createMapSpace(0, nDims); // { [] -> field[indexset] }
      auto coordsAff = coordsAffSpace.createZeroMultiAff();
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        coordsAff.setAff_inplace(i, coordsAff.getDomainSpace().createVarAff(isl_dim_param, i));
      }
      coordsAffSpace = coordsAffSpace.getDomainSpace().mapsTo(fty->getIndexsetSpace());
      coordsAff.cast_inplace(coordsAffSpace);

      auto homeAff = dist.pullback(coordsAff); // { [] -> node[cluster] }


      auto clusterLengths = clusterConf->getClusterLengthsAff();
      RectangularMapping mapping(clusterLengths, clusterLengths.getSpace().createZeroMultiAff());
      auto trans = homeAff.getDomainSpace().mapsToItself().createIdentityMultiAff();
      auto rank = mapping.codegenIndex(codegen, trans, homeAff);

      call->replaceAllUsesWith(rank);
      call->eraseFromParent();
    }


    void replaceLocalIndexof(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_local_indexof);
      assert(call->getNumArgOperands() >= 2);
      auto fieldobj = call->getOperand(0);

      SmallVector<Value*, 4> coords;
      auto nDims = call->getNumArgOperands() - 1;
      coords.reserve(nDims);
      for (auto i = nDims - nDims; i < nDims; i += 1) {
        coords.push_back(call->getOperand(i + 1));
      }

      auto fvar = pm->getFieldVariable(fieldobj);
      auto codegen = makeCodegen(call);

      Value *idx;
      if (fvar) {
        // Field known at runtime; inline the computation
        auto clusterConf = pm->getClusterConfig();

        auto fty = fvar->getFieldType();
        auto layout = fvar->getLayout();

        auto islctx = getIslContext();

        auto paramsSpace = islctx->createParamsSpace(nDims);
        for (auto i = nDims - nDims; i < nDims; i += 1) {
          auto coord = call->getOperand(i + 1);
          auto id = islctx->createId("idx" + Twine(i), coord);
          paramsSpace.setDimId_inplace(isl_dim_param, i, id);
          codegen.addParam(id, coord);
        }

        auto coordsAffSpace = paramsSpace.createMapSpace(0, nDims); // { [] -> field[indexset] }
        auto coordsAff = coordsAffSpace.createZeroMultiAff();
        for (auto i = nDims - nDims; i < nDims; i += 1) {
          coordsAff.setAff_inplace(i, coordsAff.getDomainSpace().createVarAff(isl_dim_param, i));
        }
        coordsAffSpace = coordsAffSpace.getDomainSpace().mapsTo(fty->getIndexsetSpace());
        coordsAff.cast_inplace(coordsAffSpace);

        auto curnode = getCurrentNodeCoordinate(); // { [] -> node[cluster] }
        // auto trans = coordsAffSpace.getDomainSpace().mapsToItself().createIdentityMultiAff();
        idx = layout->codegenLocalIndex(codegen, curnode, coordsAff);
      }
      else {
        // Field not known at runtime; call the virtual method
        codegen.callRuntimeLocalIndexof(fieldobj, coords);
      }
      call->replaceAllUsesWith(idx);
      call->eraseFromParent();
    }


    void replaceValueLoad(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_value_load);
      assert(call->getNumArgOperands() == 4);

      auto fvarval = call->getOperand(0);
      auto valptr = call->getOperand(1);
      auto rank = call->getOperand(2);
      auto localidx = call->getOperand(3);

      auto fvar = pm->getFieldVariable(fvarval);

      auto codegen = makeCodegen(call);
      codegen.callRuntimeValueLoad(fvar, valptr, rank, localidx);

      call->eraseFromParent();
    }


    void replaceValueStore(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_value_store);
      assert(call->getNumArgOperands() == 4);

      auto fvarval = call->getOperand(0);
      auto valptr = call->getOperand(1);
      auto rank = call->getOperand(2);
      auto localidx = call->getOperand(3);

      auto fvar = pm->getFieldVariable(fvarval);

      auto codegen = makeCodegen(call);
      codegen.callRuntimeValueStore(fvar, valptr, rank, localidx);

      call->eraseFromParent();
    }

#if 0
    void replaceClusterMyrank(CallInst *call, Function *called) {
      assert(called->getIntrinsicID() == Intrinsic::molly_cluster_myrank);
      assert(call->getNumArgOperands() == 0);

      auto ClusterConf = pm->getClusterConfig();

      auto codegen = makeCodegen(call);


      auto clusterLengths = clusterConf->getClusterLengthsAff();
      RectangularMapping mapping(clusterLengths, clusterLengths.getSpace().createZeroMultiAff());
      auto trans = homeAff.getDomainSpace().mapsToItself().createIdentityMultiAff();
      auto rank = mapping.codegenIndex(codegen, trans, homeAff);


      call->replaceAllUsesWith(casted);
      call->eraseFromParent();
    }
#endif

    bool isMollyIntrinsics(unsigned intID) {
      return Intrinsic::molly_1d_islocal <= intID && intID <= Intrinsic::molly_value_store;
    }


  public:

    //TODO: This can easily refactored into its own BasicBlockPass
    void replaceIntrinsics() override {
      assert(isMollyIntrinsics(Intrinsic::molly_cluster_current_coordinate));
      assert(isMollyIntrinsics(Intrinsic::molly_global_init));
      assert(isMollyIntrinsics(Intrinsic::molly_global_free));
      assert(isMollyIntrinsics(Intrinsic::molly_field_init));
      assert(isMollyIntrinsics(Intrinsic::molly_field_free));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_send_ptr));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_recv_ptr));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_send));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_send_wait));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_recv));
      assert(isMollyIntrinsics(Intrinsic::molly_combuf_recv_wait));
      assert(isMollyIntrinsics(Intrinsic::molly_local_ptr));
      assert(isMollyIntrinsics(Intrinsic::molly_value_store));
      assert(isMollyIntrinsics(Intrinsic::molly_value_load));
      assert(isMollyIntrinsics(Intrinsic::molly_cluster_myrank));
      assert(isMollyIntrinsics(Intrinsic::molly_mod));

      lowerMollyIntrinsics();

      // Second call because some replacements (molly_field_init, field_rankof) may introduce new intrinsics that have to be replaced
      lowerMollyIntrinsics();
    }


    void lowerMollyIntrinsics() {
      // Make copy of all potentially to-be-replaced instructions
      SmallVector<CallInst*, 16> replaceWorklist;
      for (auto it = func->begin(), end = func->end(); it != end; ++it) {
        auto bb = &*it;
        for (auto itInstr = bb->begin(), endInstr = bb->end(); itInstr != endInstr; ++itInstr) {
          auto instr = &*itInstr;
          if (!isa<CallInst>(instr))
            continue;
          auto callInstr = cast<CallInst>(instr);
          auto func = callInstr->getCalledFunction();
          if (!func)
            continue;
          if (!func->isIntrinsic())
            continue;
          if (!isMollyIntrinsics(func->getIntrinsicID()))
            continue;
          replaceWorklist.push_back(callInstr);
        }
      }


      for (auto instr : replaceWorklist) {
        auto called = instr->getCalledFunction();
        if (!called)
          continue;

        auto intID = called->getIntrinsicID();
        switch (intID) {
          //case Intrinsic::molly_global_init:
          //  replaceGlobalInit(instr, called);
          //  break;
          //case Intrinsic::molly_global_free:
          //  replaceGlobalFree(instr, called);
          //  break;
        case Intrinsic::molly_cluster_current_coordinate:
        case Intrinsic::molly_cluster_pos:
          replaceClusterCurrentCoordinate(instr, called);
          break;
        case Intrinsic::molly_field_rankof:
          replaceFieldRankof(instr, called);
          break;
        case Intrinsic::molly_local_indexof:
          replaceLocalIndexof(instr, called);
          break;
        case Intrinsic::molly_value_load:
          replaceValueLoad(instr, called);
          break;
        case Intrinsic::molly_value_store:
          replaceValueStore(instr, called);
          break;
        case Intrinsic::molly_field_init:
          replaceFieldInit(instr, called);
          break;
        case Intrinsic::molly_field_free:
          replaceFieldFree(instr, called);
          break;
        case Intrinsic::molly_combuf_send_ptr:
          replaceCombufSendPtr(instr, called);
          break;
        case Intrinsic::molly_combuf_recv_ptr:
          replaceCombufRecvPtr(instr, called);
          break;
        case Intrinsic::molly_combuf_send:
          replaceCombufSend(instr, called);
          break;
        case Intrinsic::molly_combuf_recv:
          replaceCombufRecv(instr, called);
          break;
        case Intrinsic::molly_local_ptr:
          replaceLocalPtr(instr, called);
          break;
        case Intrinsic::molly_combuf_send_wait:
          replaceCombufSendWait(instr);
          break;
        case Intrinsic::molly_combuf_recv_wait:
          replaceCombufRecvWait(instr);
          break;
        case Intrinsic::molly_ptr:
          replacePtr(instr, called);
          break;
          //case Intrinsic::molly_cluster_myrank:
          //  replaceClusterMyrank(instr, called);
          //  break;
        default:
          llvm_unreachable("Need to replace intrinsic!");
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
      for (auto it = instrs.begin(), end = instrs.end(); it != end; ++it) {
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


    bool splitFieldAccessed(BasicBlock *bb) {
      auto name = bb->getName();
      bool changed = false;

      auto instr = &bb->front();
      auto term = bb->getTerminator();
      auto cnt = 0;
      BasicBlock *last = bb;
      while (instr && instr != term) {
        auto facc = getFieldAccess(instr);
        if (!facc.isValid()) { // Nothing to do
          instr = instr->getNextNode();
          continue;
        }

        auto gep = facc.getFieldCall();
        assert(gep->hasNUses(1));
        auto acc = facc.getAccessor();
        gep->moveBefore(acc);

        auto splitPt1 = gep;
        auto splitPt2 = acc->getNextNode();

        BasicBlock *before;
        BasicBlock *middle;
        BasicBlock *after;
        auto new1 = splitBlockIfNecessary(last, splitPt1, false, before, middle, this);
        auto new2 = splitBlockIfNecessary(middle, splitPt2, false, middle, after, this);
        changed = changed || new1 || new2;

        auto postfix = facc.isRead() ? ".fload" : ".fstore";
        middle->setName(name + Twine(postfix) + Twine(cnt));
        if (new2) {
          after->setName(name + Twine(".inter") + Twine(cnt));
        }

        cnt += 1;
        instr = splitPt2;
        last = after;
      }

      return changed;
    }


    void isolateFieldAccesses() {
      // "Normalize" existing BasicBlocks
     //auto ib = findOrRunAnalysis(&polly::IndependentBlocksID);
      auto ib = pm->findAnalysis(&polly::IndependentBlocksID, func);
      if (ib)
        removePass(ib); // Invalidate existing pass

      // Because splitFieldAccessed add more BasicBlocks that we do not want to enumerate
      SmallVector<BasicBlock*, 8> bbList;
      for (auto &b : *func) {
        bbList.push_back(&b);
      }

      bool changed = false;
      for (auto bb : bbList) {
        if (splitFieldAccessed(bb))
          changed = true;
      }

#if 0
      SmallVector<Instruction*, 16> instrs;
      collectInstructionList(func, instrs);
      for (auto instr : instrs){
        auto facc = getFieldAccess(instr);
        if (!facc.isValid())
          continue;
        // Create a distinct BB for this access
        // polly::IndependentBlocks will then make it independent from the other BBs and
        // polly::ScopInfo create a ScopStmt for it
        isolateInBB(facc);
        changed = true;
      }
#endif


      //if (changed) {
        // Also make new BBs independent
      //  ib = findOrRunAnalysis(&polly::IndependentBlocksID);
      //}


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

    Value* getClusterCoordinate(unsigned i) override {
      auto clusterConf = pm->getClusterConfig();

      currentClusterCoord.reserve(clusterConf->getClusterDims());
      while (i >= currentClusterCoord.size())
        currentClusterCoord.push_back(nullptr);

      auto &curCoord = currentClusterCoord[i];
      if (curCoord) {
        //assert(cast<CallInst>(curCoord)->getCalledFunction()->getIntrinsicID() == Intrinsic::molly_cluster_current_coordinate); // Or the runtime'e function
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

      for (auto i = nClusterDims - nClusterDims; i < nClusterDims; i += 1) {
        result.push_back(getClusterCoordinate(i));
      }

      return result;
    }

#pragma region molly::MollyFunctionProcessor
    llvm::FunctionPass *asPass() override {
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
