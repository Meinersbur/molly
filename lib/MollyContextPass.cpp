#include "MollyContextPass.h"

#include "MollyContext.h"

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


static RegisterPass<MollyContextPass> FieldAnalysisRegistration("molly-context", "Molly - Context", false, true);
