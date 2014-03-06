#ifndef MOLLY_FIELDLAYOUT_H
#define MOLLY_FIELDLAYOUT_H

#pragma region includes & forward declarations
#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"

#include "islpp/Islfwd.h"
#include "islpp/Map.h"
#pragma endregion


namespace molly {

  /// Represents how the data of a FieldType can be laid out
  /// That is, distribution around nodes and local ordering within the node
  class FieldLayout {
    FieldType *fty;

    // Functions for the runtime
    // TODO: Remove; replace with intrinsics that generate these
  public:
    llvm::Function *rankoffunc;
    llvm::Function *localidxfunc;
  private:

    /// The main relation
    /// { logical[indexset] -> (physical[cluster], physical[local]) }
    /// TODO: Want to chain them, also non-affine mappings like space-filling curves and indirect indexing
    isl::Map relation;
    isl::PwMultiAff relationAff; //TODO: For compatibility; Code should be able to handle multiple locations

    /// To make a single, ordered index out of a local coordinate
    /// Inputs: physical[cluster], physical[local]
    /// Output: ordinal
    /// Note that it is possible that one logical index has _multiple_ physical locations
    RectangularMapping *linearizer;

  protected:
    FieldLayout(FieldType *fty, isl::PwMultiAff relationAff, /*take*/ RectangularMapping *linearizer) : fty(fty), relation(relationAff), relationAff(relationAff), linearizer(linearizer) {
      assert(fty);
      assert(linearizer);
    }

  public:
    ~FieldLayout();

    /// Create a new value distribution
    /// It it specific to one logical indexset and cluster shape
    static FieldLayout *create(FieldType *fty, ClusterConfig *clusterConf, isl::PwMultiAff relation);

    FieldType *getFieldType() { return fty; }

    /// { cluster[nodecoord] -> fty[indexset] } 
    /// which coordinates are stored at these nodes 
    isl::PwMultiAff getHomeAff() const {
      return relationAff.sublist(relation.getRangeSpace().wrap().getDomainSpace());
    }

    llvm::Value *codegenLocalSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator);
    llvm::Value *codegenLocalMaxSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator);
    llvm::Value *codegenLocalIndex(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator, isl::MultiPwAff logicalCoord);
  }; // class FieldLayout

} // namespace molly
#endif /* MOLLY_FIELDLAYOUT_H */
