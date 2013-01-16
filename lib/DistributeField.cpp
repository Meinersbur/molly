#pragma once

#define DEBUG_TYPE "molly-distr"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace molly;

namespace {

	class FieldDistribution : public ModulePass {
	public:
		static char ID;
		FieldDistribution() : ModulePass(ID) {
		}

		virtual bool runOnModule(Module &M);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.setPreservesAll();
		}

	private:
		Pass *createFieldDistributionPass();



	};
}


bool FieldDistribution::runOnModule(Module &M) {
	return false;
}


char FieldDistribution::ID = 0;
static RegisterPass<FieldDistribution> FieldDistributionRegistration("moly-distr", "Molly - Field Distribution");

