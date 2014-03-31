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

  auto desc = (Twine("local initbuf ") + bufptr->getName()).str();
  codegen.callBeginMarker(desc);

  auto currentNode = funcCtx->getCurrentNodeCoordinate(); // { [] -> rank[cluster] }
  auto size = shape->codegenSize(codegen, currentNode);
  //auto eltTy = bufptr->getType()->getPointerElementType()->getPointerElementType();

  auto dl = codegen.getDataLayout();
  auto ptrSize = dl->getPointerSizeInBits();
  auto intTy = Type::getIntNTy(codegen.getLLVMContext(), ptrSize);

  auto allocCall = codegen.callCombufLocalAlloc(size, ConstantExpr::getSizeOf(eltTy));
 // auto mallocCall = CallInst::CreateMalloc(irBuilder.GetInsertPoint(), intTy, eltTy, irBuilder.CreateZExtOrTrunc(size, intTy));
  //irBuilder.SetInsertPoint(mallocCall->getParent(), mallocCall->getNextNode());
  irBuilder.CreateStore(allocCall, bufptr);

  codegen.callEndMarker(desc);
}
