#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

#include "llvm/ADT/SmallVector.h"

namespace llvm {
  class Module;
  class MDNode;
  class StructType;
  class LLVMContext;
}

namespace molly {
  class IslSet;
  class MollyContext;


  class FieldType {
  private:
    molly::MollyContext *mollyContext;
    llvm::Module *module;
    llvm::MDNode *metadata;

    llvm::StructType *ty;
    llvm::SmallVector<uint32_t, 4> lengths;

  protected:
    FieldType(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      assert(mollyContext);
      assert(module);
      assert(metadata);

      this->module = module;
      this->metadata = metadata;
      
      if (metadata)
        readMetadata();
    }

    void readMetadata();

  public:
    static FieldType *createFromMetadata(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(mollyContext, module, metadata);
    }

    void dump() {
      // Nothing yet
    }

    uint32_t getDimensions() { 
      return lengths.size(); 
    }

    IslSet &getIndexset();

    llvm::StructType *getType() { 
      assert(ty);
      return ty;
    }

  }; /* class FieldType */

} /* namespace molly */

#endif /* MOLLY_FIELDTYPE_H */
