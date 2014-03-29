#include "LocalBuffer.h"

#include "MollyFunctionProcessor.h"
#include "RectangularMapping.h"
#include "Codegen.h"

//#include <llvm\IR\Intrinsics.h>
#include <llvm/IR/GlobalVariable.h>

//using namespace polly;
using namespace llvm;
using namespace std;



void molly::LocalBuffer::codegenInit(MollyCodeGenerator &codegen, MollyPassManager *pm, MollyFunctionProcessor *funcCtx) {
  auto &irBuilder = codegen.getIRBuilder();

 auto currentNode = funcCtx->getCurrentNodeCoordinate(); // { [] -> rank[cluster] }
 auto size = shape->codegenSize(codegen, currentNode); 
 auto eltTy = bufptr->getType()->getPointerElementType()->getPointerElementType();

 auto dl = codegen.getDataLayout();
 auto ptrSize = dl->getPointerSize();
 auto intTy = Type::getIntNTy(codegen.getLLVMContext(), ptrSize);

 auto mallocCall = CallInst::CreateMalloc(irBuilder.GetInsertPoint(), intTy, eltTy, size);
 irBuilder.SetInsertPoint(mallocCall->getParent(), mallocCall->getNextNode());
 irBuilder.CreateStore(mallocCall, bufptr);
}
