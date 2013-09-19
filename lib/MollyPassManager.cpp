#define DEBUG_TYPE "molly"
#include "MollyPassManager.h"

#include <llvm/Support/Debug.h>
#include <llvm/Pass.h>
#include "MollyUtils.h"
#include <llvm/ADT/DenseSet.h>
#include <polly/ScopInfo.h>
#include <polly/LinkAllPasses.h>
#include "ClusterConfig.h"
#include <llvm/Support/CommandLine.h>
#include "molly/RegisterPasses.h"
#include <polly/RegisterPasses.h>
#include <llvm/Support/Debug.h>
#include "FieldType.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <polly/ScopInfo.h>
#include "MollyUtils.h"
#include <llvm/Support/ErrorHandling.h>
#include "ScopUtils.h"
#include "MollyFieldAccess.h"
#include "islpp/Set.h"
#include "islpp/Map.h"
#include <llvm/Analysis/ScalarEvolution.h>
#include "FieldVariable.h"
#include <llvm/IR/Intrinsics.h>
#include "islpp/UnionMap.h"
#include "ScopEditor.h"
#include "islpp/Point.h"
#include <polly/TempScopInfo.h>
#include <polly/LinkAllPasses.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <polly/CodeGen/BlockGenerators.h>
#include "llvm/ADT/StringRef.h"
#include "MollyIntrinsics.h"
#include "CommunicationBuffer.h"
#include <clang/CodeGen/MollyRuntimeMetadata.h>
#include "IslExprBuilder.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include <polly/CodeGen/CodeGeneration.h>

#include "MollyModuleProcessor.h"
#include "MollyFunctionProcessor.h"
#include "MollyRegionProcessor.h"
#include "MollyScopProcessor.h"
#include "MollyScopStmtProcessor.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


cl::opt<string> MollyShape("shape", cl::desc("Molly - MPI cartesian grid shape"));


namespace {

  class MollyPassManagerImpl : public MollyPassManager, public ModulePass {
    typedef DefaultIRBuilder BuilderTy;

  private:
    bool changedIR;
    bool changedScop;

    void modifiedIR() LLVM_OVERRIDE {
      changedIR = true;
    }

  private:
    Module *module;

    llvm::DenseSet<AnalysisID> alwaysPreserve;
    llvm::DenseMap<llvm::AnalysisID, llvm::ModulePass*> currentModuleAnalyses; 
    llvm::DenseMap<std::pair<llvm::AnalysisID, llvm::Function*>, llvm::FunctionPass*> currentFunctionAnalyses; 
    llvm::DenseMap<std::pair<llvm::AnalysisID,llvm::Region*>, llvm::RegionPass*> currentRegionAnalyses; 

    void removePass(Pass *pass) LLVM_OVERRIDE {
      if (!pass)
        return;

      //FIXME: Also remove transitively dependent passes
      //TODO: Can this be done more efficiently?
      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto moduleAnalysisID = it->first;
        auto moduleAnalysisPass = it->second;

        if (moduleAnalysisPass==pass)
          currentModuleAnalyses.erase(it);
      }

      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto function = it->first.second;
        auto functionAnalysisID = it->first.first;
        auto functionAnalysisPass = it->second;

        if (functionAnalysisPass==pass)
          currentFunctionAnalyses.erase(it);
      }

      decltype(currentRegionAnalyses) rescuedRegionAnalyses;
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto region = it->first.second;
        auto regionAnalysisID = it->first.first;
        auto regionAnalysisPass = it->second;

