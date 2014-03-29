#include "Codegen.h"

#include "MollyScopStmtProcessor.h"
#include "IslExprBuilder.h"
#include "FieldVariable.h"
#include "FieldType.h"
#include "IslExprBuilder.h"
#include "MollyIntrinsics.h"
#include "MollyPassManager.h"
#include "CommunicationBuffer.h"
#include "MollyScopProcessor.h"
#include "AffineMapping.h"
#include "FieldLayout.h"
#include "MollyUtils.h"

#include "islpp/Map.h"
#include "islpp/MultiPwAff.h"
#include "islpp/AstExpr.h"

#include <polly/CodeGen/CodeGeneration.h>
#include <polly/CodeGen/BlockGenerators.h>
#include <polly/ScopInfo.h>

#include <clang/CodeGen/MollyRuntimeMetadata.h>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/CommandLine.h>

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


extern cl::OptionCategory MollyCategory;

static cl::opt<bool>
MollyMarkStmts("molly-mark-stmts",
cl::desc("Emit begin end and markers for generated ScopStmts"),
cl::Hidden, cl::init(false), cl::ZeroOrMore,
cl::cat(MollyCategory));

isl::AstBuild &MollyCodeGenerator::initAstBuild() {
  if (astBuild.isNull()) {
    if (stmtCtx) {
      fillIdToValueMap();
      //auto islctx = stmtCtx->getIslContext();
      auto domain = context.isValid() ? context : stmtCtx->getDomainWithNamedDims(); //TODO: Add constraints for parameter dimensions
      this->astBuild = isl::AstBuild::createFromContext(domain);
    } else {
      assert(context.isValid());
      this->astBuild = isl::AstBuild::createFromContext(context);
    }
  }
  return astBuild;
}

#if 0
static llvm::Value *copyOperandTree_recursive(llvm::DenseMap<Instruction*, Instruction*> &oldToNew, Value *old) {
  auto oldInstr = dyn_cast<Instruction>(old);
  if (!oldInstr)
    return old; // Non-instructions (i.e. constants) are global; no need to copy

  auto destBB = builder.GetInsertBlock();
  if (oldInstr->getParent() == destBB)
    return destBB; // Already in target block

  if (is)

    auto &result = oldToNew[oldInstr];
  if (result) 
    return result; // Already copied
}
#endif


static bool isSafeToMove(Instruction *Inst) {
  if (Inst->mayReadFromMemory() || Inst->mayWriteToMemory())
    return false;

  return isSafeToSpeculativelyExecute(Inst);
}
typedef llvm::DenseMap<Instruction*, Instruction*> ReplacedMapType;


llvm::Value *MollyCodeGenerator::copyOperandTree(llvm::Value *val) {
  //llvm::DenseMap<Instruction*, Instruction*> oldToNew;
  //return copyOperandTree_recursive(oldToNew, irBuilder, val);

  auto oldInst = dyn_cast<Instruction>(val);
  if (!oldInst)
    return val; // Constants are home to any BB
  auto oldBB = oldInst->getParent();
  auto CurBB = stmtCtx->getBasicBlock();
  if (oldBB == CurBB)
    return oldInst; // Already in required block
  //auto InsertPos = irBuilder.GetInsertPoint();
  auto SE = &pass->getAnalysis<ScalarEvolution>();
  auto LI = &pass->getAnalysis<LoopInfo>();
  auto R = stmtCtx->getScopProcessor()->getRegion();
  //Instruction *result = nullptr;
  ReplacedMapType ReplacedMap;

  if (!R->contains(oldInst)) {
    return oldInst; // SCoP-invariant
  }

  if (canSynthesize(oldInst, LI, SE, R)) {
    return oldInst; // Loop counter
  }

  assert(isSafeToMove(oldInst)); // by polly::IndependentBlocks

  auto newInst = oldInst->clone();
  auto InsertPos = irBuilder.Insert(newInst);
  auto Inst = newInst;

  // Copied from IndependentBlock /////////////////////////////////////////////

  // Depth first traverse the operand tree (or operand dag, because we will
  // stop at PHINodes, so there are no cycles).
  typedef Instruction::op_iterator ChildIt;
  std::vector<std::pair<Instruction *, ChildIt>> WorkStack;

  WorkStack.push_back(std::make_pair(Inst, Inst->op_begin()));
  DenseSet<Instruction *> VisitedSet;

  while (!WorkStack.empty()) {
    Instruction *CurInst = WorkStack.back().first;
    ChildIt It = WorkStack.back().second;
    DEBUG(dbgs() << "Checking Operand of Node:\n" << *CurInst << "\n------>\n");
    if (It == CurInst->op_end()) {
      // Insert the new instructions in topological order.
      if (!CurInst->getParent()) {
        CurInst->insertBefore(InsertPos);
        SE->forgetValue(CurInst);
      }

      WorkStack.pop_back();
    } else {
      // for each node N,
      Instruction *Operand = dyn_cast<Instruction>(*It);
      ++WorkStack.back().second;

      // Can not move no instruction value.
      if (Operand == 0)
        continue;

      DEBUG(dbgs() << "For Operand:\n" << *Operand << "\n--->");

      if (Operand->getParent() != oldBB)
        continue; // The other BB must already been independent, therefore no need to copy anything from other BBs

      // If the Scop Region does not contain N, skip it and all its operands and
      // continue: because we reach a "parameter".
      // FIXME: we must keep the predicate instruction inside the Scop,
      // otherwise it will be translated to a load instruction, and we can not
      // handle load as affine predicate at this moment.
      //if (!R->contains(Operand) && !isa<TerminatorInst>(CurInst)) {
      //  DEBUG(dbgs() << "Out of region.\n");
      //  continue;
      //}

      if (canSynthesize(Operand, LI, SE, R)) {
        DEBUG(dbgs() << "is IV.\n");
        continue;
      }

      // We can not move the operand, a non trivial scalar dependence found!
      //if (!isSafeToMove(Operand)) {
      //  DEBUG(dbgs() << "Can not move!\n");
      //  continue;
      //}
      assert(isa<LoadInst>(Operand) || isSafeToMove(Operand)); // Ensured by polly::IndependendBlocks

      // Do not need to move instruction if it is contained in the same BB with
      // the root instruction.
      if (Operand->getParent() == CurBB) {
        DEBUG(dbgs() << "No need to move.\n");
        // Try to move its operand, but do not visit an instruction twice.
        if (VisitedSet.insert(Operand).second)
          WorkStack.push_back(std::make_pair(Operand, Operand->op_begin()));
        continue;
      }

      // Now we need to move Operand to CurBB.
      // Check if we already moved it.
      ReplacedMapType::iterator At = ReplacedMap.find(Operand);
      if (At != ReplacedMap.end()) {
        DEBUG(dbgs() << "Moved.\n");
        Instruction *MovedOp = At->second;
        It->set(MovedOp);
        SE->forgetValue(MovedOp);
      } else {
        // Note that NewOp is not inserted in any BB now, we will insert it when
        // it popped from the work stack, so it will be inserted in topological
        // order.
        Instruction *NewOp = Operand->clone(); // TODO: In case of LoadInst, create a MemoryAccess for it
        //if (Operand == Inst) {
        //  assert(!!result);
        //  result = NewOp;
        //}
        NewOp->setName(Operand->getName() + ".copied.to." + CurBB->getName());
        DEBUG(dbgs() << "Copy to " << *NewOp << "\n");
        It->set(NewOp);
        ReplacedMap.insert(std::make_pair(Operand, NewOp));
        SE->forgetValue(Operand);

        // Process its operands, but do not visit an instruction twice.
        if (VisitedSet.insert(NewOp).second)
          WorkStack.push_back(std::make_pair(NewOp, NewOp->op_begin()));
      }
    }
  }
  SE->forgetValue(Inst);

  //auto it = ReplacedMap.find(Inst);
  //assert(it!=ReplacedMap.end());
  //assert(it->first==Inst);
  //auto result = it->second;
  return newInst;
}


