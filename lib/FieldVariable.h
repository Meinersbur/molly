#ifndef MOLLY_FIELDVARIABLE_H
#define MOLLY_FIELDVARIABLE_H 1


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

  public:
    static FieldVariable *create(llvm::GlobalVariable *variable, FieldType *fieldTy) {
      return new FieldVariable(variable, fieldTy);
    }

    void dump();

    llvm::GlobalVariable *getVariable() { return variable; }
    FieldType *getFieldType() { return fieldTy; }
  }; /* class FieldVariable */

} /* namespace molly */
#endif /* MOLLY_FIELDVARIABLE_H */
