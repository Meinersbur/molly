#pragma once

#define DEBUG_TYPE "molly-distr"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "molly\FieldDetection.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include "MollyContextPass.h"

using namespace llvm;
using namespace molly;

namespace molly {
  ModulePass *createFieldDistributionPass();
}

namespace {

  class FieldDistribution : public ModulePass {
  private:
    FieldDetectionAnalysis *fa;

    FieldVariable *getFieldVariable(GlobalVariable *var);
    FieldType *getFieldType(StructType *ty);

    bool runOnField(FieldVariable *field);

  public:
    static char ID;
    FieldDistribution() : ModulePass(ID) {
    }

    virtual const char *getPassName() const {
      return ModulePass::getPassName();
    }

    virtual bool doInitialization(Module &M)  { 
      return ModulePass::doInitialization(M);
    }

    virtual bool doFinalization(Module &M) { 
      return ModulePass::doFinalization(M);
    }

    virtual void print(raw_ostream &O, const Module *M) const {
      ModulePass::print(O, M);
    }

    virtual bool runOnModule(Module &M);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MollyContextPass>();
      AU.addRequired<FieldDetectionAnalysis>();
    }

    virtual void preparePassManager(PMStack &PMS) {
      ModulePass::preparePassManager(PMS);
    }

    virtual void assignPassManager(PMStack &PMS, PassManagerType T) {
      ModulePass::assignPassManager(PMS, T);
    }

    virtual PassManagerType getPotentialPassManagerType() const {
      return ModulePass::getPotentialPassManagerType();
    }

    virtual Pass *createPrinterPass(raw_ostream &O, const std::string &Banner) const  {
      return ModulePass::createPrinterPass(O, Banner);
    }

    virtual void releaseMemory() {
      ModulePass::releaseMemory();
    }

    virtual void *getAdjustedAnalysisPointer(AnalysisID ID) {
      return ModulePass::getAdjustedAnalysisPointer(ID);
    }

    virtual ImmutablePass *getAsImmutablePass() {
      return ModulePass::getAsImmutablePass();
    }

    virtual PMDataManager *getAsPMDataManager() {
      return ModulePass::getAsPMDataManager();
    }

    virtual void verifyAnalysis() const {
      return ModulePass::verifyAnalysis();
    }

    virtual void dumpPassStructure(unsigned Offset = 0) {
      return ModulePass::dumpPassStructure(Offset);
    }
  };
}


FieldVariable *FieldDistribution::getFieldVariable(GlobalVariable *var) {
  FieldVariable *result = fa->getFieldVariables()[var];
  assert(result);
  return result;
}


FieldType *FieldDistribution::getFieldType(StructType *ty) {
  FieldType *result = fa->getFieldTypes()[ty];
  assert(result);
  return result;
}



bool FieldDistribution::runOnField(FieldVariable *field) {
  auto fieldTy = field->getFieldType();
  auto &contextPass = getAnalysis<MollyContextPass>();
  auto &clusterShape = contextPass.getClusterShape();
  auto &clusterLengths = contextPass.getClusterShape();

  return false;
}

bool FieldDistribution::runOnModule(Module &M) {
  fa = &getAnalysis<FieldDetectionAnalysis>();

  auto fieldVars = fa->getFieldVariables();
  for (auto it = fieldVars.begin(), end = fieldVars.end(); it != end; ++it) {
    FieldVariable *field = it->second;
    field->dump();
  }

  auto fieldTys = fa->getFieldTypes();
  for (auto it = fieldTys.begin(), end = fieldTys.end(); it != end; ++it) {
    FieldType *field = it->second;
    field->dump();
  }

  //M.dump();


  auto changed = false;
    for (auto it = fieldVars.begin(), end = fieldVars.end(); it != end; ++it) {
    FieldVariable *field = it->second;
    bool fieldChanged = runOnField(field);
    changed = changed || fieldChanged;
  }

  return changed;
}


char FieldDistribution::ID = 0;
static RegisterPass<FieldDistribution> FieldDistributionRegistration("moly-distr", "Molly - Field Distribution");


// Declaration in LinkAllPasses.h
ModulePass *molly::createFieldDistributionPass() {
  return new FieldDistribution();
}
