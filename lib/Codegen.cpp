#include "Codegen.h"

#include "IslExprBuilder.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include "islpp/MultiPwAff.h"
#include <llvm/IR/Function.h>
#include "IslExprBuilder.h"
#include "MollyIntrinsics.h"
#include <polly/ScopInfo.h>
#include "polly/CodeGen/CodeGeneration.h"
#include "MollyScopStmtProcessor.h"
#include "islpp/Map.h"
#include "MollyPassManager.h"
#include <clang/CodeGen/MollyRuntimeMetadata.h>
#include <llvm/IR/Intrinsics.h>
#include "CommunicationBuffer.h"
#include "MollyScopProcessor.h"
#include "AffineMapping.h"
#include "islpp/AstExpr.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "polly/CodeGen/BlockGenerators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "FieldLayout.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


isl::AstBuild &MollyCodeGenerator::initAstBuild() {
  if (astBuild.isNull()) {
    //auto islctx = stmtCtx->getIslContext();
    auto domain = stmtCtx->getDomainWithNamedDims();
    this->astBuild = isl::AstBuild::createFromContext(domain);
  }
  return astBuild;
}


llvm::Value *MollyCodeGenerator::allocStackSpace(llvm::Type *ty) {
  auto scopCtx  = stmtCtx->getScopProcessor();
  auto func = scopCtx->getParentFunction();
  auto bb = &func->getEntryBlock();

  auto region = scopCtx->getRegion();
  //auto bb = region->getEntry();
  assert(!region->contains(bb));

  auto result = new AllocaInst(ty, Twine(), bb->getFirstInsertionPt());
  return result;
}


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore) 
  : stmtCtx(stmtCtx), irBuilder(stmtCtx->getLLVMContext()), idtovalue(nullptr) {
    auto bb = stmtCtx->getBasicBlock();
    if (insertBefore) {
      assert(insertBefore->getParent() == bb);
      irBuilder.SetInsertPoint(insertBefore);
    } else {
      // Insert at end of block instead
      irBuilder.SetInsertPoint(bb);
    }

    //stmtCtx->identifyDomainDims();
}


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx) : stmtCtx(stmtCtx), irBuilder(stmtCtx->getBasicBlock()), idtovalue(nullptr) {
  //stmtCtx->identifyDomainDims();
}


MollyCodeGenerator::MollyCodeGenerator(llvm::BasicBlock *insertBB, llvm::Instruction *insertBefore, const std::map<isl_id *, llvm::Value *> &idtovalue) 
  : stmtCtx(nullptr), irBuilder(insertBB, insertBefore), idtovalue(&idtovalue)
{

}

StmtEditor MollyCodeGenerator::getStmtEditor() { 
  return StmtEditor(stmtCtx->getStmt());
}


llvm::Value *MollyCodeGenerator::getValueOf(const SCEV *scev) {
  auto scopCtx = stmtCtx->getScopProcessor();
  auto result = scopCtx->codegenScev(scev, irBuilder.GetInsertPoint());
  return result;
}




const std::map<isl_id *, llvm::Value *> & MollyCodeGenerator::getIdToValueMap() {
  if (idtovalue)
    return *idtovalue;

  auto &result = stmtCtx->getIdToValueMap();

  auto scopCtx = stmtCtx->getScopProcessor();
  auto &params = scopCtx->getParamSCEVs();

  auto nDomainDims = stmtCtx->getDomain().getDimCount();

  auto nExpected = params.size()+nDomainDims;
  assert(result.size() <= nExpected);
  if (result.size() == nExpected)
    return result;

  // 1. Context parameters
  for (auto paramScev : params) {
    auto id = scopCtx->idForSCEV(paramScev);
    auto value = getValueOf(paramScev);
    result[id.keep()] = value;
  }

  // 2. The loop induction variables
  for (auto i = nDomainDims-nDomainDims; i < nDomainDims; i+=1) {
    auto iv = stmtCtx->getDomainValue(i);
    auto id = stmtCtx->getDomainId(i);
    assert(id.isValid());
    result[id.keep()] = iv;
  }

  return result;
}


