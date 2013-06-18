#include "MollyContextPass.h"

#include "MollyContext.h"
#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include "ClusterConfig.h"

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <polly/PollyContextPass.h>

using namespace llvm;
using namespace molly;
using namespace std;
using namespace polly;


static cl::opt<string> MollyShape("shape", cl::desc("Molly - MPI cartesian grid shape"));



void MollyContextPass::initClusterConf() {
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

  clusterConf.reset(new ClusterConfig(getIslContext()));
  clusterConf->setClusterLengths(clusterLengths);
}


MollyContextPass::MollyContextPass() : ModulePass(ID) {
  DEBUG(llvm::dbgs() << "Creating a MollyContextPass\n");
  islctx = nullptr;
  llvmContext = nullptr;
  fieldDetection = nullptr;
  //context = MollyContext::create(NULL); 
}


MollyContextPass::~MollyContextPass() {
  //DEBUG(llvm::dbgs() << "Destroying a MollyContextPass\n");
  releaseMemory();
}


    void MollyContextPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
      AU.addRequired<PollyContextPass>();
      AU.setPreservesAll();
    }


void MollyContextPass::releaseMemory() {
  //DEBUG(llvm::dbgs() << "Releasing a MollyContextPass\n");
  clusterConf.reset();
}


bool MollyContextPass::runOnModule(Module &M) {
  llvmContext = &M.getContext();
  auto pollyCtx = &getAnalysis<PollyContextPass>();
  islctx = isl::Ctx::wrap(pollyCtx->getIslCtx());
  //context->setLLVMContext(&llvmCtx);
  initClusterConf();

  return false;
}


isl::Space MollyContextPass::getClusterSpace() {
  return clusterConf->getClusterSpace();
}

isl::BasicSet MollyContextPass::getClusterShape() { 
  return clusterConf->getClusterShape(); 
}


llvm::ArrayRef<unsigned> MollyContextPass::getClusterLengths() { 
  return clusterConf->getClusterLengths(); 
}


int MollyContextPass::getClusterDims() const {
  return clusterConf->getClusterDims();
}


int MollyContextPass::getClusterSize() const {
  return clusterConf->getClusterSize();
}


int MollyContextPass::getClusterLength(int d) const {
  return clusterConf->getClusterLength(d);
}


isl::MultiAff MollyContextPass:: getMasterRank() {
  auto islctx = getIslContext();
  auto nDims = getClusterDims();

  return isl::MultiAff::createZero(getClusterSpace());
}


char molly::MollyContextPass::ID;
char &molly::MollyContextPassID = MollyContextPass::ID;
static RegisterPass<MollyContextPass> MollyContextPassRegistration("molly-context", "Molly - Context");
