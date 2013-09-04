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
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>


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


isl::Space ScopEditor::getParamSpace() {
  return enwrap(scop->getContext()).getSpace().params();
}
#if 0
unsigned ScopEditor::getNumParamDims() {
  return getParamSpace().getParamDimCount();
}
llvm::Value *ScopEditor::getParamDimValue(unsigned pos) {
  auto id = getParamSpace().getParamDimId(pos);
  auto scev = id.getUser<const SCEV*>();
}
isl::Id ScopEditor::getParamDimId(unsigned pos) {
  return getParamSpace().getParamDimId(pos);
}
#endif

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


/// Create a new natural loop. It will look like:
///  LoopEntry  
///      |
///  LoopHeader <-
///   |     |     \
///   | LoopBody   |
///   |     |      |
///   |    LoopFooter
///   |
/// LoopExit
static BasicBlock *newLoop(Function *func, Value *nIterations, BasicBlock *&entryBB, BasicBlock *&exitBB, Pass *pass, Loop *parentLoop, /*out*/Loop *&loop, Region *parentRegion, /*out*/Region *&region) {
  auto &llvmContext = func->getContext();
  auto intTy = nIterations->getType();
  assert(!loop);
  assert(!region);

  auto entryCreated = false;
  if (!entryBB) {
    entryBB = BasicBlock::Create(llvmContext, "LoopEntry", func);
    entryCreated = true;
  }
  auto headerBB = BasicBlock::Create(llvmContext, "LoopHeader", func);
  auto bodyBB = BasicBlock::Create(llvmContext, "LoopBody", func);
  auto footerBB = BasicBlock::Create(llvmContext, "LoopFooter", func);
  auto exitCreated = false;
  if (!exitBB) {
    exitBB = BasicBlock::Create(llvmContext, "LoopExit", func);
    exitCreated = true;
  }


  // LoopEntry (also called preheader)
  if (auto term = entryBB->getTerminator())
    term->eraseFromParent();
  BranchInst::Create(headerBB, entryBB);

  // LoopHeader
  auto indvar = PHINode::Create(intTy, 2, "indvar", headerBB);
  indvar->addIncoming(Constant::getNullValue(intTy), entryBB);
  auto cmp = new ICmpInst(*headerBB, ICmpInst::ICMP_SLE, indvar, nIterations, "indvarcmp");
  BranchInst::Create(bodyBB, exitBB, cmp, headerBB);

  // LoopBody
  // Caller is meant to insert something here
  BranchInst::Create(footerBB, bodyBB);

  // LoopFooter
  auto indvarinc = BinaryOperator::CreateNSWAdd(indvar, ConstantInt::get(intTy, 1), "indvarinc", footerBB);
  BranchInst::Create(headerBB, footerBB); // Backedge
  indvar->addIncoming(indvarinc, footerBB);

  // LoopExit
  if (exitCreated) {
    new UnreachableInst(llvmContext, exitBB);
  }


  RegionInfo *RI = nullptr;
  LoopInfo *LI = nullptr;
  DominatorTree *DT = nullptr; 
  if (pass) {
    RI = pass->getAnalysisIfAvailable<RegionInfo>();
    LI = pass->getAnalysisIfAvailable<LoopInfo>();
    DT = pass->getAnalysisIfAvailable<DominatorTree>();
    assert(!RI == !DT);
  }

  if (DT) {
    SmallVector<DomTreeNode *, 8> OldChildren;
    auto entryNode = DT->getNode(entryBB);
    if (entryNode) {
      for (DomTreeNode::iterator I = entryNode->begin(), E = entryNode->end(); I != E; ++I)
        OldChildren.push_back(*I);
    } else { 
      entryNode = DT->addNewBlock(entryBB, DT->getRoot());
    }

    // What the newly created BBs domainate
    auto headerNode = DT->addNewBlock(headerBB, entryBB);
    DT->addNewBlock(bodyBB, headerBB);
    DT->addNewBlock(footerBB, bodyBB);
    
    auto exitNode = DT->getNode(exitBB);
    if (exitNode) {
      // exitBB is now below headerBB
      if (DT->findNearestCommonDominator(exitBB, entryBB) == entryBB)
        DT->changeImmediateDominator(exitBB, headerBB);
    } else {
      exitNode = DT->addNewBlock(exitBB, headerBB);
    }

    // Append everything that followed the entryNode to the headerNode
    for (auto I = OldChildren.begin(), E = OldChildren.end(); I != E; ++I) { 
      auto oldChild = *I; 
      //if (oldChild != exitNode)
        DT->changeImmediateDominator(oldChild, headerNode);
    }

    DT->verifyAnalysis();
  }

  if (RI) {
    Region *commonRegion = nullptr;
    if (!entryCreated && !entryCreated)
      commonRegion = RI->getCommonRegion(entryBB, exitBB);
    else if (!entryCreated)
      commonRegion = RI->getRegionFor(entryBB);
    else if (!exitCreated)
      commonRegion = RI->getRegionFor(exitBB);
    else
      commonRegion = RI->getTopLevelRegion();
    assert(!parentRegion || parentRegion  == commonRegion);
    if (!parentRegion)
      parentRegion = commonRegion;

    region = new Region(entryBB, exitBB, RI, DT, parentRegion);
    //parentRegion->addSubRegion(region);
    if (entryCreated)
      RI->setRegionFor(entryBB, region);
    RI->setRegionFor(headerBB, region);
    RI->setRegionFor(bodyBB, region);
    RI->setRegionFor(footerBB, region);
    if (exitCreated)
      RI->setRegionFor(exitBB, region);

    RI->verifyAnalysis();
  }

  if (LI) {
    auto commonLoop = LI->getLoopFor(entryBB);
    assert(!parentLoop || parentLoop==commonLoop);
    if (!parentLoop)
      parentLoop = commonLoop;

    loop = new Loop();
    //loop->setParentLoop(parentLoop);
    parentLoop->addChildLoop(loop);
    addBasicBlockToLoop(headerBB, loop, LI);
    addBasicBlockToLoop(bodyBB, loop, LI);
    addBasicBlockToLoop(footerBB, loop, LI);

    LI->verifyAnalysis();
  }

  return bodyBB;
}


