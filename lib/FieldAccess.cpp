#include "FieldAccess.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include "MollyContext.h"
#include "FieldVariable.h"
#include "islpp/Aff.h"

using namespace molly;
using namespace llvm;
using namespace std;

FieldAccess::FieldAccess(llvm::Instruction *accessor, llvm::CallInst *fieldCall, FieldVariable *fieldvar, bool isRead, bool isWrite) { 
  assert(accessor);
  assert(fieldCall);
  assert(fieldvar);
 //this->mollyContext = mollyContext; 
 // this->detector = detector;
  this->accessor = accessor;
  this->fieldCall = fieldCall;
  this->fieldvar = fieldvar;
  this->reads = isRead;
  this->writes = isWrite;
}

llvm::Function *FieldAccess::getFieldFunc()  {
  return getFieldCall()->getCalledFunction();
}

bool FieldAccess::isRefCall() {
  return getFieldFunc()->getAttributes().hasAttribute(AttributeSet::FunctionIndex, "molly_ref");
}

FieldType *FieldAccess::getFieldType() {
  return fieldvar->getFieldType();
}


llvm::Value *FieldAccess::getSubscript() {
  if (isRefCall())  {
    auto call = getFieldCall();
    assert(call->getNumArgOperands() == 2); // Arg [0] is 'this'-pointer
    auto arg = call->getArgOperand(1);
    return arg;


  }


}