llvm::Module *MollyCodeGenerator::getModule() {
  return irBuilder.GetInsertBlock()->getParent()->getParent();
}


clang::CodeGen::MollyRuntimeMetadata *MollyCodeGenerator::getRtMetadata() {
  return stmtCtx->getPassManager()->getRuntimeMetadata();
}


// SCEVAffinator::getPwAff
llvm::Value *MollyCodeGenerator::codegenAff(const isl::PwAff &aff) {
  //TODO: What does polly::buildIslAff do with several pieces?
  assert(aff.nPiece() == 1);
  // If it splits the BB to test for piece inclusion, the additional ScopStmt would not consider them as belonging to them.
  // Three solution ideas:
  // 1. Use conditional assignment instead of branches
  // 2. Create a ad-hoc function that contains the case distinction; meant to be inlined later by llvm optimization passes
  // 3. Create new ScopStmts for each peace, let Polly generate code to distinguish them


  auto expr = initAstBuild().exprFromPwAff(aff);
  auto &valueMap = getIdToValueMap();
  auto pass = stmtCtx->asPass();
  auto result = polly::codegenIslExpr(irBuilder, expr.takeCopy(), valueMap, pass);
  //auto result = polly::buildIslAff(irBuilder.GetInsertPoint(), aff.takeCopy(), valueMap, stmtCtx->asPass());
  return result;
}


std::vector<llvm::Value *> MollyCodeGenerator::codegenMultiAff(const isl::MultiPwAff &maff) {
  std::vector<llvm::Value *> result;

  auto nDims = maff.getOutDimCount();
  result.reserve(nDims);
  for (auto i = nDims-nDims; i < nDims; i+=1 ) {
    auto aff = maff.getPwAff(i);
    auto value = codegenAff(aff);
    result.push_back(value);
  }

  return result;
}


llvm::Value *MollyCodeGenerator::codegenScev(const llvm::SCEV *scev) {
  return stmtCtx->getScopProcessor()->codegenScev(scev, irBuilder.GetInsertPoint());
}


llvm::Value *MollyCodeGenerator::codegenId(const isl::Id &id) {
  auto user = static_cast<const SCEV*>(id.getUser());
  return codegenScev(user);
}

#if 0
llvm::Value *MollyCodeGenerator::codegenLinearize(const isl::MultiPwAff &coord, const molly::AffineMapping *layout) {
  auto mapping = layout->getMapping();

  // toPwMultiAff() can give exponential number of pieces (in terms of dimensions)
  // alternatively, execute the two mappings sequentially: Fewer cases, but two levels of them
  auto coordMapping = mapping.pullback(coord.toPwMultiAff());
  auto value = codegenAff(coordMapping);
  //TODO: generate assert() to check the result being in the layout's buffer size
  return value;
}
#endif

llvm::Type *MollyCodeGenerator::getIntTy() {
  auto &llvmContext = irBuilder.getContext();
  return Type::getInt32Ty(llvmContext);
}


llvm::Value *MollyCodeGenerator::codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices) {
  auto call = callLocalPtrIntrinsic(fvar, indices);
  return irBuilder.Insert(call, "ptr_local");
}