        if (regionAnalysisPass==pass)
          currentRegionAnalyses.erase(it);
      }

      pass->releaseMemory();
      delete pass;
    }


    void removeUnpreservedAnalyses(const AnalysisUsage &AU, Function *func, Region *inRegion) {
      if (AU.getPreservesAll()) 
        return;

      if (!func && inRegion) {
        func = getParentFunction(inRegion);
      }
      //TODO: Also mark passes inherited from Resolver->getAnalysisIfAvailable as invalid so they are not reuse
      auto &preserved = AU.getPreservedSet();
      DenseSet<AnalysisID> preservedSet;
      for (auto it = preserved.begin(), end = preserved.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }
      for (auto it = alwaysPreserve.begin(), end = alwaysPreserve.end(); it!=end; ++it) {
        auto preservedID = *it;
        preservedSet.insert(preservedID);
      }

      DenseSet<Pass*> donotFree; 
      decltype(currentModuleAnalyses) rescuedModuleAnalyses;
      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto moduleAnalysisID = it->first;
        auto moduleAnalysisPass = it->second;

        if (preservedSet.find(moduleAnalysisID) != preservedSet.end()) {
          donotFree.insert(moduleAnalysisPass);
          rescuedModuleAnalyses[moduleAnalysisID] = moduleAnalysisPass;
        }
      }

      decltype(currentFunctionAnalyses) rescuedFunctionAnalyses;
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto function = it->first.second;
        auto functionAnalysisID = it->first.first;
        auto functionAnalysisPass = it->second;

        if (preservedSet.count(functionAnalysisID) || (func && func!=function)) {
          donotFree.insert(functionAnalysisPass);
          rescuedFunctionAnalyses[std::make_pair(functionAnalysisID, function)] = functionAnalysisPass;
        }
      }

      decltype(currentRegionAnalyses) rescuedRegionAnalyses;
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto region = it->first.second;
        auto regionAnalysisID = it->first.first;
        auto regionAnalysisPass = it->second;

        if (preservedSet.count(regionAnalysisID) || (inRegion && inRegion!=region)) {
          donotFree.insert(regionAnalysisPass);
          rescuedRegionAnalyses[std::make_pair(regionAnalysisID, region)] = regionAnalysisPass;
        }
      }


      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      } 
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      }
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        if (donotFree.count(pass))
          continue;

        donotFree.insert(pass);
        pass->releaseMemory();
        delete pass;
      }

      currentModuleAnalyses = std::move(rescuedModuleAnalyses);
      currentFunctionAnalyses = std::move(rescuedFunctionAnalyses);
      currentRegionAnalyses = std::move(rescuedRegionAnalyses);
    }


    void removeAllAnalyses() {
      DenseSet<Pass*> doFree; // To avoid double-free

      for (auto it = currentModuleAnalyses.begin(), end = currentModuleAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      } 
      currentModuleAnalyses.clear();
      for (auto it = currentFunctionAnalyses.begin(), end = currentFunctionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentFunctionAnalyses.clear();
      for (auto it = currentRegionAnalyses.begin(), end = currentRegionAnalyses.end(); it!=end; ++it) {
        auto pass = it->second;
        doFree.insert(pass);
      }
      currentRegionAnalyses.clear();

      for (auto it = doFree.begin(), end = doFree.end(); it!=end; ++it) {
        auto pass = *it;
        pass->releaseMemory();
        delete pass;
      }
    }

  private:
    std::vector<Pass*> pollyPasses;

    void addPollyPass(Pass *pass) {
      pollyPasses.push_back(pass);
    }

  public:
    static char ID;
    MollyPassManagerImpl() : ModulePass(ID), moduleCtx(nullptr) {
      //alwaysPreserve.insert(&polly::PollyContextPassID);
      //alwaysPreserve.insert(&molly::MollyContextPassID);
      //alwaysPreserve.insert(&polly::ScopInfo::ID);
      //alwaysPreserve.insert(&polly::IndependentBlocksID);

      this->callToMain = nullptr;
    }


    ~MollyPassManagerImpl() {
      removeAllAnalyses();
    }


    MollyModuleProcessor *moduleCtx;

    void runModulePass(llvm::ModulePass *pass) LLVM_OVERRIDE {
      auto passID = pass->getPassID();
      assert(!currentModuleAnalyses.count(passID));

      // Execute prerequisite passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, false);
      }

      // Execute pass
      pass->setResolver(MollyModuleProcessor::createResolver(this, module));
      bool changed = pass->runOnModule(*module);
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, nullptr);

      // Register pass 
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentModuleAnalyses[intfPassID] = pass;
      }
      currentModuleAnalyses[passID] = pass;
    }


    ModulePass *findAnalysis(AnalysisID passID) {
      auto preexisting = Resolver->getAnalysisIfAvailable(passID, true);
      if (preexisting) {
        assert(preexisting->getPassKind() == PT_Module);
        return static_cast<ModulePass*>(preexisting);
      }

      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end()) {
        return it->second;
      }
      return nullptr;
    }
    ModulePass *findOrRunAnalysis(AnalysisID passID, bool permanent = false) {
      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end()) {
        return it->second;
      }

      auto parentPass = getResolver()->getAnalysisIfAvailable(passID, true);
      if (parentPass) {
        assert(parentPass->getPassKind() == PT_Module);
        return static_cast<ModulePass*>(parentPass);
      }

      auto pass = createPassFromId(passID);
      assert(pass->getPassKind() == PT_Module);
      auto modulePass = static_cast<ModulePass*>(pass);
      runModulePass(modulePass);
      return modulePass;
    }
    ModulePass *findOrRunAnalysis(const char &passID, bool permanent = false) {
      return findOrRunAnalysis(&passID, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis(bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }
    template<typename T>
    T *findOrRunPemanentAnalysis() {
      return findOrRunAnalysis<T>(true);
    }


    void runFunctionPass(llvm::FunctionPass *pass, Function *func) LLVM_OVERRIDE {
      auto passID = pass->getPassID(); 
      assert(!currentFunctionAnalyses.count(std::make_pair(passID, func)));

      // Execute prequisite passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, func, false);
      }

      // Execute pass
      pass->setResolver(MollyFunctionProcessor::createResolver(this, func));
      bool changed = pass->runOnFunction(*func);
      if (changed)
        removeUnpreservedAnalyses(AU, func, nullptr);

      // Register pass
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentFunctionAnalyses[std::make_pair(intfPassID, func)] = pass;
      }
      currentFunctionAnalyses[std::make_pair(passID, func)] = pass;
    }


    Pass *findAnalysis(AnalysisID passID, Function *func) LLVM_OVERRIDE {
      auto it = currentFunctionAnalyses.find(make_pair(passID, func));
      if (it != currentFunctionAnalyses.end()) {
        return it->second;
      }

      return findAnalysis(passID);
    }


    Pass *findOrRunAnalysis(AnalysisID passID, Function *func) LLVM_OVERRIDE {
      auto existing = findAnalysis(passID, func);
      if (existing)
        return existing;

      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass));
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }
      return pass;
    }


    Pass *findOrRunAnalysis(const char &passID, Function *func, bool permanent) {
      return findOrRunAnalysis(&passID,func); 
    }


    template<typename T>
    T *findOrRunAnalysis(Function *func, bool permanent = false) { 
      return (T*)findOrRunAnalysis(T::ID, func, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }


    template<typename T>
    T *findOrRunPemanentAnalysis(Function *func) {
      return (T*)findOrRunAnalysis(&T::ID, func, false)->getAdjustedAnalysisPointer(&T::ID); 
    }


  private:
    llvm::DenseMap<Region*, MollyRegionProcessor*> regionContexts;
    MollyRegionProcessor *getRegionContext(Region *region) {
      auto &result = regionContexts[region];
      if (!result) {
        result = MollyRegionProcessor::create(this, region);
      }
      return  result;
    }

  public:
    void runRegionPass(llvm::RegionPass *pass, Region *region) LLVM_OVERRIDE {
      auto passID = pass->getPassID();
      assert(!currentRegionAnalyses.count(std::make_pair(passID, region)));

      // Execute required passes
      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      auto &requiredSet = AU.getRequiredSet();
      for (auto it = requiredSet.begin(), end = requiredSet.end(); it!=end; ++it) {
        auto requiredID = *it;
        //auto requiredPass = static_cast<llvm::ModulePass*>(createPassFromId(requiredID));
        findOrRunAnalysis(requiredID, region, false);
      }

      // Execute pass
      pass->setResolver(MollyRegionProcessor::createResolver(this, region));
      bool changed = pass->runOnRegion(region, *(static_cast<RGPassManager*>(nullptr)));
      if (changed)
        removeUnpreservedAnalyses(AU, nullptr, region); //TODO: Re-add required analyses?

      // Add pass info
      auto passRegistry = PassRegistry::getPassRegistry();
      auto passInfo = passRegistry->getPassInfo(passID);
      auto &intfs = passInfo->getInterfacesImplemented();
      for (auto it = intfs.begin(), end = intfs.end(); it!=end; ++it) {
        auto intf = *it;
        auto intfPassID = intf->getTypeInfo();
        currentRegionAnalyses[std::make_pair(intfPassID, region)] = pass;
      }
      currentRegionAnalyses[std::make_pair(passID, region)] = pass;
    }


    Pass *findAnalysis(AnalysisID passID, Function *func, Region *region) LLVM_OVERRIDE {
      if (region && !func) {
        func = getParentFunction(region);
      }

      if (region) {
        auto it = currentRegionAnalyses.find(make_pair(passID, region));
        if (it != currentRegionAnalyses.end())
          return it->second;
      }

      if (func) {
        auto it = currentFunctionAnalyses.find(make_pair(passID, func));
        if (it != currentFunctionAnalyses.end())
          return it->second;
      }

      auto it = currentModuleAnalyses.find(passID);
      if (it != currentModuleAnalyses.end())
        return it->second;

      auto preexisting = Resolver->getAnalysisIfAvailable(passID, true);
      if (preexisting) {
        assert(preexisting->getPassKind() == PT_Module);
        return preexisting;
      }

      return nullptr;
    }

    template<typename T>
    T* findAnalysis(Function *func, Region *region) {
      auto passID = &T::ID;
      auto pass = findAnalysis(passID, func, region);
      return static_cast<T*>(pass->getAdjustedAnalysisPointer(passID));
    }


    Pass *findOrRunAnalysis(AnalysisID passID, Function *func, Region *region) {
      if (region && !func)
        func = getParentFunction(region);

      auto foundPass = findAnalysis(passID, func, region);
      if (foundPass)
        return foundPass;

      auto newPass = createPassFromId(passID);
      switch (newPass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(newPass));
        break;
      case PT_Function:
        assert(func);
        runFunctionPass(static_cast<FunctionPass*>(newPass), func);
        break;
      case PT_Region:
        assert(region);
        runRegionPass(static_cast<RegionPass*>(newPass), region);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }

      return newPass;
    }

    Pass *findAnalysis(AnalysisID passID, Region *region) {
      auto it = currentRegionAnalyses.find(make_pair(passID, region));
      if (it != currentRegionAnalyses.end()) {
        return it->second;
      }

      auto func = region->getEntry()->getParent();
      return findAnalysis(passID, func);
    }


    Pass *findOrRunAnalysis(AnalysisID passID, Region *region, bool permanent = false) {
      auto existing = findAnalysis(passID, region);
      if (existing)
        return existing;

      auto func = region->getEntry()->getParent();
      auto pass = createPassFromId(passID);
      switch (pass->getPassKind()) {
      case PT_Module:
        runModulePass(static_cast<ModulePass*>(pass));
        break;
      case PT_Function:
        runFunctionPass(static_cast<FunctionPass*>(pass), func);
        break;
      case PT_Region:
        runRegionPass(static_cast<RegionPass*>(pass), region);
        break;
      default:
        llvm_unreachable("Wrong kind of pass");
      }
      return pass;
    }
    Pass *findOrRunAnalysis(const char &passID,  Region *region,bool permanent ) {
      return findOrRunAnalysis(&passID,region, permanent); 
    }
    template<typename T>
    T *findOrRunAnalysis( Region *region,bool permanent = false) { 
      return (T*)findOrRunAnalysis(&T::ID, region, permanent)->getAdjustedAnalysisPointer(&T::ID);
    }



    template<typename T>
    T *findOrRunPemanentAnalysis( Region *region) {
      return (T*)findOrRunAnalysis(&T::ID, region, false)->getAdjustedAnalysisPointer(&T::ID); 
    }


  protected:


  private:
    void runOnFunction(Function *func) {
    }

  private:
    //llvm::OwningPtr< isl::Ctx > islctx;
    isl::Ctx *islctx;

  public:
    llvm::LLVMContext &getLLVMContext() const LLVM_OVERRIDE { return module->getContext(); }
    isl::Ctx *getIslContext() { return islctx; }

  private:
    llvm::OwningPtr<ClusterConfig> clusterConf;

    void parseClusterShape() {
      string shape = MollyShape;
      SmallVector<StringRef, 4> shapeLengths;
      SmallVector<unsigned, 4> clusterLengths;
      StringRef(shape).split(shapeLengths, "x");

      for (auto it = shapeLengths.begin(), end = shapeLengths.end(); it != end; ++it) {
        auto slen = *it;
        unsigned len;
        auto retval = slen.getAsInteger(10, len);
        assert(retval==false);
        clusterLengths.push_back(len);
      }

      clusterConf->setClusterLengths(clusterLengths);
    }


