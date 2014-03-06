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

static BasicBlock *insertDedicatedBB(BasicBlock *into, Instruction *insertBefore, bool uniqueIncoming, Pass *pass, const Twine &dedicatedPostfix = ".dedicated", const Twine &contPostfix = ".cont");


static BasicBlock *SplitBlockWithName(BasicBlock *Old, Instruction *SplitPt, Pass *P, const Twine &postfix) {
  auto name = Old->getName();
  auto result = SplitBlock(Old, SplitPt, P);
  result->setName(name + postfix);
  return result;
}


/// Return the jump target BB if the instruction is an unconditional jump; nullptr otherwise
static BasicBlock *getUnconditionalJumpTarget(TerminatorInst *term) {
  auto br = dyn_cast<BranchInst>(term);
  if (br && br->isUnconditional()) {
    assert(br->getNumSuccessors() == 1);
    return br->getSuccessor(0);
  }
  return nullptr;
}


ScopEditor ScopEditor::newScop(isl::Ctx *ctx, llvm::Instruction *insertBefore, llvm::FunctionPass *p) {
  auto scopBB = insertDedicatedBB(nullptr, insertBefore, true, p, "");
  auto name = scopBB->getName();

  //auto exitBB = SplitBlockWithName(scopBB, scopBB->getTerminator(), p, ".endscop");
  //scopBB->setName(".enterscop");
  auto exitBB = getUnconditionalJumpTarget(scopBB->getTerminator());

  auto dtw = &p->getAnalysis<llvm::DominatorTreeWrapperPass>();
  auto dt = &dtw->getDomTree();
  auto ri = &p->getAnalysis<llvm::RegionInfo>();
  auto prarentRegion = ri->getRegionFor(scopBB);
  assert(scopBB!=exitBB);
  auto region = new Region(scopBB, exitBB, ri, dt, prarentRegion); // Need to add to RegionInfo's list off regions?
  ri->setRegionFor(scopBB, region);
  //ri->setRegionFor(exitBB, region);

  // TODO: Do not forget to make ScopInfo detect this
  // Currently the caller must do this

  auto SE = &p->getAnalysis<ScalarEvolution>();
  auto scop = Scop::create(ctx->keep(), region, SE);
  return ScopEditor(scop, p);
}


llvm::Function *ScopEditor::getParentFunction() {
  return molly::getFunctionOf(scop);
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
  } else if (auto term = entryBB->getTerminator()) {
    assert(getUnconditionalJumpTarget(term)== exitBB); // Also, exitBB may not have PHI nodes depending on entryBB
    term->eraseFromParent();
  }
  auto headerBB = BasicBlock::Create(llvmContext, "LoopHeader", func);
  auto bodyBB = BasicBlock::Create(llvmContext, "LoopBody", func);
  auto footerBB = BasicBlock::Create(llvmContext, "LoopFooter", func);
  auto exitCreated = false;
  if (!exitBB) {
    exitBB = BasicBlock::Create(llvmContext, "LoopExit", func);
    exitCreated = true;
  }

  // LoopEntry (also preheader)
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
    auto DTW = pass->getAnalysisIfAvailable<DominatorTreeWrapperPass>();
    DT = DTW ? &DTW->getDomTree() : NULL;
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

    // What the newly created BBs dominate
    auto headerNode = DT->addNewBlock(headerBB, entryBB);
    DT->addNewBlock(bodyBB, headerBB);
    DT->addNewBlock(footerBB, bodyBB);

    auto exitNode = DT->getNode(exitBB);
    if (exitNode) {
      // exitBB is now below headerBB
      if (exitNode->getIDom() == entryNode)
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

    //DT->verifyAnalysis();
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
    assert(!parentRegion || parentRegion == commonRegion);
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
    if (parentLoop) { // Otherwise, it's a top-level loop
      parentLoop->addChildLoop(loop);
    } else {
      LI-> addTopLevelLoop(loop);
    }
    addBasicBlockToLoop(headerBB, loop, LI);
    addBasicBlockToLoop(bodyBB, loop, LI);
    addBasicBlockToLoop(footerBB, loop, LI);

    LI->verifyAnalysis();
  }

  return bodyBB;
}


