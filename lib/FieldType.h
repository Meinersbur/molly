#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType

namespace llvm {
  class Module;
  class MDNode;
  class StructType;
  class LLVMContext;
  class Function;
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
    typedef llvm::SmallVector<uint32_t, 4> LengthsType;
  private:
    molly::MollyContext *mollyContext;
    llvm::Module *module;
    llvm::MDNode *metadata; //TODO: Only used during construction

    // The LLVM type that represents this field
    llvm::StructType *ty;

    /// Logical (global) shape
    LengthsType lengths;
    //isl::Set shape;

    // Local shape(s)
    bool isdistributed;
    LengthsType localLengths;
    //isl::Set localShape

    llvm::Function *reffunc;
    llvm::Function *islocalfunc;

  protected:
    FieldType(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      assert(mollyContext);
      assert(module);
      assert(metadata);

      this->mollyContext = mollyContext;
      this->module = module;
      this->metadata = metadata;

      isdistributed = false;
      reffunc = NULL;
      islocalfunc = NULL;

      if (metadata)
        readMetadata();
    }

    void readMetadata();

    isl::Ctx *getIslContext();
    llvm::LLVMContext *getLLVMContext();
    llvm::Module *getModule();

  public:
    static FieldType *createFromMetadata(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(mollyContext, module, metadata);
    }

    static FieldType *create(molly::MollyContext *mollyContext, llvm::StructType *ty, llvm::MDNode *metadata = NULL) {
      return new FieldType(mollyContext, NULL, metadata);
    }

    void dump();

    uint32_t getDimensions() { 
      return lengths.size(); 
    }

    isl::Set getIndexset();

    llvm::StructType *getType() { 
      assert(ty);
      return ty;
    }

    llvm::Function *getRefFunc();
    llvm::Function *getIsLocalFunc();
    void emitIsLocalFunc();


    bool isDistributed() {
      return isdistributed;
    }
    LengthsType &getLengths() {
      return lengths;
    }

    void setDistributed(bool val = true) {
      isdistributed = val;
    }
    void setLocalLength(const llvm::SmallVectorImpl<uint32_t> &lengths) {
      this->localLengths.clear();
      this->localLengths.append(lengths.begin(), lengths.end());
    }

  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
