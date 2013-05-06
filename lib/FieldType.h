#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

//#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType
#include <clang/CodeGen/MollyFieldMetadata.h> // FieldTypeMetadata (member of FieldType)
#include <llvm/ADT/ArrayRef.h>

namespace llvm {
  class Module;
  class MDNode;
  class StructType;
  class PointerType;
  class LLVMContext;
  class Function;
  namespace CodeGen {
    class FieldTypeMetadata;
  } // namespace CodeGen
}

namespace isl {
  class Ctx;
  class Set;
  class BasicSet;
}

namespace molly {
  class MollyContext;
}


namespace molly {
  class FieldType {
  private:
    molly::MollyContext *mollyContext;
    llvm::Module *module;

    clang::CodeGen::FieldTypeMetadata metadata;

    //llvm::MDNode *metadata; //TODO: Only used during construction

    // The LLVM type that represents this field
    //llvm::StructType *ty;

    /// Logical (global) shape
    //LengthsType lengths;
    //isl::Set shape;

    // Local shape(s)
    bool isdistributed;
    llvm::SmallVector<int, 4> localLengths;
    //isl::Set localShape

    //llvm::Function *reffunc;
    //llvm::Function *islocalfunc;

  protected:
    FieldType(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      assert(mollyContext);
      assert(module);
      assert(metadata);

      this->mollyContext = mollyContext;
      this->module = module;

      localOffsetFunc = NULL;
      localLengthFunc = NULL;
      islocalFunc = NULL;

      isdistributed = false;
      this->metadata.readMetadata(module, metadata);
    }

    void readMetadata();

    isl::Ctx *getIslContext();
    llvm::LLVMContext *getLLVMContext();
    llvm::Module *getModule();

  public:
    ~FieldType();

    static FieldType *createFromMetadata(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(mollyContext, module, metadata);
    }

    static FieldType *create(molly::MollyContext *mollyContext, llvm::StructType *ty, llvm::MDNode *metadata = NULL) {
      return new FieldType(mollyContext, NULL, metadata);
    }

    void dump();

    unsigned getNumDimensions() {
      return metadata.dimLengths.size();
    }

    isl::Set getLogicalIndexset();

    llvm::StructType *getType() { 
      assert(metadata.llvmType);
      return metadata.llvmType;
    }

    //llvm::Function *getRefFunc();
    //llvm::Function *getIsLocalFunc();
    //void emitIsLocalFunc();


    bool isDistributed() {
      return isdistributed;
    }
    llvm::ArrayRef<int> getLengths() {
      assert(metadata.dimLengths.size() >= 1);
      return metadata.dimLengths;
    }
    llvm::ArrayRef<int> getLocalLengths() {
      if (!isDistributed())
        return getLengths();
      return localLengths;
    }

    void setDistributed(bool val = true) {
      isdistributed = val;
    }
    void setLocalLength(const llvm::ArrayRef<int> &lengths) {
      this->localLengths.clear();
      this->localLengths.append(lengths.begin(), lengths.end());
    }

    llvm::Function *getFuncGetBroadcast() {
      assert(metadata.funcGetBroadcast);
      return metadata.funcGetBroadcast;
    }
    llvm::Function *getFuncSetBroadcast() {
      assert(metadata.funcSetBroadcast);
      return metadata.funcSetBroadcast;
    }

    llvm::Function *localOffsetFunc;
    llvm::Function *getLocalOffsetFunc() { return localOffsetFunc; }
    void setLocalOffsetFunc(llvm::Function *func) { assert(func); this->localOffsetFunc = func; }

    llvm::Function *localLengthFunc;
    llvm::Function *getLocalLengthFunc() { return localLengthFunc; }
    void setLocalLengthFunc(llvm::Function *func) { assert(func); this->localLengthFunc = func; }

    llvm::Function *islocalFunc;
    llvm::Function *getIslocalFunc() { assert(metadata.funcIslocal); return metadata.funcIslocal; }
    //void setIslocalFunc(llvm::Function *func) { assert(func); this->islocalFunc = func; }

    llvm::Function *ptrFunc;
    llvm::Function *getPtrFunc() { return ptrFunc; }
    void setPtrFunc(llvm::Function *func) { assert(func); this->ptrFunc = func; }

    llvm::Type *getEltType() {
    	assert(metadata.llvmEltType);
    	return metadata.llvmEltType;
    }
    llvm::PointerType *getEltPtrType();
  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
