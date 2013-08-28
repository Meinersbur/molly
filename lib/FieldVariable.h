#ifndef MOLLY_FIELDVARIABLE_H
#define MOLLY_FIELDVARIABLE_H

#include "islpp/Islfwd.h"

namespace llvm {
  class Module;
  class GlobalVariable;
  class MDNode;
  class CallInst;
  class Value;
  class Instruction;
}


namespace molly {
  class IslBasicSet;
  class FieldType;


  class FieldVariable {
  private:
    llvm::GlobalVariable *variable;
    FieldType *fieldTy;

  protected:
    FieldVariable(llvm::GlobalVariable *variable, FieldType *fieldTy);

    isl::Ctx *getIslContext() const;

  public:
    static FieldVariable *create(llvm::GlobalVariable *variable, FieldType *fieldTy) {
      return new FieldVariable(variable, fieldTy);
    }

    void dump();

    llvm::GlobalVariable *getVariable() { return variable; }
    FieldType *getFieldType() { return fieldTy; }

    isl::Id getTupleId();
    isl::Space getAccessSpace();
  }; /* class FieldVariable */

} /* namespace molly */
#endif /* MOLLY_FIELDVARIABLE_H */