// The two output BB will be connected by an unconditional jump 
// If uniqueIncoming, the second BB will also have the first BB a predecessor
bool molly::splitBlockIfNecessary(BasicBlock *into, Instruction *insertBefore, bool uniqueIncoming, BasicBlock* &before, BasicBlock* &after, Pass *pass) {
  if (!into)
    into = insertBefore->getParent();
  assert(!insertBefore || insertBefore->getParent() == into);
  auto func = getFunctionOf(into);

  auto term = into->getTerminator();
  assert(term);
  auto atBegin = (&into->front() == insertBefore) || (!insertBefore && into->size()==1) || (insertBefore==term && into->size()==1);
  if (atBegin) {
    auto pred = into->getSinglePredecessor();
    if (pred && getUnconditionalJumpTarget(pred->getTerminator()) == into) {
      //return splitBlockIfNecessary(pred, nullptr, uniqueIncoming, pass);
      before = pred;
      after = into;
      return false;
    }
  }

  bool atEnd = !insertBefore || (term == insertBefore && getUnconditionalJumpTarget(term));
  if (atEnd) {
    auto succ = getUnconditionalJumpTarget(term);
    if (!uniqueIncoming || succ->getSinglePredecessor()) {
      before = into;
      after = succ;
      return false;
    }
  }

  auto succ = SplitBlock(into, insertBefore, pass);
  before = into;
  after = succ;
  return true;
}


static BasicBlock * insertNewBlockBefore(const Twine & name, BasicBlock * after, Pass * pass) {
  //FIXME: Need to change PHINodes as in BasicBlock::splitBasicBlock

  // Insert the new BB between p.first && p.second
  // We cannot use SplitBlock, this would create a new block after after and move all instructions there; existing references to after would therefore refer the empty dedicated BasicBlock instead
  auto &llvmContext = after->getContext();
  auto dedicated = BasicBlock::Create(llvmContext, name, after->getParent());
  BranchInst::Create(after, dedicated);

  // Change everything pointing to after to point to dedicated
  SmallVector<TerminatorInst *, 4>  preds;
  SmallVector<int, 4>  predsOpNo;
  //SmallVector<Use, 4>  predUses;
  for (auto pred = pred_begin(after), end = pred_end(after); pred!=end; ++pred) {
    auto &use = pred.getUse();
    //predUses.push_back(use);
    auto user = &*use;
    auto term = cast<TerminatorInst>(use.getUser());
    if (term->getParent()== dedicated)
      continue;

    preds.push_back(term);
    predsOpNo.push_back(pred.getOperandNo());
  }
  auto nUses = preds.size();
  for (auto i = nUses-nUses;i<nUses;i+=1) {
    auto opno = predsOpNo[i];
    auto term = preds[i];
    term->setOperand(opno, dedicated);
  }

  // Fixup the analyses.
  if (pass) {
    if (LoopInfo *LI = pass->getAnalysisIfAvailable<LoopInfo>()) {
      if (Loop *L = LI->getLoopFor(after))
        L->addBasicBlockToLoop(dedicated, LI->getBase());
    }

    if (auto RI = pass->getAnalysisIfAvailable<RegionInfo>()) {
      if (auto OldRegion = RI->getRegionFor(after)) 
        RI->setRegionFor(dedicated, OldRegion);
    }

    if (auto *DTW = pass->getAnalysisIfAvailable<DominatorTreeWrapperPass>()) {
      auto DT = &DTW->getDomTree();
      // dedicated dominates after
      if (DomTreeNode *afterNode = DT->getNode(after)) {
        DomTreeNode *dedicatedNode = DT->addNewBlock(dedicated, afterNode->getIDom()->getBlock());
        DT->changeImmediateDominator(after, dedicated);
      }

      //DT->verifyAnalysis();
    }
  }

  return dedicated;
}



/// Insert a new BB at the given location; guaranteed to be empty except with an unconditional jump terminator
static BasicBlock *insertDedicatedBB(BasicBlock *into, Instruction *insertBefore, bool uniqueIncoming/*currently ignored*/, Pass *pass, const Twine &dedicatedPostfix, const Twine &contPostfix) {
  if (!into)
    into = insertBefore->getParent();

  auto term = into->getTerminator();
  assert(term && "malformed BB");
  if (into->size()==1 && getUnconditionalJumpTarget(term)) {
    return into; // Already fulfills both conditions
  }

  auto name = into->getName();
  BasicBlock *before;
  BasicBlock *after;
  auto p = splitBlockIfNecessary(into, insertBefore, false, before, after, pass);
  assert(getUnconditionalJumpTarget(before->getTerminator()));

  // Does the first already fulfill the condition?
  if (before->size()==1) {
    if (p) after->setName(Twine(name) + contPostfix);
    return before;
  }

  if (p)
    after->setName(Twine(name) + dedicatedPostfix);

  // Does the second already fulfill the conditions?
  if (after->size()==1 && getUnconditionalJumpTarget(after->getTerminator()))
    return after;

  auto dedicated = insertNewBlockBefore(Twine(name) + dedicatedPostfix, after, pass);
  return dedicated;


  //auto last = SplitBlock(after, &after->front(), pass);
  //last->setName(Twine(name) + contPostfix);
  //return after;
}