llvm::CallInst *MollyCodeGenerator::callLocalPtr(FieldVariable *fvar) {
  Value *args[] = { fvar->getVariable() };
  Type *tys[] = {  fvar->getEltPtrType(), args[0]->getType() };
  auto localPtrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_local_ptr, tys);
  return irBuilder.CreateCall(localPtrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSend(molly::CommunicationBuffer *combuf, Value *dstRank) {
  Value *args[] = { combuf->getVariableSend(), dstRank };
  Type *tys[] = { args[0]->getType() };
  auto sendFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send, tys);
  return irBuilder.CreateCall(sendFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecv(molly::CommunicationBuffer *combuf, Value *srcRank) {
  Value *args[] = { combuf->getVariableRecv(), srcRank };
  Type *tys[] = { args[0]->getType() };
  auto recvFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv, tys);
  return irBuilder.CreateCall(recvFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSendWait(molly::CommunicationBuffer *combuf, llvm::Value *dstRank) {
  Value *args[] = { combuf->getVariableSend(), dstRank };
  Type *tys[] = { combuf->getEltPtrType(), args[0]->getType() };
  auto sendWaitFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send_wait, tys);
  return irBuilder.CreateCall(sendWaitFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecvWait(molly::CommunicationBuffer *combuf, llvm::Value *srcRank) {
  Value *args[] = { combuf->getVariableRecv(), srcRank };
  Type *tys[] = { combuf->getEltPtrType(), args[0]->getType() };
  auto recvWaitFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv_wait, tys);
  return irBuilder.CreateCall(recvWaitFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSendbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *dst) {
  Value *args[] = { combuf->getVariableSend(), dst };
  Type *tys[] = { combuf->getFieldType()->getEltPtrType(), args[0]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_sendbuf_ptr, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecvbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *src) {
  Value *args[] = { combuf->getVariableRecv(), src };
  Type *tys[] = { combuf->getFieldType()->getEltPtrType(), args[0]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recvbuf_ptr, tys);
  return irBuilder.CreateCall(ptrFunc, args);      
}

#if 0
void MollyCodeGenerator::codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, isl::Map accessRelation) {
  llvm_unreachable("to be removed");
  auto ptrVal = codegenPtrLocal(fvar, indices);
  auto store = irBuilder.CreateStore(materialize(val), ptrVal);

  auto editor = getStmtEditor();
  accessRelation.setInTupleId_inplace(editor.getDomainTupleId());
  editor.addWriteAccess(store, fvar, accessRelation.move());
}
#endif

void MollyCodeGenerator::codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::PwMultiAff where, isl::MultiPwAff index) {
  auto bufptr = callLocalPtr(fvar);

  auto layout = fvar->getLayout();
  assert(layout);
  auto idx = layout->codegenLocalIndex(*this, where, index);
  auto ptr = irBuilder.CreateGEP(bufptr, idx, "localbufeltptr");

  auto store = irBuilder.CreateStore(materialize(val), ptr);
  addStoreAccess(fvar->getVariable(), index, store);
}


void MollyCodeGenerator::addScalarStoreAccess(llvm::Value *base, llvm::Instruction *instr) {
  auto accessFirstElement = stmtCtx->getDomainSpace().mapsTo(0/*1*/).createZeroMultiAff();
  addStoreAccess(base, accessFirstElement, instr);
}

void molly::MollyCodeGenerator::addStoreAccess( llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr )
{
  stmtCtx->addMemoryAccess(polly::MemoryAccess::MUST_WRITE,base, accessRelation, instr);
}

void MollyCodeGenerator::addScalarLoadAccess(llvm::Value *base, llvm::Instruction *instr) {
  auto accessFirstElement = stmtCtx->getDomainSpace().mapsTo(0/*1*/).createZeroMultiAff();
  addLoadAccess(base, accessFirstElement, instr);
}

void molly::MollyCodeGenerator::addLoadAccess( llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr )
{
  stmtCtx->addMemoryAccess(polly::MemoryAccess::READ,base, accessRelation, instr);
}


bool MollyCodeGenerator::isDependent(llvm::Value *val) {
  // Not in SCoP-scope, therefore never dependent
  if (!stmtCtx)
    return false;

  // Something that does not relate to scopes (Constants, GlobalVariables)
  auto instr = dyn_cast<Instruction>(val);
  if (!instr)
    return false;

  // Something within this statement is also non-dependent of the SCoP around
  auto bb = stmtCtx->getBasicBlock();
  if (instr->getParent() == bb)
    return false;

  // Something outside the SCoP, but inside the function is also not dependent
  auto scopCtx = stmtCtx->getScopProcessor();
  auto scopRegion = stmtCtx->getRegion();
  if (!scopRegion->contains(instr))
    return false;

  // As special exception, something that dominates all ScopStmts also is not concerned by shuffling the ScopStmts' order
  // This is meant for AllocaInsts we put there
  //if (instr->getParent() == scopRegion->getEntry())
  //  return false;

  return true;
}


llvm::Value *MollyCodeGenerator::getScalarAlloca(llvm::Value *val) {
  // Is this value loading the scalar; if yes, it's scalar location is obviously where it has been loaded from
  if (auto load = dyn_cast<LoadInst>(val)) {
    auto ptr = load->getPointerOperand();
    // auto allocaInst = dyn_cast<AllocaInst>(ptr);
    if (!isDependent(ptr))
      return ptr;
  }

  // Look where IndependentBlocks stored the val
  for (auto itUse = val->use_begin(), endUse= val->use_end(); itUse!=endUse;++itUse ) {
    auto useInstr = *itUse;
    if (!isa<StoreInst>(useInstr))
      continue;
    if (itUse.getOperandNo() == StoreInst::getPointerOperandIndex())
      continue; // Must be the value operand, not the target ptr

    auto store = cast<StoreInst>(useInstr);
    auto ptr = store->getPointerOperand();
    //if (!isa<AllocaInst>(ptr))
    //  continue;
    if (isDependent(ptr))
      continue; // Write to some array at an index that might depend on the IVs

    return ptr;
  }
  return nullptr;
}


llvm::Value * molly::MollyCodeGenerator::materialize(llvm::Value *val){
  if (!isDependent(val))
    return val; // Use val directly; no materialization needed
  auto instr = cast<Instruction>(val);

  auto scopCtx = stmtCtx->getScopProcessor();
  auto scopRegion = stmtCtx->getRegion();
  auto pass = stmtCtx->asPass();
  auto SE = &pass->getAnalysis<ScalarEvolution>();
  auto LI = &pass->getAnalysis<LoopInfo>();

  // If possible, let ScalarEvolution handle this
  if (polly::canSynthesize(instr, LI, SE, scopRegion)) { // FIXME: Still works when SCEVCodegen is enabled?
    auto scev = SE->getSCEV(val);
    auto result = scopCtx->codegenScev(scev, irBuilder.GetInsertPoint());
    assert(!isDependent(result));
    return result;
  }

  // Do it manually 
  // If it is a loaded value from stack, reload it; Typically, this has been done by IndependentBlocks
  Value *target=nullptr;
  if (auto load = dyn_cast<LoadInst>(val)) {
    auto ptr = load->getPointerOperand();
    if (!isDependent(ptr))
      target = ptr;
  }

  // If the value is stored somewhere, load it from the same location
  if (!target) {
    target = getScalarAlloca(val);
  }

  // Otherwise, insert a store ourselves
  if (!target) {
    target = allocStackSpace(val->getType());
    auto store = new StoreInst(val, target, instr->getNextNode());
    addScalarStoreAccess(target, store);
  }

  // TODO: Could also search all LoadInsts to see if this value has already been loaded

  // Load the value from memory in the right BB
  auto reload = irBuilder.CreateLoad(target);
  addScalarLoadAccess(target, reload);
  assert(!isDependent(reload));
  return reload;
}


void MollyCodeGenerator::updateScalar(llvm::Value *toupdate, llvm::Value *val) {
  assert(!isDependent(val));
  auto ptr = getScalarAlloca(toupdate);

  if (ptr) { // if !ptr this should mean that val is not used outside the current ScopStmt
    auto store = irBuilder.CreateStore(val, ptr);
    addScalarStoreAccess(ptr, store);
  }
  //TODO: If necessary, iterator through all uses and replace those in this BasicBlock
  //toupdate->replaceAllUsesWith(val);
}


isl::Ctx *molly::MollyCodeGenerator::getIslContext()
{
  return stmtCtx->getIslContext();
}
