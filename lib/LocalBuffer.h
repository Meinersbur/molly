#ifndef MOLLY_LOCALBUFFER_H
#define MOLLY_LOCALBUFFER_H

#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"

#include <cassert>


namespace molly {

  class LocalBuffer {
    llvm::GlobalVariable *bufptr;
    RectangularMapping *shape;
    llvm::Type *eltTy;

  protected:
    LocalBuffer(llvm::GlobalVariable *bufptr, RectangularMapping *shape, llvm::Type *eltTy) : bufptr(bufptr), shape(shape), eltTy(eltTy) {
      assert(bufptr);
      assert(shape);
      assert(eltTy);
    }

  public:
    static LocalBuffer *create(llvm::GlobalVariable *bufptr, RectangularMapping *shape, llvm::Type *eltTy) {
      return new LocalBuffer(bufptr,shape,eltTy);
    }

    llvm::GlobalVariable *getGlobalVariable() const {
      return bufptr;
    }

    void codegenInit(MollyCodeGenerator &codegen, MollyPassManager *pm, MollyFunctionProcessor *funcCtx);
  }; // class LocalBuffer

} // namespace molly

#endif /* MOLLY_LOCALBUFFER_H */