#pragma region Field Detection
  private:
    llvm::DenseMap<llvm::StructType*, FieldType*> fieldTypes;

    void fieldDetection() {
      auto &glist = module->getGlobalList();
      auto &flist = module->getFunctionList();
      auto &alist = module->getAliasList();
      auto &mlist = module->getNamedMDList();
      auto &vlist = module->getValueSymbolTable();

      auto fieldsMD = module->getNamedMetadata("molly.fields"); 
      if (fieldsMD) {
        auto numFields = fieldsMD->getNumOperands();

        for (unsigned i = 0; i < numFields; i+=1) {
          auto fieldMD = fieldsMD->getOperand(i);
          FieldType *field = FieldType::createFromMetadata(islctx, module, fieldMD);
          fieldTypes[field->getType()] = field;
        }
      }
    }
#pragma endregion


#pragma region Field Distribution  
    void fieldDistribution_processFieldType(FieldType *fty) {
      auto lengths = fty->getLengths();
      auto cluster = clusterConf->getClusterLengths();
      auto nDims = lengths.size();

      SmallVector<int,4> locallengths;
      for (auto d = nDims-nDims; d<nDims; d+=1) {
        auto len = lengths[d];
        auto clusterLen = (d < cluster.size()) ? cluster[d] : 1;

        assert(len % clusterLen == 0);
        auto localLen = (len + clusterLen - 1) / clusterLen;
        locallengths.push_back(localLen);
      }
      fty->setDistributed();
      fty->setLocalLength(locallengths, clusterConf->getClusterTuple());
    }


    void fieldDistribution() {
      for (auto it : fieldTypes) {
        fieldDistribution_processFieldType(it.second);
      }
    }