static BasicBlock* insertLoop(BasicBlock *into, Instruction *insertBefore, Value *nIterations, Pass *pass,Loop *parentLoop, /*out*/Loop *&loop, Region *parentRegion, /*out*/Region *&region) {
  auto func = getParentFunction(into);

  BasicBlock *entryBB = nullptr;
  BasicBlock *exitBB = nullptr;
  if (insertBefore) {
    exitBB = SplitBlock(into, insertBefore, pass);
    entryBB = into;
  } else {
    entryBB = into;
    auto term = cast<BranchInst>(into->getTerminator());
    assert(term->isUnconditional());
    exitBB = term->getSuccessor(0);
  }

  return newLoop(into->getParent(), nIterations, entryBB, exitBB, pass, parentLoop, loop, parentRegion, region); 
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
  auto junctionBB = BasicBlock::Create(llvmContext, "dummyForScopStmt", function, entryBB);
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






/// { (i,j) -> (0, i, 0, j, 4) }, 2, -1 -> { (i, j) -> (0, i, -1,   0, 0) }
/// { (i,j) -> (0, i, 0, j, 4) }, 2, +1 -> { (i, j) -> (0, i, +1,   0, 0) }
/// { (i,j) -> (0, i, 0, j, 4) }, 3, -1 -> { (i, j) -> (0, i,  0, j-1, 0) }
/// { (i,j) -> (0, i, 0, j, 4) }, *,  0 -> { (i, j) -> (0, i,  0,   j, 4) }
static isl::Map relativeScatter(const isl::Map &modelScatter,  unsigned atLevel, int relative) {
  if (relative == 0)
    return modelScatter;

  auto islctx = modelScatter.getCtx();
  auto origSpace = modelScatter.getSpace();

  auto nPrefixDims = atLevel;
  auto nSuffixDims = modelScatter.getOutDimCount()-atLevel-1;
  //auto scatter = modelScatter.projectOut(isl_dim_out, atLevel + 1, nSuffixDims);
  //scatter.addDims_inplace(isl_dim_out, nSuffixDims);
  auto result = modelScatter.resetTupleId(isl_dim_out);

  auto scatterSpace = result.getRangeSpace();
  auto mapSpace = isl::Space::createMapFromDomainAndRange(scatterSpace, scatterSpace);
  auto map = mapSpace.createZeroMultiAff();

  for (auto i = 0; i < nPrefixDims; i+=1) {
    map.setAff_inplace(i, scatterSpace.createVarAff(isl_dim_in, i));
  }
  map.setAff_inplace(nPrefixDims, scatterSpace.createVarAff(isl_dim_in, nPrefixDims) + relative);
  // Rest remains zero

  result.applyRange_inplace(map);
  result.cast_inplace(origSpace);
  return result;
}


StmtEditor StmtEditor::createStmt(const isl::Set &newdomain, const isl::Map &subscatter_, const isl::Map &where, const std::string &name) {
  auto &llvmContext = getLLVMContext();
  auto function = getParentFunction();
  auto modelLoopNests = stmt->getLoopNests();
  auto scop = getParentScop();
  auto region = getParentRegion(this->stmt);
  auto intTy = Type::getInt32Ty(llvmContext);

  auto model = this;
  auto modelDomain = getIterationDomain().resetSpace(isl_dim_set);
  auto subdomain = newdomain.projectOut(isl_dim_set, modelDomain.getDimCount(), newdomain.getDimCount() - modelDomain.getDimCount());
  assert(subdomain <= modelDomain);
  assert(newdomain <= subscatter_.getDomain());
  auto subscatter = subscatter_.resetSpace(isl_dim_in);

  auto nModelDims = modelDomain.getDimCount();
  auto nNewDims = newdomain.getDimCount();
  auto innermostBody = getBasicBlock();
  auto innermostRegion = const_cast<Region*>(stmt->getRegion());
  auto innermostLoop = modelLoopNests[modelLoopNests.size()-1];
  BasicBlock *newStmtBB = nullptr;
  assert(modelLoopNests.size() == nModelDims);

  SmallVector<Loop*, 4> nests;
  nests.reserve(nNewDims);

  for (auto i = 0; i < nModelDims; i+=1) {
    nests.push_back(modelLoopNests[i]);
  }

  // the new domain has more dimensions than model's
  // The caller wants us to insert some additional loops
  for (auto i = nModelDims; i < nNewDims; i+=1) {
    Loop *loop = nullptr;
    Region *region = nullptr;
    innermostBody = insertLoop(innermostBody, nullptr, ConstantInt::get(intTy, 3), pass, innermostLoop, loop, innermostRegion, region);
    newStmtBB = innermostBody;
    innermostRegion = region;
    innermostLoop = loop;

    nests.push_back(loop);
  }

  if (!newStmtBB) {
    newStmtBB = BasicBlock::Create(llvmContext, name, function);
    DefaultIRBuilder builder(newStmtBB);
    builder.CreateUnreachable();

    assert(pass);
    auto RI = pass->getAnalysisIfAvailable<RegionInfo>();
    if (RI)
      RI->setRegionFor(newStmtBB, region);
  }

  auto newStmt = new ScopStmt(scop, newStmtBB, name, region, nests, newdomain.takeCopy(), subscatter.takeCopy());
  auto newWhere = where.setInTupleId(enwrap(stmt->getDomainId()));
  stmt->setWhereMap(newWhere.takeCopy());
  scop->addScopStmt(newStmt);
  return StmtEditor(newStmt);
}


#if 0
StmtEditor ScopEditor::createStmtRelative(polly::ScopStmt *model, const isl::Set &subdomain, unsigned atLevel, int relative, const isl::Map &where, const std::string &name) {
  auto modelDomain = enwrap(model->getDomain());
  assert(modelDomain >= subdomain);

  auto modelScatter = enwrap(model->getScattering());


  auto domain = enwrap(model->getDomain());
  auto modelScatter = enwrap(model->getScattering()).intersectDomain(domain);

  auto subscatter = relativeScatter(modelScatter, atLevel, relative);

  auto &llvmContext = getLLVMContext();
  auto function = getParentFunction();
  auto loopNests = model->getLoopNests();

  auto bb = BasicBlock::Create(llvmContext, name, function);
  DefaultIRBuilder builder(bb);
  builder.CreateUnreachable();

  auto stmt = new ScopStmt(scop, bb, name, nullptr, loopNests, domain.take(), newScatter.take());
  auto newWhere = where.setInTupleId(enwrap(stmt->getDomainId()));
  stmt->setWhereMap(newWhere.takeCopy());
  scop->addScopStmt(stmt);
  return StmtEditor(stmt);
}


static isl::Map computeRelativeScatter(const isl::Map &scatter, const isl::Map subscatter, int relative) {
  auto scatterSpace = scatter.getSpace();
  auto nScatterDims = scatter.getOutDimCount();
  auto subscatterSpace = subscatter.getRangeSpace();
  auto nSubscatterDims = subscatter.getOutDimCount();
  assert(nScatterDims >= nSubscatterDims); 

  auto resultSpace = isl::Space::createMapFromDomainAndRange(subscatter.getDomainSpace(), scatterSpace);
  auto mapSpace = isl::Space::createMapFromDomainAndRange(subscatterSpace, scatterSpace);
  isl::Map newScatter;
  if (nScatterDims == nSubscatterDims) {
    isl::PwMultiAff origin;
    if (relative > 0)
      origin = scatter.lexmaxPwMultiAff();
    else if (relative < 0)
      origin = scatter.lexminPwMultiAff();
    else
      return scatter; // Order doesn't seem to matter

    auto originMPA = origin.toMultiPwAff();
    auto leastmostPA = originMPA[nSubscatterDims-1];
    auto relativePA = leastmostPA + relative;
    auto relativeMPA = originMPA.setPwAff(nSubscatterDims-1, relativePA);
    auto result = relativeMPA.toMap();
    assert(result.matchesMapSpace(resultSpace));
    return result;
  } else {
    auto cutscatter = scatter.projectOut(isl_dim_out, nSubscatterDims+1, nScatterDims-nSubscatterDims-1);
    isl::PwMultiAff origin;
    if (relative > 0)
      origin = cutscatter.lexmaxPwMultiAff();
    else if (relative < 0)
      origin = cutscatter.lexminPwMultiAff();
    else 
      return scatter; // Execut in parallel/order unimportant
    assert(origin.getDomain() >= cutscatter.getDomain() && "No undefined areas, for instance because of unboundedness");

    auto originMPA = origin.toMultiPwAff();
    auto leastmostPA = originMPA[originMPA.getOutDimCount()-1];
    auto relativePA = leastmostPA + relative;

    auto resultMPA = resultSpace.createZeroMultiPwAff();
    for (auto i = 0; i < nSubscatterDims; i+=1) {
      resultMPA.setPwAff_inplace(i, originMPA[i]);
    }
    resultMPA.setPwAff_inplace(nSubscatterDims, relativePA);
    auto result = resultMPA.toMap();
    result.cast_inplace(resultSpace);
    return result;
  }
}


StmtEditor ScopEditor::createStmtRelative(polly::ScopStmt *model, const isl::Map &subscatter, int relative, const isl::Map &where, const std::string &name) {
  auto domain = enwrap(model->getDomain());
  auto scatter = enwrap(model->getScattering()).intersectDomain(domain);

  auto newScatter = computeRelativeScatter(scatter, subscatter, relative);

  auto &llvmContext = getLLVMContext();
  auto function = getParentFunction();
  auto loopNests = model->getLoopNests();

  auto bb = BasicBlock::Create(llvmContext, name, function);
  DefaultIRBuilder builder(bb);
  builder.CreateUnreachable();

  auto stmt = new ScopStmt(scop, bb, name, nullptr, loopNests, domain.take(), newScatter.take());
  auto newWhere = where.setInTupleId(enwrap(stmt->getDomainId()));
  stmt->setWhereMap(newWhere.takeCopy());
  scop->addScopStmt(stmt);
  return StmtEditor(stmt);
}


StmtEditor ScopEditor::createStmtBefore(polly::ScopStmt *model, const isl::Map &subscattering, const isl::Map &where, const std::string &name) {
  return createStmtRelative(model, subscattering, -1, where, name);
}


StmtEditor ScopEditor::createStmtAfter(polly::ScopStmt *model, const isl::Map &subscattering, const isl::Map &where, const std::string &name) {
  return createStmtRelative(model, subscattering, +1, where, name);
}
#endif

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
  StmtEditor stmtEditor(stmt);
  replaceDomainWhere.setInTupleId_inplace(stmtEditor.getDomainTupleId());
  stmtEditor.setWhere(replaceDomainWhere.move());
  //stmt->setWhereMap(replaceDomainWhere.take());
  //auto result = createStmt(createdModelDomain.move(), scattering.move(), replaceDomainWhere.move(), name);
  model->setDomain(newModelDomain.take());

  return StmtEditor(stmt);
}


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
  assert(where.getDomain().isSupersetOf(getIterationDomain()));
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


void StmtEditor::remove() {
  // May also remove from the parent SCoP
  stmt->setDomain(getDomainSpace().emptySet().take());
}


void StmtEditor::addWriteAccess(llvm::StoreInst *instr, FieldVariable *fvar, isl::Map &&accessRelation) {
  assert(accessRelation.matchesMapSpace(getDomainSpace(), fvar->getFieldType()->getIndexsetSpace()));
  stmt->addAccess(MemoryAccess::MUST_WRITE, fvar->getVariable(), accessRelation.take(), instr);
}