llvm::Value *MollyCodeGenerator::allocStackSpace(llvm::Type *ty, const llvm::Twine &name) {
  auto intTy = Type::getInt64Ty(getLLVMContext());
  return allocStackSpace(ty, ConstantInt::get(intTy, 1));
}


llvm::Value *MollyCodeGenerator::allocStackSpace(llvm::Type *ty, int count, const llvm::Twine &name) {
  auto intTy = Type::getInt64Ty(getLLVMContext());
  return allocStackSpace(ty, ConstantInt::get(intTy, count), name);
}


llvm::Value *MollyCodeGenerator::allocStackSpace(llvm::Type *ty, llvm::Value *count, const llvm::Twine &name) {
  auto bb = irBuilder.GetInsertBlock();
  auto func = getFunctionOf(bb);
  auto entryBB = &func->getEntryBlock();
  if (entryBB == getInsertBlock()) {
    if (auto instr = dyn_cast<Instruction>(count)) {
      assert(instr->getParent()==entryBB);
    }
    return irBuilder.CreateAlloca(ty, count, name);
  }
  assert(isa<ConstantInt>(count));
  return new AllocaInst(ty, count, name, entryBB->getFirstInsertionPt());
}


llvm::Value *MollyCodeGenerator::createPointerCast(llvm::Value *val, llvm::Type *type) {
  return irBuilder.CreatePointerCast(val, type);
}


MollyCodeGenerator::MollyCodeGenerator(llvm::BasicBlock *insertBB, llvm::Instruction *insertBefore, llvm::Pass *pass)
: stmtCtx(nullptr), irBuilder(insertBB->getContext()), pass(pass) {
  if (insertBefore) {
    assert(insertBB == insertBefore->getParent());
    irBuilder.SetInsertPoint(insertBefore);
  } else
    irBuilder.SetInsertPoint(insertBB);
}


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore)
: stmtCtx(stmtCtx), irBuilder(stmtCtx->getLLVMContext()), pass(stmtCtx->asPass()) {
  auto bb = stmtCtx->getBasicBlock();
  if (insertBefore) {
    assert(insertBefore->getParent() == bb);
    irBuilder.SetInsertPoint(insertBefore);
  } else {
    // Insert at end of block instead
    irBuilder.SetInsertPoint(bb);
  }

  //fillIdToValueMap();
}


MollyCodeGenerator::MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx) : stmtCtx(stmtCtx), irBuilder(stmtCtx->getBasicBlock()), pass(stmtCtx->asPass()) {
  //fillIdToValueMap();
}


MollyCodeGenerator::MollyCodeGenerator(llvm::BasicBlock *insertBB, llvm::Instruction *insertBefore, Pass *pass, isl::Set context, const std::map<isl_id *, llvm::Value *> &idtovalue)
: stmtCtx(nullptr), irBuilder(insertBB->getContext()), pass(pass), context(context.move()), idtovalue(idtovalue) {
  if (insertBefore) {
    assert(insertBB == insertBefore->getParent());
    irBuilder.SetInsertPoint(insertBefore);
  } else
    irBuilder.SetInsertPoint(insertBB);
}


StmtEditor MollyCodeGenerator::getStmtEditor() {
  return StmtEditor(stmtCtx->getStmt());
}


void MollyCodeGenerator::addParam(isl::Id id, llvm::Value *val) {
  astBuild.reset();

  // Create a empty context if none was there
  // Not possible in constructor since we may not have a isl::Ctx
  if (context.isNull()) {
    auto islctx = id.getCtx();
    context = islctx->createSetSpace(0, 0).universeSet();
  }

  context.addParamDim_inplace(id);
  idtovalue[id.keep()] = val;
}


llvm::Value *MollyCodeGenerator::getValueOf(const SCEV *scev) {
  auto scopCtx = stmtCtx->getScopProcessor();
  auto result = scopCtx->codegenScev(scev, irBuilder.GetInsertPoint());
  return result;
}


