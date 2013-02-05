#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

#include "llvm/ADT/SmallVector.h"

namespace llvm {
  class Module;
  class MDNode;
  class StructType;
  class LLVMContext;
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
    llvm::MDNode *metadata;

    llvm::StructType *ty;
    llvm::SmallVector<uint32_t, 4> lengths;

  protected:
    FieldType(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      assert(mollyContext);
      assert(module);
      assert(metadata);

      this->mollyContext = mollyContext;
      this->module = module;
      this->metadata = metadata;
      
      if (metadata)
        readMetadata();
    }

    void readMetadata();

    isl::Ctx *getIslContext();

  public:
    static FieldType *createFromMetadata(molly::MollyContext *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(mollyContext, module, metadata);
    }

    void dump();

    uint32_t getDimensions() { 
      return lengths.size(); 
    }

    isl::BasicSet getIndexset();

    llvm::StructType *getType() { 
      assert(ty);
      return ty;
    }

  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