#pragma endregion


#pragma region Field CodeGen
    Function *emitLocalLength(FieldType *fty) {
      auto &context = module->getContext();
      auto llvmTy = fty->getType();
      auto nDims = fty->getNumDimensions();
      auto intTy = Type::getInt64Ty(context);
      auto lengths = fty->getLengths();
      //auto molly = &getAnalysis<MollyContextPass>();

      SmallVector<Type*, 5> argTys;
      argTys.push_back(PointerType::getUnqual(llvmTy));
      argTys.append(nDims, intTy);
      auto localLengthFuncTy = FunctionType::get(intTy, argTys, false);

      auto localLengthFunc = Function::Create(localLengthFuncTy, GlobalValue::InternalLinkage, "molly_locallength", module);
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
        ReturnInst::Create(context, ConstantInt::get(intTy, lengths[d] / clusterConf ->getClusterLength(d)/*FIXME: wrong if nondivisible*/), caseBB);

        sw->addCase(ConstantInt::get(intTy, d), caseBB);
      }

      modifiedIR();
      fty->setLocalLengthFunc(localLengthFunc);
      return localLengthFunc;
    }

    void generateAccessFuncs() {
      for (auto it : fieldTypes) {
        emitLocalLength(it.second);
      }
    }

  private:
    CallInst *callToMain;

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
      this->callToMain = ret;
      DEBUG(llvm::dbgs() << ">>>Wrapped main\n");
      modifiedIR();
      builder.CreateRet(ret);
    }