static BasicBlock* insertLoop(BasicBlock *into, Instruction *insertBefore, Value *nIterations, Pass *pass,Loop *parentLoop, /*out*/Loop *&loop, Region *parentRegion, /*out*/Region *&region) {
  auto func = getFunctionOf(into);
  //viewRegionOnly(func);

  BasicBlock *entryBB;
  BasicBlock *exitBB;

  if (!insertBefore)
    insertBefore = into->getTerminator();

  RegionInfo *RI = nullptr;
  if (pass) {
    RI = pass->getAnalysisIfAvailable<RegionInfo>();
  }

  if (RI) {
    auto intoRegion = RI->getRegionFor(into);
    //assert(intoRegion == parentRegion);
    parentRegion = intoRegion;
  }

  entryBB = into;
  exitBB = SplitBlock(into, insertBefore, pass);
  //auto p = splitBlockIfNecessary(into, insertBefore, false, entryBB, exitBB,  pass);
  exitBB->setName(Twine(into->getName()) + ".loopexit");
  assert(!RI || RI->getRegionFor(into)==parentRegion);
  assert(!RI || RI->getRegionFor(exitBB)==parentRegion);

#if 0
  if (insertBefore) {
    exitBB = SplitBlock(into, insertBefore, pass);
    entryBB = into;
  } else {
    entryBB = into;
    auto term = cast<BranchInst>(into->getTerminator());
    assert(term->isUnconditional());
    exitBB = term->getSuccessor(0);
  }
#endif
  return newLoop(into->getParent(), nIterations, entryBB, exitBB, pass, parentLoop, loop, parentRegion, region); 
}


