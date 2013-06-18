#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

//#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType
#include <clang/CodeGen/MollyFieldMetadata.h> // FieldTypeMetadata (member of FieldType)
#include <llvm/ADT/ArrayRef.h>
#include "islpp/Space.h"
#include "islpp/MultiAff.h"

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
  class Map;
}

namespace molly {
  class MollyContextPass;
}


namespace molly {
  class FieldType {
  private:
    molly::MollyContextPass *mollyContext;
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
    FieldType(molly::MollyContextPass *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
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

    static FieldType *createFromMetadata(molly::MollyContextPass *mollyContext, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(mollyContext, module, metadata);
    }

    void dump();

    unsigned getNumDimensions() {
      return metadata.dimLengths.size();
    }

    isl::BasicSet getLogicalIndexset();
    isl::Space getLogicalIndexsetSpace();

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
    int getLocalLength(int d) {
      assert(d <= 0 && d < getNumDimensions());
      return localLengths[d];
    }

    void setDistributed(bool val = true) {
      isdistributed = val;
    }
    void setLocalLength(const llvm::ArrayRef<int> &lengths) {
      this->localLengths.clear();
      this->localLengths.append(lengths.begin(), lengths.end());
    }
    //void setDistribution(const llvm::ArrayRef<int> &localLengths )
    isl::Map getDistributionMapping(); /* global coordinate -> node coordinate */
    isl::MultiAff getDistributionAff(); /* global coordinate -> node coordinate */

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

    //TODO: Element length in bytes

    isl::Set getGlobalIndexset();
    int getGlobalLength(int d);

    isl::Map getLocalIndexset(const isl::BasicSet &clusterSet); // { clusterCoordinate -> Indexset }

    /// { globalcoord -> nodecoord } where the value is stored
    isl::PwMultiAff getHomeAff(); 
  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