void MollyCodeGenerator::fillIdToValueMap() {
  //auto &result = stmtCtx->getIdToValueMap();
  auto &result = idtovalue;
  //assert(result.empty());
  //result.clear();

  auto scopCtx = stmtCtx->getScopProcessor();
  auto &params = scopCtx->getParamSCEVs();

  auto nDomainDims = stmtCtx->getDomain().getDimCount();

  auto nExpected = params.size() + nDomainDims;
  assert(result.size() <= nExpected);
  //if (result.size() == nExpected)
  //  return result;

  // 1. Context parameters
  for (auto paramScev : params) {
    auto id = scopCtx->idForSCEV(paramScev);
    auto value = getValueOf(paramScev);
    result[id.keep()] = value;
  }

  // 2. The loop induction variables
  for (auto i = nDomainDims - nDomainDims; i < nDomainDims; i += 1) {
    auto iv = stmtCtx->getDomainValue(i);
    auto id = stmtCtx->getDomainId(i);
    assert(id.isValid());
    result[id.keep()] = iv;
  }
}


llvm::Module *MollyCodeGenerator::getModule() {
  return irBuilder.GetInsertBlock()->getParent()->getParent();
}


clang::CodeGen::MollyRuntimeMetadata *MollyCodeGenerator::getRtMetadata() {
  return stmtCtx->getPassManager()->getRuntimeMetadata();
}


static llvm::Value *codegenIslExprwithDefaultIRBuilder(DefaultIRBuilder &irBuilder, isl::AstExpr expr, const std::map<isl_id *, llvm::Value *> &values, llvm::Pass *pass) {
  PollyIRBuilder pollyBuilder(irBuilder.GetInsertBlock(), irBuilder.GetInsertPoint(), irBuilder.getFolder(), irBuilder.getDefaultFPMathTag());
  return polly::codegenIslExpr(pollyBuilder, expr.take(), values, pass);
}


// SCEVAffinator::getPwAff
llvm::Value *MollyCodeGenerator::codegenAff(const isl::PwAff &aff) {
  //TODO: What does polly::buildIslAff do with several pieces? Answer: Insert select instructions 
  //assert(aff.nPiece() == 1);
  // If it splits the BB to test for piece inclusion, the additional ScopStmt would not consider them as belonging to them.
  // Three solution ideas:
  // 1. Use conditional assignment instead of branches
  // 2. Create a ad-hoc function that contains the case distinction; meant to be inlined later by llvm optimization passes
  // 3. Create new ScopStmts for each peace, let Polly generate code to distinguish them

  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);

  auto expr = initAstBuild().exprFromPwAff(aff); //aff.resetDimIds(isl_dim_all)
  // auto pass = stmtCtx->asPass();
  auto result = codegenIslExprwithDefaultIRBuilder(irBuilder, expr, idtovalue, pass);
  //auto result = polly::buildIslAff(irBuilder.GetInsertPoint(), aff.takeCopy(), valueMap, stmtCtx->asPass());
  return irBuilder.CreateIntCast(result, intTy, false/*???*/);
}


std::vector<llvm::Value *> MollyCodeGenerator::codegenMultiAff(const isl::MultiPwAff &maff) {
  std::vector<llvm::Value *> result;

  auto nDims = maff.getOutDimCount();
  result.reserve(nDims);
  for (auto i = nDims - nDims; i < nDims; i += 1) {
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
  return Type::getInt64Ty(llvmContext);
}


llvm::Value *MollyCodeGenerator::codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices) {
  auto call = callLocalPtrIntrinsic(fvar, indices);
  return irBuilder.Insert(call, "ptr_local");
}



Function *MollyCodeGenerator::getRuntimeFunc(llvm::StringRef name, llvm::Type *retTy, llvm::ArrayRef<llvm::Type*> tys) {
  auto module = getModuleOf(irBuilder);
  auto &llvmContext = module->getContext();

  auto funcTy = FunctionType::get(retTy, tys, false);
  auto func = cast<Function>(module->getOrInsertFunction(name, funcTy));
  //auto initFunc = module->getFunction(name);
  //if (!initFunc) {
  // auto initFuncTy = FunctionType::get(retTy, tys, false);
  //  initFunc = Function::Create(initFuncTy, GlobalValue::ExternalLinkage, name, module);
  // }

  // Check if the function matches
  assert(func->getReturnType() == retTy);
  assert(func->getFunctionType()->getNumParams() == tys.size());
  auto nParams = tys.size();
  for (auto i = nParams - nParams; i < nParams; i += 1) {
    assert(func->getFunctionType()->getParamType(i) == tys[i]);
  }
  return func;
}


llvm::CallInst *MollyCodeGenerator::callLocalInit(llvm::Value *fvar, llvm::Value *elts, llvm::Function *rankoffunc, llvm::Function *indexoffunc) {
  Value *args[] = { fvar, elts, rankoffunc, indexoffunc };
  Type *tys[] = { args[0]->getType(), args[2]->getType(), args[3]->getType() };
  auto intFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_local_init, tys);
  return irBuilder.CreateCall(intFunc, args);
}


