#include "MollyContextPass.h"

#include "MollyContext.h"
#include "islpp/Space.h"
#include "islpp/MultiAff.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace molly;
using namespace std;


static cl::opt<string> MollyShape("shape", cl::desc("Molly - MPI cartesian grid shape"));




char MollyContextPass::ID;


MollyContextPass::MollyContextPass() : ModulePass(ID) {
  context = MollyContext::create(NULL); //TODO: Who owns it?

  string shape = MollyShape;
  SmallVector<StringRef, 4> shapeLengths;
  StringRef(shape).split(shapeLengths, "x");

  for (auto it = shapeLengths.begin(), end = shapeLengths.end(); it != end; ++it) {
    auto slen = *it;
    unsigned len;
    auto retval = slen.getAsInteger(10, len);
    assert(retval==false);
    clusterLengths.push_back(len);
  }
  clusterShape = getIslContext()->createRectangularSet(clusterLengths);
}


bool MollyContextPass::runOnModule(Module &M) {
  auto &llvmCtx = M.getContext();
  context->setLLVMContext(&llvmCtx);

  return false;
}


isl::Space MollyContextPass::getClusterSpace() {
  auto result = getIslContext()->createSetSpace(0, clusterLengths.size());
  return result;
}


isl::Ctx *MollyContextPass::getIslContext() {
  return context->getIslContext();
}


int MollyContextPass::getClusterDims() const {
	return clusterLengths.size();
}


int MollyContextPass::getClusterSize() const {
	int result = 1;
	for (auto it = clusterLengths.begin(), end = clusterLengths.end(); it != end; ++it) {
		auto len = *it;
		result *= len;
	}
	assert(result >= 1);
	return result;
}

int MollyContextPass::getClusterLength(int d) const {
	assert(d >= 0);
	if (d < clusterLengths.size()) {
		return clusterLengths[d];
	}
	return 1;
}

isl::MultiAff MollyContextPass:: getMasterRank() {
  auto islctx = getIslContext();
  auto nDims = getClusterDims();

  return isl::MultiAff::createZero(getClusterSpace());
}


static RegisterPass<MollyContextPass> FieldAnalysisRegistration("molly-context", "Molly - Context", false, true);
