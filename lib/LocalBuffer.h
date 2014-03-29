#ifndef MOLLY_LOCALBUFFER_H
#define MOLLY_LOCALBUFFER_H

#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"


namespace molly {

  class LocalBuffer {
    llvm::GlobalVariable *bufptr;
    RectangularMapping *shape;

  protected:
    LocalBuffer(llvm::GlobalVariable *bufptr, RectangularMapping *shape) : bufptr(bufptr), shape(shape) {}

  public:
    static LocalBuffer *create(llvm::GlobalVariable *bufptr, RectangularMapping *shape) {
      return new LocalBuffer(bufptr,shape);
    }

    llvm::GlobalVariable *getGlobalVariable() const {
      return bufptr;
    }

    void codegenInit(MollyCodeGenerator &codegen, MollyPassManager *pm, MollyFunctionProcessor *funcCtx);
  }; // class LocalBuffer

} // namespace molly

#endif /* MOLLY_LOCALBUFFER_H */
