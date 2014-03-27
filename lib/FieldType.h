#ifndef MOLLY_FIELDTYPE_H
#define MOLLY_FIELDTYPE_H

//#include <llvm/ADT/SmallVector.h> // SmallVector member of FieldType
#include <clang/CodeGen/MollyFieldMetadata.h> // FieldTypeMetadata (member of FieldType); TODO: There shouldn't be any reference to clang, i.e. move the XYZMetadata classes to LLVM, although they are not used there (like DIBuilder)
#include <llvm/ADT/ArrayRef.h>
#include "LLVMfwd.h"
#include "molly/Mollyfwd.h"
#include "islpp/Islfwd.h"
#include "islpp/Id.h"
#include "islpp/Space.h"
#include "llvm/ADT/SmallVector.h"

namespace isl {
  class Ctx;
  class Set;
  class BasicSet;
  class Map;
  class Space;
} // namespace isl

namespace molly {
  class MollyContextPass;
  class FieldLayout;
} // namespace molly


namespace molly {

  /// Fields of same FieldType are logically assignable
  /// Therefore, the logical indexsets and element type match
  /// In future implementations the their layouts may be different
  class FieldType {
  private:
    isl::Ctx *islctx;
    llvm::Module *module;

    clang::CodeGen::FieldTypeMetadata metadata;

    isl::Space clusterSpace;

    /// Layouts that occur in the program for this field type
    /// Thats is, if fields are passed by reference, which are the layouts the SCoP must be prepared for
    /// NOT SUPPORTED YET
    /// We need to add a discriminator to identify the layout a field currently has
    /// Choices:
    /// - The index in this vector
    /// - The address to the index computation function
    std::vector<FieldLayout*> layouts;

    /// Without #pragma molly transform, this should be the standard layout
    FieldLayout *defaultLayout;

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



    llvm::ArrayRef<int> getLengths() {
      assert(metadata.dimLengths.size() >= 1);
      return metadata.dimLengths;
    }
  
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


    llvm::Function *localLengthFunc;
    llvm::Function *getLocalLengthFunc() { return localLengthFunc; }
    void setLocalLengthFunc(llvm::Function *func) { assert(func); this->localLengthFunc = func; }

    llvm::Function *getIslocalFunc() { assert(metadata.funcIslocal); return metadata.funcIslocal; }

    llvm::Type *getEltType() const;
    llvm::PointerType *getEltPtrType() const;

    //TODO: Element length in bytes

    isl::Set getGlobalIndexset();
    int getGlobalLength(unsigned d);

    //isl::Map getLocalIndexset(const isl::BasicSet &clusterSet); // { clusterCoordinate -> Indexset }

    /// { globalcoord -> nodecoord } where the value is stored
    //isl::PwMultiAff getHomeAff(); 
    //isl::Map getHomeRel(); /* { cluster[nodecoord] -> fty[indexset] } which coordnated are stored at these nodes */

    llvm::StringRef getName() const;

    /// In the current implementation, every type has just one layout
    /// TODO: This should be removed, layouts can be assigned per variable using #pragma molly transform; for some builtins we need a switch to select the applied layout
    FieldLayout *getDefaultLayout() { return defaultLayout; }
    void setDefaultLayout(FieldLayout *layout) { this->defaultLayout = layout; }
  }; // class FieldType
} // namespace molly
#endif /* MOLLY_FIELDTYPE_H */
