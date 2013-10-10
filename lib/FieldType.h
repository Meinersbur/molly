#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

//#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType
#include <clang/CodeGen/MollyFieldMetadata.h> // FieldTypeMetadata (member of FieldType)
#include <llvm/ADT/ArrayRef.h>
#include "LLVMfwd.h"
#include "molly/Mollyfwd.h"
#include "islpp/Islfwd.h"
#include "islpp/Id.h"
#include "islpp/Space.h"

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
    //llvm::DataLayout *datalayout;
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
    //isl::Id clusterTupleId;
    isl::Space clusterSpace;
    //isl::Set localShape

    //llvm::Function *reffunc;
    //llvm::Function *islocalfunc;

    FieldLayout *layout;

  protected:
    FieldType(isl::Ctx *islctx, llvm::Module *module, llvm::MDNode *metadata);

    void readMetadata();
    llvm::LLVMContext *getLLVMContext();

  public:
    ~FieldType();

    static FieldType *createFromMetadata(isl::Ctx *islctx, llvm::Module *module, llvm::MDNode *metadata) {
      return new FieldType(islctx, module, metadata);
    }

    isl::Ctx *getIslContext() const;
    void dump();

    size_t getNumDimensions() const {
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

    llvm::Module *getModule() const { return module; }

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
    int getLocalLength(unsigned d) {
      assert(0 <= d && d < getNumDimensions());
      return localLengths[d];
    }

    void setDistributed(bool val = true) {
      isdistributed = val;
    }
    void setLocalLength(const llvm::ArrayRef<int> &lengths, isl::Space clusterSpace) {
      this->localLengths.clear();
      this->localLengths.append(lengths.begin(), lengths.end());
      this->clusterSpace = clusterSpace.move();
    }
    //void setDistribution(const llvm::ArrayRef<int> &localLengths )
    //isl::Map getDistributionMapping(); /* global coordinate -> node coordinate */
    //isl::MultiAff getDistributionAff(); /* global coordinate -> node coordinate */

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

    llvm::Type *getEltType() const;
    llvm::PointerType *getEltPtrType() const;

    //TODO: Element length in bytes

    isl::Set getGlobalIndexset();
    int getGlobalLength(unsigned d);

    isl::Map getLocalIndexset(const isl::BasicSet &clusterSet); // { clusterCoordinate -> Indexset }

    /// { globalcoord -> nodecoord } where the value is stored
    isl::PwMultiAff getHomeAff(); 
    isl::Map getHomeRel(); /* { cluster[nodecoord] -> fty[indexset] } which coordnated are stored at these nodes */

    //isl::MultiAff getLocalIdxAff();

    llvm::StringRef getName() const;

    //uint64_t getEltSize() const;

    //llvm::CallInst *callLocalPtrIntrinsic(llvm::Value *fieldvar, llvm::ArrayRef<llvm::Value> indices, llvm::Instruction *insertBefore = nullptr);

    FieldLayout *getLayout() { return layout; }
    void setLayout(FieldLayout *layout) { this->layout = layout; }
  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