#pragma endregion


    MollyFunctionProcessor *getFuncContext(Function *func) LLVM_OVERRIDE {
      auto &ctx = funcs[func];
      if (!ctx)
        ctx = MollyFunctionProcessor::create(this, func);
      return ctx;
    }


    MollyScopProcessor *getScopContext(Scop *scop) LLVM_OVERRIDE {
      auto &ctx = scops[scop];
      if (!ctx)
        ctx = MollyScopProcessor::create(this, scop);
      return ctx;
    }


  private:
    DenseMap<ScopStmt*, MollyScopStmtProcessor*> stmts;
  public:
    MollyScopStmtProcessor *getScopStmtContext(ScopStmt *stmt) {
      auto &ctx = stmts[stmt];
      if (!ctx)
        ctx = MollyScopStmtProcessor::create(this, stmt);
      return ctx;
    }


  private:
    void setAlwaysPreserve(Pass *pass) {
      alwaysPreserve.insert(pass);

      AnalysisUsage AU;
      pass->getAnalysisUsage(AU);
      for (auto reqTrans : AU.getRequiredTransitiveSet()) {
        assert(!"TODO: Also preserve transitive requirements");
      }
    }

#pragma region Scop Detection
  private:
    DenseMap<Function*, MollyFunctionProcessor*> funcs;
    DenseMap<Scop*, MollyScopProcessor*> scops;

    ScopInfo *si;

    void runScopDetection() {
      for (auto &it : *module) {
        auto func = &it;
        if (func->isDeclaration())
          continue;

        auto funcCtx = MollyFunctionProcessor::create(this, func);
        funcs[func] = funcCtx;

        auto regionInfo = findOrRunAnalysis<RegionInfo>(func);
        SmallVector<Region*,12> regions;
        collectAllRegions(regionInfo->getTopLevelRegion(), regions);
        for (auto region : regions) {
          auto scopInfo = new ScopInfo(islctx->keep()); // We need ScopInfo to convince to take our isl_ctx
          runRegionPass(scopInfo, region);
          setAlwaysPreserve(scopInfo);
          auto scop = scopInfo->getScop();
          if (scop) {
            auto scopCtx = MollyScopProcessor::create(this, scop);
            scops[scop] = scopCtx;
          }
        }
      }
    }
