#include "ScopEditor.h"

#include <polly/ScopInfo.h>
#include "islpp/Set.h"
#include "islpp/Map.h"
#include "ScopUtils.h"
#include "MollyUtils.h"
#include <llvm/IR/Module.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


llvm::LLVMContext &ScopEditor::getLLVMContext() {
  return getParentModule()->getContext();
}


llvm::Module *ScopEditor::getParentModule() {
  return molly:: getParentModule(scop);
}


llvm::Function *ScopEditor::getParentFunction() { 
  return molly::getParentFunction(scop); 
}


ScopEditor ScopEditor::newScop(llvm::Instruction *insertBefore, llvm::FunctionPass *p) {
  auto beforeBB  = insertBefore->getParent();
  auto afterBB = SplitBlock(beforeBB, insertBefore, p);

  // Insert an empty BB into the middle
  auto scopBB = SplitBlock(beforeBB, beforeBB->getTerminator(), p);

  auto dt = &p->getAnalysis<llvm::DominatorTree>();
  auto ri = &p->getAnalysis<llvm::RegionInfo>();
  auto region = new Region(scopBB, scopBB, ri, dt); // Need to add to RegionInfo's list off regions?

  auto scop = Scop::create(region);
  return ScopEditor(scop);
}


void ScopEditor::getParamsMap(std::map<isl_id *, llvm::Value *> &params, ScopStmt *stmt) {
  params.clear(); 
  //SCEVExpander expander(

  for (auto paramEv : scop->getParams()) {
    auto paramId = scop->getIdForParam(paramEv);
    assert(!params.count(paramId));

    //TODO: Use SCEVExpander to get llvm::Value
    auto unk = cast<SCEVUnknown>(paramEv);
    params[paramId] = unk->getValue();
  }

  //TODO: Also add ids for induction variables
} 

polly::ScopStmt *ScopEditor::createBlock(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name) {
  assert(isSubset(domain, scattering.getDomain()));
  assert(isSubset(domain, where.getDomain()));

  auto &llvmContext = getLLVMContext();
  auto function = getParentFunction();

  auto bb = BasicBlock::Create(llvmContext, name, function);

  auto stmt = new ScopStmt(scop, bb, name, nullptr, ArrayRef<Loop*>(), domain.take(), scattering.take());
  stmt->setWhereMap(where.take());
  return stmt;
}


StmtEditor ScopEditor::createStmt(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name) {
  assert(isSubset(domain, scattering.getDomain()));
  assert(isSubset(domain, where.getDomain()));

  auto &llvmContext = getLLVMContext();
  auto function = getParentFunction();

  auto bb = BasicBlock::Create(llvmContext, name, function);

  auto stmt = new ScopStmt(scop, bb, name, nullptr, ArrayRef<Loop*>(), domain.take(), scattering.take());
  stmt->setWhereMap(where.take());
  scop->addScopStmt(stmt);
  return StmtEditor(stmt);
}


StmtEditor ScopEditor::replaceStmt(polly::ScopStmt *model, isl::Map &&replaceDomainWhere, const std::string &name) {
  auto scop = model->getParent();
  auto origDomain = getIterationDomain(model);
  auto origWhere = getWhereMap(model);
  origWhere.intersectDomain_inplace(origDomain);
  auto scattering = getScattering(model);

  assert(isSubset(replaceDomainWhere, origWhere));
  auto newModelDomainWhere = origWhere.subtract(replaceDomainWhere);
  auto newModelDomain = newModelDomainWhere.getDomain();
  auto createdModelDomain = replaceDomainWhere.getDomain();

  auto result = createStmt(createdModelDomain.move(),  scattering.move(), replaceDomainWhere.move(), name);
  model->setDomain(newModelDomain.take());
  return result;
}


polly::ScopStmt *molly::replaceScopStmt(polly::ScopStmt *model, llvm::BasicBlock *bb, const std::string &baseName, isl::Map &&replaceDomainWhere) {
  auto scop = model->getParent();
  auto origDomain = getIterationDomain(model);
  auto origWhere = getWhereMap(model);
  origWhere.intersectDomain_inplace(origDomain);
  auto scattering = getScattering(model);

  assert(isSubset(replaceDomainWhere, origWhere));
  auto newModelDomainWhere = origWhere.subtract(replaceDomainWhere);
  auto newModelDomain = newModelDomainWhere.getDomain();
  auto createdModelDomain = replaceDomainWhere.getDomain();

  auto result = createScopStmt(
    scop,
    bb,
    model->getRegion(),
    baseName,
    model->getLoopNests(),
    createdModelDomain.move(),
    scattering.move()
    );
  model->setDomain(newModelDomain.take());
  return result;
}


polly::ScopStmt *molly::createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering) {
  return new ScopStmt(parent, bb, baseName, region, sourroundingLoops, domain.take(), scattering.take());
}


llvm::BasicBlock * molly::StmtEditor::getBasicBlock() {
  return stmt->getBasicBlock();
}


llvm::TerminatorInst *StmtEditor::getTerminator() {
  //TODO: Think about createing a terminator in createStmt() 
  auto bb = getBasicBlock();
  auto term = bb->getTerminator();
  if (term)
    return term;
  return new UnreachableInst(bb->getContext(), bb);
}


std::vector<llvm::Value*> StmtEditor:: getDomainValues() {
  //FIXME: Fill!
  std::vector<llvm::Value*> result;
  return result;
}
