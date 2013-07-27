#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

//#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType
#include <clang/CodeGen/MollyFieldMetadata.h> // FieldTypeMetadata (member of FieldType)
#include <llvm/ADT/ArrayRef.h>
#include "islpp/Space.h"
#include "islpp/MultiAff.h"
#include "islpp/Islfwd.h"

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
  class Space;
} // namespace isl

namespace molly {
  class MollyContextPass;
} // namespace molly


namespace molly {

  class FieldType {

  private:
    //molly::MollyContextPass *mollyContext;
    isl::Ctx *islctx;
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
    FieldType(isl::Ctx *islctx, llvm::Module *module, llvm::MDNode *metadata);

    void readMetadata();

    isl::Ctx *getIslContext() const;
    llvm::LLVMContext *getLLVMContext();
    llvm::Module *getModule();

  public:
    ~FieldType();

    static FieldType *createFromMetadata(isl::Ctx *islctx, /*molly::MollyContextPass *mollyContext,*/ llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(islctx,/*mollyContext,*/ module, metadata);
    }

    void dump();

    unsigned getNumDimensions() const {
      return metadata.dimLengths.size();
    }

    isl::BasicSet getLogicalIndexset();
    isl::Space getLogicalIndexsetSpace();
    isl::Id getIndexsetTuple() const; // for global/logical indexset
    isl::Space getIndexsetSpace() const; 

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
    llvm::Function *getFuncSetLocal() {
      assert(metadata.funcSetLocal);
    return metadata.funcSetLocal;
    }

    llvm::Function *getFuncPtrLocal() {
      assert(metadata.funcPtrLocal);
      return metadata.funcPtrLocal;
    }

    //llvm::Function *localOffsetFunc;
    //llvm::Function *getLocalOffsetFunc() { return localOffsetFunc; }
    //void setLocalOffsetFunc(llvm::Function *func) { assert(func); this->localOffsetFunc = func; }

    llvm::Function *localLengthFunc;
    llvm::Function *getLocalLengthFunc() { return localLengthFunc; }
    void setLocalLengthFunc(llvm::Function *func) { assert(func); this->localLengthFunc = func; }

    //llvm::Function *islocalFunc;
    llvm::Function *getIslocalFunc() { assert(metadata.funcIslocal); return metadata.funcIslocal; }
    //void setIslocalFunc(llvm::Function *func) { assert(func); this->islocalFunc = func; }

    //llvm::Function *ptrFunc;
    //llvm::Function *getPtrFunc() { return ptrFunc; }
    //void setPtrFunc(llvm::Function *func) { assert(func); this->ptrFunc = func; }

    llvm::Type *getEltType();
    llvm::PointerType *getEltPtrType();

    //TODO: Element length in bytes

    isl::Set getGlobalIndexset();
    int getGlobalLength(int d);

    isl::Map getLocalIndexset(const isl::BasicSet &clusterSet); // { clusterCoordinate -> Indexset }

    /// { globalcoord -> nodecoord } where the value is stored
    isl::PwMultiAff getHomeAff(); 
    isl::Map getHomeRel(); /* { cluster[nodecoord] -> fty[indexset] } which coordnated are stored at these nodes */

    llvm::StringRef getName() const;
  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
