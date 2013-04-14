#define DEBUG_TYPE "molly"
#include "molly/FieldDetection.h"

#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "molly/LinkAllPasses.h"
#include "MollyFieldAccess.h"

#include <llvm/IR/DerivedTypes.h>

#include "FieldVariable.h"
#include "FieldType.h"
#include "MollyContextPass.h"
#include "MollyContext.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
//#include <polly/MollyMeta.h>
#include <polly/ScopInfo.h>

using namespace llvm;
using namespace molly;

static cl::opt<bool> EnableFieldDetection("molly-field", cl::desc("Enable field detection"), cl::Hidden, cl::init(true));
static RegisterPass<FieldDetectionAnalysis> FieldAnalysisRegistration("molly-detect", "Molly - Field detection", false, true);

STATISTIC(NumGlobalFields, "Number of detected global fields");

char FieldDetectionAnalysis::ID = 0;
char &molly::FieldDetectionAnalysisID = FieldDetectionAnalysis::ID;

//static RegisterPass<FieldDetectionAnalysis> FieldDetectionRegistration("molly-detect", "Molly - Find fields", false, true);
//INITIALIZE_PASS_BEGIN(FieldDetectionAnalysis, "molly-detect", "Molly - Find fields", false, false)
//INITIALIZE_PASS_END(FieldDetectionAnalysis, "molly-detect", "Molly - Find fields", false, false)



void FieldDetectionAnalysis::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<MollyContextPass>();
  AU.setPreservesAll();
}


bool FieldDetectionAnalysis::runOnModule(Module &M) {
  mollyContext = getAnalysis<MollyContextPass>().getMollyContext();
  assert(mollyContext);

  auto &glist = M.getGlobalList();
  auto &flist = M.getFunctionList();
  auto &alist = M.getAliasList();
  auto &mlist = M.getNamedMDList();
  auto &vlist = M.getValueSymbolTable();

  auto fieldsMD = M.getNamedMetadata("molly.fields"); 
  if (fieldsMD) {
    auto numFields = fieldsMD->getNumOperands();

    for (unsigned i = 0; i < numFields; i+=1) {
      auto fieldMD = fieldsMD->getOperand(i);
      FieldType *field = FieldType::createFromMetadata(mollyContext, &M, fieldMD);
      fieldTypes[field->getType()] = field;
    }
  }
  return false;
}


void FieldDetectionAnalysis::releaseMemory() {
  for (auto it = fieldVars .begin(), end = fieldVars.end(); it != end; ++it) {
    auto fvar = it->second;
    delete fvar;
  }
  fieldVars.clear();

  for (auto it = fieldTypes.begin(), end = fieldTypes.end(); it != end; ++it) {
    auto fty = it->second;
    delete fty;
  }
  fieldTypes.clear();
}


FieldType *FieldDetectionAnalysis::lookupFieldType(llvm::StructType *ty) {
  auto result = fieldTypes.find(ty);
  if (result == fieldTypes.end())
    return NULL;
  return result->second;
}


FieldType *FieldDetectionAnalysis::getFieldType(llvm::StructType *ty) {
  auto result = lookupFieldType(ty);
  if (result)
    return result;
  result = FieldType::create(mollyContext, ty); // TODO: Don't create here, all field types should have been added to llvm.fields NamedMD
  fieldTypes[ty] = result;
  return result;
}


FieldType *FieldDetectionAnalysis::getFieldType(llvm::Value *val) {
  auto ty = val->getType();
  if (auto pty = dyn_cast<PointerType>(ty)) {
    ty = pty->getPointerElementType();
  }
  return getFieldType(llvm::cast<llvm::StructType>(ty));
}


FieldVariable *FieldDetectionAnalysis::lookupFieldVariable(GlobalVariable *gvar) {
  auto res = fieldVars.find(gvar);
  if (res == fieldVars.end())
    return NULL;
  return res->second;
}


FieldVariable *FieldDetectionAnalysis::getFieldVariable(GlobalVariable *gvar) {
  auto &res = fieldVars[gvar];
  if (!res) {
    auto fieldTy = getFieldType(cast<StructType>(gvar->getType()->getPointerElementType()));
    res = FieldVariable::create(gvar, fieldTy);
  }
  return res;
}


FieldType *FieldDetectionAnalysis::getFromFunction(Function *func) {
  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_ptr")) {
    auto fieldParm = &func->getArgumentList().front();
    return getFieldType(cast<StructType>( fieldParm->getType()));
  }

  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_get")) {
    assert(!"Not yet implemented");
  }

  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_set")) {
    assert(!"Not yet implemented");
  }

  return NULL;
}


FieldVariable *FieldDetectionAnalysis::getFromCall(const CallInst *callInst) {
  Function *func = callInst->getCalledFunction();
  if (!func) {
    // Field accessors are not virtual
    return NULL;
  }

  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_ptr")) {
    auto nArgs = callInst->getNumArgOperands();
    assert(nArgs >= 2); // At least reference to field and one coordinate
    auto field = callInst->getArgOperand(0);

    auto gfield = cast<GlobalVariable>(field); // Non-global fields not supported at the moment
    return getFieldVariable(gfield);
  }

  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_get")) {
    assert(!"Not yet implemented");
  }

  if (func->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_set")) {
    assert(!"Not yet implemented");
  }

  return NULL;
}


FieldVariable *FieldDetectionAnalysis::getFromAccess(Instruction *inst) {
  if (auto ld = dyn_cast<LoadInst>(inst)) {
    auto ptr = ld->getPointerOperand();
    if (auto call = dyn_cast<CallInst>(ptr))
      return getFromCall(call);
  } else if (auto st = dyn_cast<StoreInst>(inst)) {
    auto ptr = st->getPointerOperand();
    if (auto call = dyn_cast<CallInst>(ptr))
      return getFromCall(call);
  } else if (auto call = dyn_cast<CallInst>(inst)) {
    return getFromCall(call);
  }

  std::nullptr_t x = NULL;
  // Not a field accessor
  return NULL;
}


MollyFieldAccess FieldDetectionAnalysis::getFieldAccess(const llvm::Instruction *instr) {
  auto result = MollyFieldAccess::fromAccessInstruction(const_cast<Instruction*>(instr));
  if (result.isNull())
    return MollyFieldAccess();
  auto base = result.getBaseField();
  result.fieldvar = getFieldVariable(dyn_cast<GlobalVariable>(base));
  return result;
}


MollyFieldAccess FieldDetectionAnalysis::getFieldAccess(polly::MemoryAccess *memacc) {
  auto instr = memacc->getAccessInstruction();
  auto result = getFieldAccess(instr);
  if (!result.isValid())
    return result;

  result.setScopAccess(memacc);
  return result;
}


ModulePass *molly::createFieldDetectionAnalysisPass() {
  return new FieldDetectionAnalysis();
}
