#pragma once

#define DEBUG_TYPE "molly-distr"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "molly\FieldDetection.h"

using namespace llvm;
using namespace molly;

namespace molly {
	ModulePass *createFieldDistributionPass();
}

namespace {

	class FieldDistribution : public ModulePass {
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


bool FieldDistribution::runOnModule(Module &M) {
	M.dump();
	return false;
}


char FieldDistribution::ID = 0;
static RegisterPass<FieldDistribution> FieldDistributionRegistration("moly-distr", "Molly - Field Distribution");


// Declaration in LinkAllPasses.h
ModulePass *molly::createFieldDistributionPass() {
	return new FieldDistribution();
}