StmtEditor ScopEditor::createStmt(isl::Set domain, isl::Map scattering, isl::Map where, const std::string &name) {
  assert(isSubset(domain, scattering.getDomain()));
  assert(isSubset(domain, where.getDomain()));

  auto &llvmContext = getLLVMContext();
  auto LI = getAnalysisIfAvailable<LoopInfo>();
  auto function = getParentFunction();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto boolTy = Type::getInt1Ty(llvmContext);
  auto nDomainDims = domain.getDimCount();
  auto its = ConstantInt::get(intTy, 3); // dummy, Polly will determine loop iterations

  SmallVector<Loop*, 4> nests;
  nests.reserve(nDomainDims);

  // Loop analysis is necessary, otherwise insertLoop() will not be able to tell us the the Loop and therefore the LoopInductionVariable
  auto li = &pass->getAnalysis<LoopInfo>();

  auto innermostRegion = &scop->getRegion();
  auto innermostBody = innermostRegion->getEntry();
  auto innermostInsertion = innermostBody->getTerminator();
  Loop *innermostLoop = nullptr;
  for (auto i = nDomainDims-nDomainDims; i < nDomainDims; i+=1) {
    Loop *theLoop = nullptr;
    Region *newRegion = nullptr;
    auto newBB = insertLoop(innermostBody, innermostInsertion, its, pass, innermostLoop, theLoop, innermostRegion, newRegion);
    assert(theLoop);
    nests.push_back(theLoop);

    innermostLoop = theLoop;
    innermostRegion = newRegion;
    innermostBody = newBB;
    innermostInsertion = nullptr;
  }

#if 0
  auto nDomainDims = domain.getDimCount();
  auto nLoopNests = nDomainDims; // Correct?
  SmallVector<Loop *,4> loopNests;
  SmallVector<BasicBlock *,4> footerBBs;
  SmallVector<BasicBlock *,4> exitBBs;
  for (auto i = nLoopNests-nLoopNests; i < nLoopNests; i+=1) {
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
    auto exitBB = BasicBlock::Create(llvmContext, "LoopExxit", function);
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


  for (auto i = nLoopNests-nLoopNests; i < nLoopNests; i+=1) {
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
#endif

  auto stmt = new ScopStmt(scop, innermostBody, name, nullptr, nests, domain.take(), scattering.take());
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

  for (auto i = nPrefixDims-nPrefixDims; i < nPrefixDims; i+=1) {
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
  auto func = getParentFunction();
  auto modelLoopNests = stmt->getLoopNests();
  auto scop = getParentScop();
  auto parentRegion = getRegionOf(this->stmt);
  auto intTy = Type::getInt32Ty(llvmContext);

  auto model = this;
  auto modelDomain = getIterationDomain().cast();
  auto subdomain = newdomain.projectOut(isl_dim_set, modelDomain.getDimCount(), newdomain.getDimCount() - modelDomain.getDimCount());
  assert(subdomain <= modelDomain);
  //auto subscatter = subscatter_.resetSpace(isl_dim_in);
  //assert(newdomain <= subscatter_.getDomain());
  auto subscatter = subscatter_.cast(newdomain.getSpace() >> subscatter_.getRangeSpace());

  auto nModelDims = modelDomain.getDimCount();
  auto nNewDims = newdomain.getDimCount();
  auto innermostBody = getBasicBlock();
  auto innermostRegion = parentRegion;
  auto innermostLoop = modelLoopNests[modelLoopNests.size()-1];
  BasicBlock *newStmtBB = nullptr;
  assert(modelLoopNests.size() == nModelDims);

  SmallVector<Loop*, 4> nests;
  nests.reserve(nNewDims);

  for (auto i = nModelDims-nModelDims; i < nModelDims; i+=1) {
    nests.push_back(modelLoopNests[i]);
  }

  // the new domain has more dimensions than model's
  // The caller wants us to insert some additional loops
  for (auto i = nModelDims; i < nNewDims; i+=1) {
    Loop *loop = nullptr;
    Region *newRegion = nullptr;
    innermostBody = insertLoop(innermostBody, nullptr, ConstantInt::get(intTy, 3), pass, innermostLoop, /*out*/loop, innermostRegion, /*out*/newRegion);
    newStmtBB = innermostBody;
    innermostRegion = newRegion;
    innermostLoop = loop;

    nests.push_back(loop);
  }

  if (!newStmtBB) {
    newStmtBB = insertDedicatedBB(innermostBody, &innermostBody->front(), false, pass);

    //newStmtBB = BasicBlock::Create(llvmContext, name, function);
    //new UnreachableInst(llvmContext, newStmtBB);

    //assert(pass);
    //auto RI = pass->getAnalysisIfAvailable<RegionInfo>();
    //if (RI)
    //  RI->setRegionFor(newStmtBB, innermostRegion);
    //RI->verifyAnalysis();
  }

  auto newStmt = new ScopStmt(scop, newStmtBB, name, innermostRegion, nests, newdomain.takeCopy(), subscatter.takeCopy());
  auto newWhere = where.setInTupleId(enwrap(newStmt->getDomainId()));
  newStmt->setWhereMap(newWhere.takeCopy());
  scop->addScopStmt(newStmt);

  assert(scop->getRegion().contains(newStmtBB));
  return StmtEditor(newStmt);
}


StmtEditor StmtEditor::replaceStmt(const isl::Map &sub, const std::string &name) {
  restrictInstances(sub);
  return createStmt(sub.getDomain(), enwrap(stmt->getScattering()), sub, name);
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
  return molly::getFunctionOf(stmt);
}


llvm::Module *StmtEditor::getParentModule() {
  return molly::getModuleOf(stmt);
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


isl::MultiAff StmtEditor::getCurrentIteration()  {
  auto domainSpace = getDomainSpace();
  auto result = domainSpace.mapsToItself().createIdentityMultiAff();
  //auto nDims = result.getInDimCount();
  //for (auto i = nDims-nDims; i<nDims; i+=1) {
  //  result.setInDimId_inplace(i, stmt->getDomainId());
  //}
  return result;
}


isl::Map StmtEditor::getWhere() {
  auto result = enwrap(stmt->getWhereMap());
  //result.intersectDomain(getIterationDomain()); // TODO: Remove for efficiency
  return result;
}


isl::Map StmtEditor:: getInstances() {
  return getWhere().intersectDomain(getIterationDomain());
}


void StmtEditor::setWhere(isl::Map where) {
  assert(where.getDomainSpace().matchesSetSpace(getDomainSpace()));
  assert(where.hasOutTupleId());
  assert(where.getDomain().isSupersetOf(getIterationDomain()));
  stmt->setWhereMap(where.take());
}


void StmtEditor::restrictInstances(const isl::Map &where) {
  assert(where.matchesMapSpace(getWhere().getSpace()));
  assert(where.hasOutTupleId());

  assert(getInstances() >= where);

  stmt->setDomain(where.getDomain().take());
  stmt->setWhereMap(where.takeCopy());

  if (where.isEmpty()) {
    // Could totally remove the ScopStmt
  }
}


void StmtEditor::removeInstances(const isl::Map &disableInsts) {
  auto allInstances = getInstances();
  assert(disableInsts <= allInstances);
  restrictInstances(allInstances.subtract(disableInsts));
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
  assert(accessRelation.matchesMapSpace(getDomainSpace(), fvar->getAccessSpace()));
  stmt->addAccess(MemoryAccess::MUST_WRITE, fvar->getVariable(), accessRelation.take(), instr);
}