#pragma endregion


    FieldType *lookupFieldType(llvm::StructType *ty) {
      auto result = fieldTypes.find(ty);
      if (result == fieldTypes.end())
        return nullptr;
      return result->second;
    }


  public:
    FieldType *getFieldType(llvm::StructType *ty) {
      auto result = lookupFieldType(ty);
      assert(result);
      return result;
    }

    FieldType *getFieldType(llvm::Value *val) {
      auto ty = val->getType();
      if (auto pty = dyn_cast<PointerType>(ty)) {
        ty = pty->getPointerElementType();
      }
      return getFieldType(llvm::cast<llvm::StructType>(ty));
    }


    private:
      DenseMap<const GlobalVariable*, FieldVariable*> fvars;
  public:
    FieldVariable *getFieldVariable(GlobalVariable *gvar) {
      auto fieldTy = getFieldType(cast<StructType>(gvar->getType()->getPointerElementType()));
      auto &fvar = fvars[gvar];
      if (!fvar) {
        fvar = FieldVariable::create(gvar, fieldTy);
      }
      return fvar;
    }


    FieldVariable *getFieldVariable(llvm::Value *val) LLVM_OVERRIDE {
      if (!isa<GlobalVariable>(val))
        return nullptr;
      return getFieldVariable(cast<GlobalVariable>(val));
    }


    void augmentFieldVariable(MollyFieldAccess &facc) {
      if (!facc.isValid())
        return;

      auto base = facc.getBaseField();
      auto globalbase = dyn_cast<GlobalVariable>(base);
      assert(globalbase && "Currently only global fields supported");
      auto gvar = getFieldVariable(globalbase);
      assert(gvar);
      facc.setFieldVariable(gvar);
    }


    MollyFieldAccess getFieldAccess(llvm::Instruction *instr) {
      assert(instr);
      auto result = MollyFieldAccess::fromAccessInstruction(instr);
      augmentFieldVariable(result); 
      return result;
    }


    MollyFieldAccess getFieldAccess(ScopStmt *stmt) LLVM_OVERRIDE {
      assert(stmt);
      auto result = MollyFieldAccess::fromScopStmt (stmt);
      augmentFieldVariable(result);
      return result;
    }


    MollyFieldAccess getFieldAccess(polly::MemoryAccess *memacc) LLVM_OVERRIDE {
      assert(memacc);
      auto result = MollyFieldAccess::fromMemoryAccess(memacc);
      augmentFieldVariable(result);
      return result;
    }