llvm::CallInst *molly::MollyCodeGenerator::callRuntimeLocalInit(llvm::Value *fvar, llvm::Value *elts, llvm::Function *rankoffunc, llvm::Function *indexoffunc){
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy, voidPtrTy, voidPtrTy };
  auto funcDecl = getRuntimeFunc("__molly_local_init", voidTy, tys);

  auto fvarvoid = irBuilder.CreatePointerCast(fvar, voidPtrTy);
  auto rankofvoid = irBuilder.CreatePointerCast(rankoffunc, voidPtrTy);
  auto indexofvoid = irBuilder.CreatePointerCast(indexoffunc, voidPtrTy);
  return irBuilder.CreateCall4(funcDecl, fvarvoid, elts, rankofvoid, indexofvoid);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeLocalIndexof(llvm::Value *fvar, llvm::ArrayRef<llvm::Value *> coords) {
  assert(fvar);

  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);
  auto nDims = coords.size();

  SmallVector<Type*, 4> tys;
  SmallVector<Value*, 4> args;
  tys.reserve(1 + coords.size());
  args.reserve(1 + coords.size());

  // "this"
  tys.push_back(voidPtrTy);
  args.push_back(fvar);

  // Args
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    tys.push_back(intTy);
    args.push_back(coords[i]);
  }

  auto funcDecl = getRuntimeFunc("__molly_local_indexof", intTy, tys);

  return irBuilder.CreateCall(funcDecl, args);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeClusterCurrentCoord(llvm::Value *d) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  Type *tys[] = { intTy };
  auto funcDecl = getRuntimeFunc("__molly_cluster_current_coordinate", intTy, tys);

  if (d->getType() != intTy)
    d = irBuilder.CreateIntCast(d, intTy, false);
  return irBuilder.CreateCall(funcDecl, d);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufSendAlloc(llvm::Value *nDst, llvm::Value *eltSize, llvm::Value *tag) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);

  Type *tys[] = { intTy, intTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_send_alloc", voidPtrTy, tys);
  return irBuilder.CreateCall3(funcDecl, nDst, eltSize, tag);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufRecvAlloc(llvm::Value *nDst, llvm::Value *eltSize, llvm::Value *tag) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);

  Type *tys[] = { intTy, intTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_recv_alloc", voidPtrTy, tys);
  return irBuilder.CreateCall3(funcDecl, nDst, eltSize, tag);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufSendDstInit(llvm::Value *combufSend, llvm::Value *dst, llvm::Value *nClusterDims, llvm::Value *dstCoords, llvm::Value *countElts, llvm::Value *tag) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intPtrTy = Type::getInt64PtrTy(llvmContext);

  Type *tys[] = { voidPtrTy, intTy, intTy, intPtrTy, intTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_send_dst_init", voidTy, tys);

  //auto combufVal = irBuilder.CreatePointerCast(combufSend, voidPtrTy);
  Value *Args[] = { combufSend, dst, nClusterDims, dstCoords, countElts, tag };
  return irBuilder.CreateCall(funcDecl, Args);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufRecvSrcInit(llvm::Value *combufSend, llvm::Value *src, llvm::Value *nClusterDims, llvm::Value *srcCoords, llvm::Value *countElts, llvm::Value *tag) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intPtrTy = Type::getInt64PtrTy(llvmContext);

  Type *tys[] = { voidPtrTy, intTy, intTy, intPtrTy, intTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_recv_src_init", voidTy, tys);

  Value *Args[] = { combufSend, src, nClusterDims, srcCoords, countElts, tag };
  return irBuilder.CreateCall(funcDecl, Args);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufSendPtr(llvm::Value *combufSend, llvm::Value *dstRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_send_ptr", voidPtrTy, tys);

  //auto combufSendAsVoid = irBuilder.CreatePointerCast(combufSend, voidPtrTy);
  return irBuilder.CreateCall2(funcDecl, combufSend, dstRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufRecvPtr(llvm::Value *combufRecv, llvm::Value *srcRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_recv_ptr", voidPtrTy, tys);

  //auto combufRecvAsVoid = irBuilder.CreatePointerCast(combufRecv, voidPtrTy);
  return irBuilder.CreateCall2(funcDecl, combufRecv, srcRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufSend(llvm::Value *combufSend, llvm::Value *dstRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_send", voidTy, tys);

  return irBuilder.CreateCall2(funcDecl, combufSend, dstRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufRecv(llvm::Value *combufRecv, llvm::Value *srcRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_recv", voidTy, tys);

  return irBuilder.CreateCall2(funcDecl, combufRecv, srcRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufSendWait(llvm::Value *combufSend, llvm::Value *dstRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_send_wait", voidPtrTy, tys);

  return irBuilder.CreateCall2(funcDecl, combufSend, dstRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeCombufRecvWait(llvm::Value *combufRecv, llvm::Value *srcRank) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy, intTy };
  auto funcDecl = getRuntimeFunc("__molly_combuf_recv_wait", voidPtrTy, tys);

  return irBuilder.CreateCall2(funcDecl, combufRecv, srcRank);
}


llvm::CallInst *MollyCodeGenerator::callRuntimeLocalPtr(llvm::Value *localobj) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy };
  auto funcDecl = getRuntimeFunc("__molly_local_ptr", voidPtrTy, tys);

  auto localvoidptr = irBuilder.CreatePointerCast(localobj, voidPtrTy);
  return irBuilder.CreateCall(funcDecl, localvoidptr);
}


llvm::CallInst *MollyCodeGenerator::callClusterCurrentCoord(llvm::Value *d) {
  auto localPtrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_cluster_current_coordinate);
  return irBuilder.CreateCall(localPtrFunc, d);
}


llvm::CallInst *MollyCodeGenerator::callLocalPtr(FieldVariable *fvar) {
  Value *args[] = { fvar->getVariable() };
  Type *tys[] = { fvar->getEltPtrType(), args[0]->getType() };
  auto localPtrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_local_ptr, tys);
  return irBuilder.CreateCall(localPtrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSend(molly::CommunicationBuffer *combuf, Value *dstRank) {
  Value *args[] = { combuf->codegenPtrToSendbufObj(*this), dstRank };
  Type *tys[] = { args[0]->getType() };
  auto sendFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send, tys);
  return irBuilder.CreateCall(sendFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecv(molly::CommunicationBuffer *combuf, Value *srcRank) {
  Value *args[] = { combuf->codegenPtrToRecvbufObj(*this), srcRank };
  Type *tys[] = { args[0]->getType() };
  auto recvFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv, tys);
  return irBuilder.CreateCall(recvFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSendWait(molly::CommunicationBuffer *combuf, llvm::Value *dstRank) {
  Value *args[] = { combuf->codegenPtrToSendbufObj(*this), dstRank };
  Type *tys[] = { combuf->getEltPtrType(), args[0]->getType() };
  auto sendWaitFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send_wait, tys);
  return irBuilder.CreateCall(sendWaitFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecvWait(molly::CommunicationBuffer *combuf, llvm::Value *srcRank) {
  Value *args[] = { combuf->codegenPtrToRecvbufObj(*this), srcRank };
  Type *tys[] = { combuf->getEltPtrType(), args[0]->getType() };
  auto recvWaitFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv_wait, tys);
  return irBuilder.CreateCall(recvWaitFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufSendbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *dst) {
  Value *args[] = { combuf->codegenPtrToSendbufObj(*this), dst };
  Type *tys[] = { combuf->getFieldType()->getEltPtrType(), args[0]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_send_ptr, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callCombufRecvbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *src) {
  Value *args[] = { combuf->codegenPtrToRecvbufObj(*this), src };
  Type *tys[] = { combuf->getFieldType()->getEltPtrType(), args[0]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_combuf_recv_ptr, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}



llvm::CallInst *MollyCodeGenerator::callValueLoad(FieldVariable *fvar, llvm::Value *valptr, llvm::Value *rank, llvm::Value *idx) {
  Value *args[] = { fvar->getVariable(), valptr, rank, idx };
  Type *tys[] = { args[0]->getType(), args[1]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_value_load, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}
llvm::CallInst *MollyCodeGenerator::callRuntimeValueLoad(FieldVariable *fvar, llvm::Value *valptr, llvm::Value *rank, llvm::Value *idx) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy/*field*/, voidPtrTy/*buffer*/, intTy/*rank*/, intTy/*idx*/ };
  auto funcDecl = getRuntimeFunc("__molly_value_load", voidTy, tys); auto funcTy = funcDecl->getFunctionType();

  auto fvarvoidptr = irBuilder.CreatePointerCast(fvar->getVariable(), funcTy->getParamType(0));
  auto valvoidptr = irBuilder.CreatePointerCast(valptr, funcTy->getParamType(1));
  return irBuilder.CreateCall4(funcDecl, fvarvoidptr, valvoidptr, rank, idx);
}
llvm::LoadInst *MollyCodeGenerator::codegenValueLoad(FieldVariable *fvar, llvm::Value *rank, llvm::Value *idx) {
  auto tmp = allocStackSpace(fvar->getEltType());
  callValueLoad(fvar, tmp, rank, idx);
  return irBuilder.CreateLoad(tmp, "valld");
}


//void MollyCodeGenerator::codegenValueLoadPtr(FieldVariable *fvar, llvm::Value *rank, llvm::Value *idx, llvm::Value *writeIntoPtr) {
//  callValueLoad(fvar, writeIntoPtr, rank, idx);
//}


llvm::CallInst *MollyCodeGenerator::callValueStore(FieldVariable *fvar, llvm::Value *valueToStore, llvm::Value *rank, llvm::Value *idx) {
  Value *args[] = { fvar->getVariable(), valueToStore, rank, idx };
  Type *tys[] = { args[0]->getType(), args[1]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_value_store, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}
llvm::CallInst *MollyCodeGenerator::callRuntimeValueStore(FieldVariable *fvar, llvm::Value *dstbufptr, llvm::Value *rank, llvm::Value *idx) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto voidPtrTy = Type::getInt8PtrTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);

  Type *tys[] = { voidPtrTy/*field*/, voidPtrTy/*buffer*/, intTy/*rank*/, intTy/*idx*/ };
  auto funcDecl = getRuntimeFunc("__molly_value_store", voidTy, tys);

  auto fvarvoidptr = irBuilder.CreatePointerCast(fvar->getVariable(), voidPtrTy);
  auto valvoidptr = irBuilder.CreatePointerCast(dstbufptr, voidPtrTy);
  return irBuilder.CreateCall4(funcDecl, fvarvoidptr, valvoidptr, rank, idx);
}
void MollyCodeGenerator::codegenValueStore(FieldVariable *fvar, llvm::Value *val, llvm::Value *rank, llvm::Value *idx) {
  auto tmp = allocStackSpace(fvar->getEltType());
  irBuilder.CreateStore(val, tmp);
  callValueStore(fvar, tmp, rank, idx);
}
void MollyCodeGenerator::codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::PwMultiAff where, isl::MultiPwAff index) {
  auto bufptr = callLocalPtr(fvar);

  auto layout = fvar->getLayout();
  assert(layout);
  auto idx = layout->codegenLocalIndex(*this, where, index);
  auto ptr = irBuilder.CreateGEP(bufptr, idx, "localbufeltptr");

  auto store = irBuilder.CreateStore(materialize(val), ptr);
  addStoreAccess(fvar->getVariable(), index, store);
}


void MollyCodeGenerator::codegenAssignLocalFromScalar(FieldVariable *dstFvar, isl::PwMultiAff dstWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff dstIndex/* [domain] -> field[indexset] */, llvm::Value *srcPtr) {
  auto dstPtr = codegenLoadLocalPtr(dstFvar, dstWhere, dstIndex);
  codegenAssignPtrPtr(dstPtr, dstFvar->getVariable(), dstIndex, srcPtr, srcPtr, isl::Map());
}


llvm::Value *MollyCodeGenerator::getLocalBufPtr(FieldVariable *fvar) {
  auto ptrptr = stmtCtx->getScopProcessor()->codegenLocalBufferPtrOf(fvar);
  return irBuilder.CreateLoad(ptrptr);
}


llvm::Value *MollyCodeGenerator::getSendBufPtrs(CommunicationBuffer *combuf) {
  auto ptrarray = stmtCtx->getScopProcessor()->codegenSendbufPtrsOf(combuf);
  return ptrarray;
}


llvm::Value *MollyCodeGenerator::getSendbufPtrPtr(CommunicationBuffer *combuf, llvm::Value *index) {
  auto ptrarray = getSendBufPtrs(combuf);
  auto sendbufPtrPtr = irBuilder.CreateGEP(ptrarray, index, "sendbufptrptr");
  return sendbufPtrPtr;
}


llvm::Value *MollyCodeGenerator::getSendbufPtr(CommunicationBuffer *combuf, llvm::Value *index) {
  auto sendbufPtrPtr = getSendbufPtrPtr(combuf, index);
  auto sendbufPtr = irBuilder.CreateLoad(sendbufPtrPtr);
  return sendbufPtr;
}


llvm::Value *MollyCodeGenerator::getRecvBufPtrs(CommunicationBuffer *combuf) {
  auto ptrarray = stmtCtx->getScopProcessor()->codegenRecvbufPtrsOf(combuf);
  return ptrarray;
}


llvm::Value *MollyCodeGenerator::getRecvbufPtrPtr(CommunicationBuffer *combuf, llvm::Value *index) {
  auto ptrarray = getRecvBufPtrs(combuf);
  auto sendbufPtrPtr = irBuilder.CreateGEP(ptrarray, index, "recvbufptrptr");
  return sendbufPtrPtr;
}


llvm::Value *MollyCodeGenerator::getRecvbufPtr(CommunicationBuffer *combuf, llvm::Value *index) {
  auto sendbufPtrPtr = getRecvbufPtrPtr(combuf, index);
  auto sendbufPtr = irBuilder.CreateLoad(sendbufPtrPtr);
  return sendbufPtr;
}



llvm::Value *MollyCodeGenerator::codegenLoadLocalPtr(FieldVariable *fvar, isl::PwMultiAff where/* [domain] -> curNode[cluster] */, isl::MultiPwAff index/* [domain] -> field[indexset] */) {
  auto bufptr = getLocalBufPtr(fvar);

  auto layout = fvar->getLayout();
  assert(layout);
  auto idx = layout->codegenLocalIndex(*this, where, index);
  auto ptr = irBuilder.CreateGEP(bufptr, idx, "localbufeltptr");
  return ptr;
}


llvm::Value *MollyCodeGenerator::codegenLoadLocal(FieldVariable *fvar, isl::PwMultiAff where/* [domain] -> curNode[cluster] */, isl::MultiPwAff index/* [domain] -> field[indexset] */) {
  auto ptr = codegenLoadLocalPtr(fvar, where, index);
  auto load = irBuilder.CreateLoad(ptr, "localval");
  addLoadAccess(fvar->getVariable(), index, load);
  return load;
}


llvm::MemCpyInst *MollyCodeGenerator::codegenAssignPtrPtr(llvm::Value *dstPtr, llvm::Value *srcPtr) {
  assert(dstPtr);
  assert(srcPtr);
  assert(dstPtr->getType() == srcPtr->getType());
  auto ty = dstPtr->getType()->getPointerElementType();
  auto size = ConstantExpr::getSizeOf(ty);
  // Could also add a load/store pair for non-aggregate types
  auto result = irBuilder.CreateMemCpy(dstPtr, srcPtr, size, 0);
  return cast<MemCpyInst>(result);
}


void MollyCodeGenerator::codegenAssign(const AnnotatedPtr &dst, const AnnotatedPtr &src) {
  auto dstPtr = dst.getPtr();
  auto ty = dstPtr->getType()->getPointerElementType();

  llvm::Instruction *ldInstr = nullptr;
  llvm::Instruction *stInstr = nullptr;
  if (src.isRegister()) {
    auto srcVal = src.getRegister();
    assert(srcVal->getType() == ty);

    auto bbVal = copyOperandTree(srcVal); // TODO: This should be able to handle the other cases as well
    assert(bbVal);
    stInstr = irBuilder.CreateStore(bbVal, dstPtr, "assignvalreg");
  } else {
    auto srcPtr = src.getPtr();
    assert(ty == srcPtr->getType()->getPointerElementType());

    if (ty->isSingleValueType()) {
      ldInstr = irBuilder.CreateLoad(srcPtr, "assignval");
      stInstr = irBuilder.CreateStore(ldInstr, dstPtr);
    } else {
      auto memcpyInstr = irBuilder.CreateMemCpy(dstPtr, srcPtr, ConstantExpr::getSizeOf(ty), 0);
      ldInstr = memcpyInstr;
      stInstr = memcpyInstr;
    }
  }

  auto domainSpace = stmtCtx->getDomainSpace();
  if (ldInstr && src.isAnnotated())
    addLoadAccess(src.getDiscriminator(), src.getCoord().castDomain(domainSpace), ldInstr);
  if (stInstr && dst.isAnnotated())
    addStoreAccess(dst.getDiscriminator(), dst.getCoord().castDomain(domainSpace), stInstr);
}


llvm::MemCpyInst *MollyCodeGenerator::codegenAssignPtrPtr(llvm::Value *dstPtr, llvm::Value *dstBase, isl::Map dstIndex, llvm::Value *srcPtr, llvm::Value *srcBase, isl::Map srcIndex) {
  auto instr = codegenAssignPtrPtr(dstPtr, srcPtr);
  if (dstIndex.isValid())
    addStoreAccess(dstBase, dstIndex, instr);
  else
    addScalarStoreAccess(dstBase, instr);
  if (srcIndex.isValid())
    addLoadAccess(srcBase, srcIndex, instr);
  else
    addScalarLoadAccess(srcBase, instr);
  return instr;
}


void MollyCodeGenerator::codegenAssignPtrVal(llvm::Value *dstPtr, llvm::Value *srcVal) {
  assert(dstPtr);
  assert(srcVal);
  irBuilder.CreateStore(srcVal, dstPtr);
}

void MollyCodeGenerator::codegenAssignValPtr(llvm::Value *dstVal, llvm::Value *srcPtr) {
  assert(dstVal);
  assert(srcPtr);
  auto newVal = irBuilder.CreateLoad(srcPtr);
  dstVal->replaceAllUsesWith(newVal);
}
void MollyCodeGenerator::codegenAssignValVal(llvm::Value *dstVal, llvm::Value *srcVal) {
  assert(dstVal);
  assert(srcVal);
  dstVal->replaceAllUsesWith(srcVal);
}


void MollyCodeGenerator::codegenAssignScalarFromLocal(llvm::Value *dstPtr, FieldVariable *srcFvar, isl::PwMultiAff srcWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff srcIndex/* [domain] -> field[indexset] */) {
  assert(dstPtr);
  assert(srcFvar);

  auto srcPtr = codegenLoadLocalPtr(srcFvar, srcWhere, srcIndex);

  assert(isa<AllocaInst>(dstPtr));
  codegenAssignPtrPtr(dstPtr, dstPtr, isl::Map(), srcPtr, srcFvar->getVariable(), srcIndex);
}


AnnotatedPtr MollyCodeGenerator::codegenFieldLocalPtr(FieldVariable *srcFvar, isl::PwMultiAff srcWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff srcIndex/* [domain] -> field[indexset] */) {
  auto srcPtr = codegenLoadLocalPtr(srcFvar, srcWhere, srcIndex);
  return AnnotatedPtr::createFieldPtr(srcPtr, srcFvar, srcIndex);
}


llvm::CallInst *MollyCodeGenerator::callFieldRankof(FieldVariable *fvar, llvm::ArrayRef<llvm::Value *> coords) {
  auto nDims = coords.size();

  SmallVector<Value *, 4> args;
  args.push_back(fvar->getVariable());
  for (auto coord : coords) {
    args.push_back(coord);
  }

  SmallVector<Type *, 4> tys;
  tys.push_back(args[0]->getType());
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    tys.push_back(coords[i]->getType());
  }

  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_field_rankof, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callLocalIndexof(FieldVariable *fvar, llvm::ArrayRef<llvm::Value *> coords) {
  auto nDims = coords.size();

  SmallVector<Value *, 4> args;
  args.push_back(fvar->getVariable());
  for (auto coord : coords) {
    args.push_back(coord);
  }

  SmallVector<Type *, 4> tys;
  tys.push_back(args[0]->getType());
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    tys.push_back(coords[i]->getType());
  }

  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_local_indexof, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}


llvm::CallInst *MollyCodeGenerator::callFieldInit(llvm::Value *field, llvm::MDNode *metadata) {
  if (!metadata) {
    metadata = llvm::MDNode::get(getLLVMContext(), ArrayRef<llvm::Value*>());
  }
  Value *args[] = { field, metadata };
  Type *tys[] = { args[0]->getType() };
  auto ptrFunc = Intrinsic::getDeclaration(getModule(), Intrinsic::molly_field_init, tys);
  return irBuilder.CreateCall(ptrFunc, args);
}


void MollyCodeGenerator::addScalarStoreAccess(llvm::Value *base, llvm::Instruction *instr) {
  auto accessFirstElement = stmtCtx->getDomainSpace().mapsTo(0/*1*/).createZeroMultiAff();
  addStoreAccess(base, accessFirstElement, instr);
}


void molly::MollyCodeGenerator::addStoreAccess(llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr) {
  stmtCtx->addMemoryAccess(polly::MemoryAccess::MUST_WRITE, base, accessRelation, instr);
}


void MollyCodeGenerator::addScalarLoadAccess(llvm::Value *base, llvm::Instruction *instr) {
  auto accessFirstElement = stmtCtx->getDomainSpace().mapsTo(0/*1*/).createZeroMultiAff();
  //stmtCtx->addMemoryAccess(polly::MemoryAccess::SCALARREAD, base, accessFirstElement, instr);
  addLoadAccess(base, accessFirstElement, instr);
}


void molly::MollyCodeGenerator::addLoadAccess(llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr) {
  stmtCtx->addMemoryAccess(polly::MemoryAccess::READ, base, accessRelation, instr);
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
  auto scopRegion = scopCtx->getRegion();
  if (!scopRegion->contains(instr))
    return false;

  // As special exception, something that dominates all ScopStmts also is not concerned by shuffling the ScopStmts' order
  // This is meant for AllocaInsts we put there
  //if (instr->getParent() == scopRegion->getEntry())
  //  return false;

  return true;
}


llvm::Value *MollyCodeGenerator::getScalarAlloca(llvm::Value *val) {
  val = val->stripPointerCasts();
  if (auto alloca = dyn_cast<AllocaInst>(val)) {
    // It is a alloca itself, use aggregate semantics 
    return alloca;
  }

  // Is this value loading the scalar; if yes, it's scalar location is obviously where it has been loaded from
  if (auto load = dyn_cast<LoadInst>(val)) {
    auto ptr = load->getPointerOperand();
    // auto allocaInst = dyn_cast<AllocaInst>(ptr);
    if (!isDependent(ptr))
      return ptr;
  }

  // Look where IndependentBlocks stored the val
  for (auto itUse = val->user_begin(), endUse = val->user_end(); itUse != endUse; ++itUse) {
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
  //auto pass = stmtCtx->asPass();
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
  Value *target = nullptr;
  if (auto load = dyn_cast<LoadInst>(val)) {
    auto ptr = load->getPointerOperand();
    if (!isDependent(ptr))
      target = ptr;
  }

  // If the value is stored somewhere, load it from the same location
  if (!target) {
    // Maybe the value is stored already
    for (auto useIt = instr->user_begin(), useEnd = instr->user_end(); useIt != useEnd; ++useIt) {
      auto user = *useIt;
      if (auto storeUse = dyn_cast<StoreInst>(user)) {
        auto ptr = storeUse->getPointerOperand();
        if (isa<AllocaInst>(ptr)) {
          target = ptr;
          break;
        }
      }
    }
  }

  // No store location found, add a new one
  if (!target) {
    target = getScalarAlloca(val);
  }

  // Otherwise, insert a store ourselves
  if (!target) {
    target = allocStackSpace(val->getType());
    auto store = new StoreInst(val, target, instr->getNextNode());
    auto producerBB = instr->getParent();
    auto producerStmt = scopCtx->getStmtForBlock(producerBB);

    // Add a store MemoryAccess to THE OTHER stmt
    auto accessFirstElement = enwrap(producerStmt->getDomain()).getSpace().mapsTo(0/*1*/).createZeroMultiAff();
    producerStmt->addAccess(polly::MemoryAccess::MUST_WRITE, target, accessFirstElement.toMap().take(), store);
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

  if (ptr) { // !ptr should mean that val is not used outside the current ScopStmt
    auto store = irBuilder.CreateStore(val, ptr);
    addScalarStoreAccess(ptr, store);
  }
  //TODO: If necessary, iterate through all uses and replace those in this BasicBlock
  //toupdate->replaceAllUsesWith(val);
}


isl::Ctx *molly::MollyCodeGenerator::getIslContext() {
  return stmtCtx->getIslContext();
}


llvm::StoreInst * molly::MollyCodeGenerator::createArrayStore(llvm::Value *val, llvm::Value *baseptr, int idx) {
  auto &llvmContext = getLLVMContext();
  auto intTy = Type::getInt64Ty(llvmContext);
  auto islctx = getIslContext();

  auto domainSpace = stmtCtx->getDomainMultiAff().getRangeSpace();
  auto idxval = ConstantInt::get(intTy, idx);
  auto idxaff = domainSpace.createConstantAff(idx);
  return createArrayStore(val, baseptr, idxval, idxaff);
}


llvm::CallInst *molly::MollyCodeGenerator::callBeginMarker(StringRef str) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);
  auto strTy = PointerType::getUnqual(Type::getInt8Ty(llvmContext));
  Type *argTys[] = { strTy };
  auto funcDecl = getRuntimeFunc("__molly_begin_marker", voidTy, argTys);

  auto strConst = ConstantDataArray::getString(llvmContext, str);
  auto strGlob = new GlobalVariable(*getModule(), strConst->getType(), true, GlobalValue::PrivateLinkage, strConst);

  llvm::Value *args[] = { irBuilder.CreatePointerCast(strGlob, strTy) };
  return irBuilder.CreateCall(funcDecl, args);
}


llvm::CallInst *molly::MollyCodeGenerator::callEndMarker(StringRef str) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);
  auto strTy = PointerType::getUnqual(Type::getInt8Ty(llvmContext));
  Type *argTys[] = { strTy };
  auto funcDecl = getRuntimeFunc("__molly_end_marker", voidTy, argTys);

  auto strConst = ConstantDataArray::getString(llvmContext, str);
  auto strGlob = new GlobalVariable(*getModule(), strConst->getType(), true, GlobalValue::PrivateLinkage, strConst);

  llvm::Value *args[] = { irBuilder.CreatePointerCast(strGlob, strTy) };
  return irBuilder.CreateCall(funcDecl, args);
}

string compatName(StringRef arg) {
  SmallString<255> result;
  result.reserve(arg.size());

  int pos = 0;
  while (pos < arg.size()) {
    auto c = arg[pos];
    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '.') || (c == '_'))
      result.push_back(c);
    else if (pos>0 && (('0' <= c && c <= '9') || (c == '-')))
      result.push_back(c);
    pos += 1;
  }

  return result.str();
}


void molly::MollyCodeGenerator::markBlock(StringRef str) {
  if (!MollyMarkStmts)
    return;

  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);
  auto strTy = PointerType::getUnqual(Type::getInt8Ty(llvmContext));
  Type *argTys[] = { strTy };

  auto strConst = ConstantDataArray::getString(llvmContext, str);
  auto strGlob = new GlobalVariable(*getModule(), strConst->getType(), true, GlobalValue::PrivateLinkage, strConst, compatName(str));
  llvm::Value *args[] = { irBuilder.CreatePointerCast(strGlob, strTy) };

  auto bb = irBuilder.GetInsertBlock();
  irBuilder.SetInsertPoint(bb, bb->begin());
  auto beginDecl = getRuntimeFunc("__molly_begin_marker", voidTy, argTys);
  irBuilder.CreateCall(beginDecl, args);

  auto term = bb->getTerminator();
  if (term)
    irBuilder.SetInsertPoint(bb, term);
  else
    irBuilder.SetInsertPoint(bb, bb->end());
  auto endDecl = getRuntimeFunc("__molly_end_marker", voidTy, argTys);
  irBuilder.CreateCall(endDecl, args);
}


void molly::MollyCodeGenerator::markBlock(StringRef str, isl::MultiPwAff coord) {
  auto &llvmContext = getLLVMContext();
  auto voidTy = Type::getVoidTy(llvmContext);
  auto intTy = Type::getInt64Ty(llvmContext);
  auto intPtrTy = PointerType::getUnqual(intTy);
  auto strTy = PointerType::getUnqual(Type::getInt8Ty(llvmContext));
  Type *argTys[] = { strTy, intTy, intPtrTy };

  auto strConst = ConstantDataArray::getString(llvmContext, str);
  auto strGlob = new GlobalVariable(*getModule(), strConst->getType(), true, GlobalValue::PrivateLinkage, strConst, compatName(str));

  auto nDims = coord.getOutDimCount();
  auto arrayTy = ArrayType::get(intTy, nDims);
  //SmallVector<llvm::Value*, 4> coords;

  auto bb = irBuilder.GetInsertBlock();
  irBuilder.SetInsertPoint(bb, bb->begin());

  auto coords = this->codegenMultiAff(coord);
  auto space = this->allocStackSpace(intTy, nDims);
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    irBuilder.CreateStore(coords[i], irBuilder.CreateConstGEP1_64(space, i));
  }
  llvm::Value *args[] = { irBuilder.CreatePointerCast(strGlob, strTy), ConstantInt::get(intTy, nDims), space };

  auto beginDecl = getRuntimeFunc("__molly_begin_marker_coord", voidTy, argTys);
  irBuilder.CreateCall(beginDecl, args);

  auto term = bb->getTerminator();
  if (term)
    irBuilder.SetInsertPoint(bb, term);
  else
    irBuilder.SetInsertPoint(bb, bb->end());
  auto endDecl = getRuntimeFunc("__molly_end_marker_coord", voidTy, argTys);
  irBuilder.CreateCall(endDecl, args);
}


const llvm::DataLayout * molly::MollyCodeGenerator::getDataLayout() {
  auto DLP = &pass->getAnalysis<llvm::DataLayoutPass>();
  return &DLP->getDataLayout();
}


llvm::Value *molly::AnnotatedPtr::getDiscriminator() const {
  return fvar ? fvar->getVariable() : base;
}


void molly::AnnotatedPtr::dump() const {
  llvm::errs() << "AnnotatedPtr ";
  ptr->dump();
  if (isFieldPtr())
    llvm::errs() << " FIELD";
  if (isScalar())
    llvm::errs() << " SCALAR";
  if (coord.isValid()) {
    llvm::errs() << ' ';
    coord.dump();
  }
  llvm::errs() << '\n';
}
