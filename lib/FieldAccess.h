#ifndef MOLLY_FIELDACCESS_H
#define MOLLY_FIELDACCESS_H

namespace llvm {
  class Instruction;
  class CallInst;
  class Function;
  class Value;
} // namespace llvm

namespace molly {
  class MollyContext;
  class FieldDetection;
  class FieldVariable;
  class FieldType;
} // namespace molly

namespace isl {
  class Aff;
} // namespace isl


namespace molly {
  class FieldAccess {
    //MollyContext *mollyContext;
    //FieldDetection *detector;
    llvm::Instruction *accessor;
    llvm::CallInst *fieldCall;
    FieldVariable *fieldvar;
    bool reads;
    bool writes;

  public:
    FieldAccess() { this->accessor = nullptr; }
    FieldAccess(llvm::Instruction *accessor, llvm::CallInst *fieldCall, FieldVariable *fieldvar, bool isRead, bool isWrite);

    bool isValid() { return accessor; }
    llvm::Instruction *getAccessor() {return accessor;}
    llvm::CallInst *getFieldCall() { return fieldCall; }
    llvm::Function *getFieldFunc();
    bool isRefCall();
    FieldVariable *getFieldVariable() { return fieldvar; }
    FieldType *getFieldType();
    bool isRead() { return reads; }
    bool isWrite() { return writes; }

    llvm::Value *getSubscript();//TODO: Multi-dimensional
  }; // class FieldAccess
} // namepsace molly
#endif /* MOLLY_FIELDACCESS_H */