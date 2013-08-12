#include "ScopEditor.h"

#include <polly/ScopInfo.h>
#include "islpp/Set.h"
#include "islpp/Map.h"
#include "ScopUtils.h"
#include "MollyUtils.h"
#include <llvm/IR/Module.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Analysis/LoopInfo.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


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


llvm::Function *ScopEditor::getParentFunction() {
  return molly::getParentFunction(scop);
}


llvm::Module *ScopEditor::getParentModule() {
  return getParentFunction()->getParent();
}


llvm::LLVMContext &ScopEditor::getLLVMContext() {
  return getParentFunction()->getContext();
}


isl::Id ScopEditor::getScatterTupleId() {
  return getScatterTuple(scop);
}


isl::Space ScopEditor::getScatterSpace() {
  return molly::getScatteringSpace(scop);
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

#if 0
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
#endif


static void addBasicBlockToLoop(BasicBlock *headerBB, Loop* loop, LoopInfo *LI) {
  if (LI) {
    loop->addBasicBlockToLoop(headerBB, LI->getBase());
  } else {
    auto L = loop; 
    while (L) {
      L->addBlockEntry(headerBB);
      L = L->getParentLoop();
    }
  }
}

StmtEditor ScopEditor::createStmt(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name) {
  assert(isSubset(domain, scattering.getDomain()));
  assert(isSubset(domain, where.getDomain()));

  auto &llvmContext = getLLVMContext();
  auto LI = getAnalysisIfAvailable<LoopInfo>();
  auto function = getParentFunction();
  auto intTy = Type::getInt32Ty(llvmContext);
  auto boolTy = Type::getInt1Ty(llvmContext);

  auto nDomainDims = domain.getDimCount();
  auto nLoopNests = nDomainDims; // Correct?
  SmallVector<Loop *,4> loopNests;
  SmallVector<BasicBlock *,4> footerBBs;
  SmallVector<BasicBlock *,4> exitBBs;
  for (auto i = 0; i < nLoopNests; i+=1) {
    auto loop = new Loop();
    if (!loopNests.empty()) {
      auto parenLoop =  loopNests.back();
      parenLoop->addChildLoop(loop);
    }

    // Loop header
    auto headerBB = BasicBlock::Create(llvmContext, "LoopHeader", function);
    addBasicBlockToLoop(headerBB, loop, LI);
    //DefaultIRBuilder headerBuilder(headerBB);

    // Loop footer
    auto footerBB = BasicBlock::Create(llvmContext, "LoopFooter", function);
    addBasicBlockToLoop(footerBB, loop, LI);
    footerBBs.push_back(footerBB);

    // Loop exit
    auto exitBB = BasicBlock::Create(llvmContext, "LoopExit", function);
    addBasicBlockToLoop(exitBB, loop, LI); // Or maybe add to parent loop?
    exitBBs.push_back(footerBB);

    loopNests.push_back(loop);
  }

  auto entryBB = &function->getEntryBlock();
  auto junctionBB = BasicBlock::Create(llvmContext, "dummyForScopStmt",function, entryBB);
  DefaultIRBuilder junctionBuilder(junctionBB);
  junctionBuilder.CreateCondBr(ConstantInt::get(boolTy, 0), loopNests[0]->getHeader(), entryBB);
  auto incoming = junctionBB;
  auto outgoing = BasicBlock::Create(llvmContext, "ScopStmtExit", function);
  DefaultIRBuilder outgoingBuilder(outgoing);
  outgoingBuilder.CreateUnreachable();

  auto stmtBB = BasicBlock::Create(llvmContext, name, function);


  for (auto i = 0; i < nLoopNests; i+=1) {
    auto parent = i > 0 ? loopNests[i-1] : nullptr;

    auto childHeaderBB = i == nDomainDims-1 ? stmtBB : loopNests[i+1]->getHeader();
    auto childFooterBB = i == nDomainDims-1 ? stmtBB : footerBBs[i+1];
    auto loop = loopNests[i]; 
    auto loopHeaderBB = loop->getHeader();
    auto loopFooterBB = footerBBs[i];
    auto loopExitBB = exitBBs[i];

    auto headerBB = loop->getHeader();
    DefaultIRBuilder headerBuilder(headerBB); 
    auto indvar = headerBuilder.CreatePHI(intTy, 2, "indvar");
    indvar->addIncoming(ConstantInt::get(intTy, 0), incoming);
    auto inbound = headerBuilder.CreateICmpSLE(indvar, ConstantInt::get(intTy, 100/*arbitrary*/), "boundtest");
    headerBuilder.CreateCondBr(inbound, childHeaderBB, loopExitBB);

    DefaultIRBuilder footerBuilder(loopFooterBB); 
    auto indvarnext = footerBuilder.CreateAdd(indvar, ConstantInt::get(intTy, 1), "indvarnext");
    footerBuilder.CreateBr(headerBB);
    indvar->addIncoming(indvarnext, loopFooterBB);

    DefaultIRBuilder exitBuilder(loopExitBB); 
    exitBuilder.CreateBr(outgoing);

    incoming = loopHeaderBB;
    outgoing = loopFooterBB;
  }
  DefaultIRBuilder stmtBuilder(stmtBB);
  stmtBuilder.CreateBr(outgoing);

  if (!loopNests.empty() && LI) {
    LI->addTopLevelLoop(loopNests[0]);
  }


  auto stmt = new ScopStmt(scop, stmtBB, name, nullptr, loopNests, domain.take(), scattering.take());
  where.setInTupleId_inplace(enwrap(stmt->getDomainId()));
  stmt->setWhereMap(where.take());
  scop->addScopStmt(stmt);
  return StmtEditor(stmt);
}


StmtEditor ScopEditor::replaceStmt(polly::ScopStmt *model, isl::Map &&replaceDomainWhere, const std::string &name) {
  auto scop = model->getParent();
  auto function = getParentFunction();
  assert(scop == model->getParent());

  auto &llvmContext = getLLVMContext();
  auto origDomain = getIterationDomain(model);
  auto origWhere = getWhereMap(model);
  origWhere.intersectDomain_inplace(origDomain);
  auto scattering = getScattering(model);

  assert(isSubset(replaceDomainWhere, origWhere));
  auto newModelDomainWhere = origWhere.subtract(replaceDomainWhere);
  auto newModelDomain = newModelDomainWhere.getDomain();
  auto createdModelDomain = replaceDomainWhere.getDomain();

  auto stmtBB = BasicBlock::Create(llvmContext, name, function);
  auto stmt = new ScopStmt(scop, stmtBB, name, nullptr, model->getLoopNests(), createdModelDomain.take(), scattering.take());
  stmt->setWhereMap(replaceDomainWhere.take());
  //auto result = createStmt(createdModelDomain.move(), scattering.move(), replaceDomainWhere.move(), name);
  model->setDomain(newModelDomain.take());

  return StmtEditor(stmt);
}


#if 0
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
#endif


polly::Scop *StmtEditor::getParentScop() {
  return stmt->getParent();
}


ScopEditor StmtEditor::getParentScopEditor() {
  return ScopEditor(getParentScop());
}


llvm::Function *StmtEditor::getParentFunction() {
  return molly::getParentFunction(stmt);
}


llvm::Module *StmtEditor::getParentModule() {
  return molly::getParentModule(stmt);
}


llvm::LLVMContext &StmtEditor::getLLVMContext() {
  return getParentFunction()->getContext();
}


llvm::BasicBlock * molly::StmtEditor::getBasicBlock() {
  return stmt->getBasicBlock();
}


isl::Space StmtEditor::getDomainSpace() {
  auto result = getIterationDomain().getSpace();
  return result;
}


isl::Id StmtEditor::getDomainTupleId() {
  auto result = getDomainSpace().getSetTupleId();
  return result;
}


isl::Space StmtEditor::getScatterSpace() {
  return getScattering().getRangeSpace();
}


isl::Set StmtEditor::getIterationDomain() {
  auto result = enwrap(stmt->getDomain());
  assert(result.getDimCount() >= 1);
  assert(result.hasTupleId());
  return result;
}


void StmtEditor::setIterationDomain(isl::Set &&domain) {
  assert(domain.getSpace().matchesSetSpace(getDomainSpace()));
  stmt->setDomain(domain.take());
}


isl::Map StmtEditor::getScattering() {
  auto result = enwrap(stmt->getScattering());
  assert(isSubset(getIterationDomain(), result.getDomain()));
  //result.intersectDomain(getIterationDomain()); // TODO: Remove for efficiency
  assert(result.getSpace().matchesMapSpace(getDomainSpace(),  ScopEditor(getParentScop()).getScatterSpace() ));
  return result;
}


void StmtEditor::setScattering(isl::Map &&scatter) {
  assert(scatter.getSpace().matchesMapSpace(getDomainSpace(), getParentScopEditor().getScatterTupleId()));
  stmt->setScattering(scatter.take());
}


isl::Map StmtEditor::getWhere() {
  auto result = enwrap(stmt->getWhereMap());
  //result.intersectDomain(getIterationDomain()); // TODO: Remove for efficiency
  return result;
}


void StmtEditor::setWhere(isl::Map &&where) {
  assert(where.getDomainSpace().matchesSetSpace(getDomainSpace()));
  assert(where.hasOutTupleId());
  stmt->setWhereMap(where.take());
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