#pragma region Communication buffers
  private:
    std::vector<CommunicationBuffer *> combufs;

  public :
    CommunicationBuffer *newCommunicationBuffer(FieldType *fty, const isl::Map &relation) {
      auto comvarSend = new GlobalVariable(*module, runtimeMetadata.tyCombufSend, false, GlobalValue::PrivateLinkage, nullptr, "combufsend");
      auto comvarRecv = new GlobalVariable(*module, runtimeMetadata.tyCombufRecv, false, GlobalValue::PrivateLinkage, nullptr, "combufrecv");
      auto result = CommunicationBuffer::create(comvarSend, comvarRecv, fty, relation.copy());
      combufs.push_back(result);
      return result;
    }

  private:
    llvm::Function* emitCombufInit(CommunicationBuffer *combuf) {
      auto &llvmContext = module->getContext();
      auto func = createFunction(nullptr, module, GlobalValue::PrivateLinkage, "initCombuf");
      auto bb = BasicBlock::Create(llvmContext, "entry", func);
      IRBuilder<> builder (bb);
      auto rtn = builder.CreateRetVoid();

      auto editor = ScopEditor::newScop(rtn, getFuncContext(func)->asPass());
      auto scop = editor.getScop();
      auto scopCtx = getScopContext(scop);

      auto rel = combuf->getRelation(); /* { (src[coord] -> dst[coord]) -> field[indexset] } */
      auto mapping = combuf->getMapping();
      //auto eltCount = combuf->getEltCount();

      auto scatterTupleId = isl::Space::enwrap( scop->getScatteringSpace() ).getSetTupleId();
      auto singletonSet = islctx->createSetSpace(0,1).setSetTupleId(scatterTupleId).createUniverseBasicSet().fix(isl_dim_set, 0, 0);

      // Send buffers
      auto sendWhere = rel.getDomain().unwrap(); /* { src[coord] -> dst[coord] } */ // We are on the src node, send to dst node
      auto sendDomain = sendWhere.getRange(); /* { dst[coord] }*/
      auto sendScattering = islctx->createAlltoallMap(sendDomain, singletonSet) ; /* { dst[coord] -> scattering[] } */
      auto stmtEditor = editor.createStmt(sendDomain.copy(), sendScattering.copy(), sendWhere.copy(), "sendcombuf_create");
      auto sendStmt = stmtEditor.getStmt();
      auto sendCtx = getScopStmtContext(sendStmt);
      auto sendBB = stmtEditor.getBasicBlock();
      IRBuilder<> sendBuilder(stmtEditor. getTerminator());
      auto domainVars = sendCtx->getDomainValues();
      auto sendDstRank = clusterConf->codegenComputeRank(sendBuilder,domainVars);
      std::map<isl_id *, llvm::Value *> sendParams;
      editor.getParamsMap( sendParams, sendStmt);
      //auto sendSize = buildIslAff(sendBuilder.GetInsertPoint(), eltCount.takeCopy(), sendParams, this);
      //Value* sendArgs[] = { sendDstRank, sendSize };
      //sendBuilder.CreateCall(runtimeMetadata.funcCreateSendCombuf, sendArgs);

      // Receive buffers
      auto recvWhere = sendWhere.reverse(); /* { src[coord] -> dst[coord] } */ // We are on dst node, recv from src
      auto recvDomain = recvWhere.getDomain(); /* { src[coord] } */
      auto recvScatter = islctx->createAlltoallMap(recvDomain, singletonSet) ; /* { src[coord] -> scattering[] } */
      auto recvEditor = editor.createStmt(sendDomain.copy(), sendScattering.copy(), sendWhere.copy(), "sendcombuf_create");
      auto recvStmt = recvEditor.getStmt();
      auto recvCtx = getScopStmtContext(recvStmt);
      auto recvBB = recvEditor.getBasicBlock();
      IRBuilder<> recvBuilder(recvEditor.getTerminator());
      auto recvVars = recvCtx->getDomainValues();
      auto recvSrcRank = clusterConf->codegenComputeRank(sendBuilder,recvVars);
      std::map<isl_id *, llvm::Value *> recvParams;
      editor.getParamsMap(recvParams, recvStmt);
      //auto recvSize = buildIslAff(recvBuilder, eltCount, recvParams);
      //auto recvSize =  buildIslAff(recvBuilder.GetInsertPoint(), eltCount.takeCopy(), recvParams, this);
      //Value* recvArgs[] = { recvSrcRank, recvSize };
      //sendBuilder.CreateCall(runtimeMetadata.funcCreateRecvCombuf, sendArgs);

      return func;
    }


    Function *emitAllCombufInit() {
      auto &llvmContext = module->getContext();
      auto allInitFunc = createFunction(nullptr, module, GlobalValue::PrivateLinkage, "initCombufs");
      auto bb = BasicBlock::Create(llvmContext, "entry", allInitFunc);
      IRBuilder<> builder (bb);

      for (auto combuf : combufs) {
        auto func = emitCombufInit(combuf);
        builder.CreateCall(func);
      }

      builder.CreateRetVoid();
      return allInitFunc;
    }


    void addCallToCombufInit() {
      auto initFunc = emitAllCombufInit();
      IRBuilder<> builder(callToMain);
      builder.CreateCall(initFunc);
    } 
#pragma endregion


#pragma region Runtime
  private:
    clang::CodeGen::MollyRuntimeMetadata runtimeMetadata;

  public:
    llvm::Type *getCombufSendType() { return runtimeMetadata.tyCombufSend; } 
    llvm::Type *getCombufRecvType() { return runtimeMetadata.tyCombufRecv; } 
    llvm::Function *getCombufSendFunc() { return runtimeMetadata.funcCombufSend; }
#pragma endregion



#pragma region llvm::ModulePass
  public:
    void releaseMemory() LLVM_OVERRIDE {
      removeAllAnalyses();
    }
    const char *getPassName() const LLVM_OVERRIDE { 
      return "MollyPassManager"; 
    }
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const LLVM_OVERRIDE {
      AU.addRequired<DataLayout>();
      // Requires nothing, preserves nothing
    }

    bool runOnModule(llvm::Module &M) LLVM_OVERRIDE {
      this->changedIR = false;
      this->changedScop = false;
      this->module = &M;
      this->moduleCtx = MollyModuleProcessor::create(this, module);

      // PollyContext
      // Need one isl_ctx to combine isl objects from different SCoPs 
      this->islctx = isl::Ctx::create();

      // Cluster configuration
      this->clusterConf.reset(new ClusterConfig(islctx));
      parseClusterShape();

      // Get runtime info
      runtimeMetadata.readMetadata(module);

      // Search fields
      fieldDetection();

      // Decide where fields should have their home location
      fieldDistribution();

      // Generate access functions
      generateAccessFuncs();

      wrapMain();

      for (auto &f : *module) {
        auto func = &f;
        if (func->isDeclaration())
          continue;

        auto funcCtx = getFuncContext(func);
        funcCtx->isolateFieldAccesses();
      }

      // Find all scops
      runScopDetection();

      for (auto &it : scops) {
        auto scop = it.first;
        auto scopCtx = it.second;
        if (!scopCtx->hasFieldAccess())
          continue;

        // Decide on which node(s) a ScopStmt should execute 
        scopCtx->computeScopDistibution();
        scopCtx->validate();

        // Insert communication between ScopStmt
        scopCtx->genCommunication();
        scopCtx->validate();
      }

      // Create some SCoPs that init the combufs
      addCallToCombufInit();

      for (auto &it : scops) {
        auto scop = it.first;
        auto scopCtx = it.second;
        if (!scopCtx->hasFieldAccess())
          continue;

        for (auto stmt : *scop) {
          auto stmtCtx = getScopStmtContext(stmt);
          stmtCtx->applyWhere();
          stmtCtx->validate();
        }

        // Let polly optimize and and codegen the scops
        //scopCtx->pollyOptimize();
        scopCtx->pollyCodegen();
      }


      // Replace all remaining accesses by some generated intrinsic
      for (auto &func : *module) {
        auto funcCtx = getFuncContext(&func);
        funcCtx->replaceRemainaingIntrinsics();
      }

      //FIXME: Find all the leaks
      //this->islctx.reset();
      this->clusterConf.reset();
      //delete this->moduleCtx;
      return changedIR;
    }


#pragma region molly::MollyPassManager
    ClusterConfig *getClusterConfig() LLVM_OVERRIDE {
      return clusterConf.get();
    }

    clang::CodeGen::MollyRuntimeMetadata *getRuntimeMetadata() LLVM_OVERRIDE {
      return &runtimeMetadata;
    }

    void modifiedScop() LLVM_OVERRIDE {
      changedScop = true;
    }
#pragma endregion

  }; // class MollyPassManagerImpl

} // namespace


char MollyPassManagerImpl::ID = 0;
extern char &::molly::MollyPassManagerID = MollyPassManagerImpl::ID;
ModulePass *::molly::createMollyPassManager() {
  return new MollyPassManagerImpl();
}
