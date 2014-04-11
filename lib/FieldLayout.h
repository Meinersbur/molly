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
    //isl::PwMultiAff relationAff; //TODO: For compatibility; Code should be able to handle multiple locations

    /// For every element selects a node that can be used as data source if the element is not local
    /// { logical[indexset] -> physical[cluster] }
    // TODO: select a primary remote for every node, so traffic is load-balanced
    isl::PwMultiAff primary;

    /// For every node, the physical coordinate
    /// { logical[indexset] -> physical[local] }
    isl::PwMultiAff localPhysical;

    /// To make a single, ordered index out of a local coordinate
    /// Inputs: physical[cluster], physical[local]
    /// Output: ordinal
    /// Note that it is possible that one logical index has _multiple_ physical locations
    RectangularMapping *linearizer;

  protected:
    FieldLayout(FieldType *fty, isl::Map relation, /*take*/ RectangularMapping *linearizer) : rankoffunc(nullptr), localidxfunc(nullptr), fty(fty), relation(relation), linearizer(linearizer) {
      assert(fty);
      assert(linearizer);

      // FIXME: There are way better algorithms for choosing a primary
      this->primary = relation.uncurry().domain().unwrap().anyElement();
      this->localPhysical = relation.uncurry().toPwMultiAff(); // No multiple location on the same node supported yet
    }

  public:
    ~FieldLayout();

    /// Create a new value distribution
    /// It it specific to one logical indexset and cluster shape
    static FieldLayout *create(FieldType *fty, ClusterConfig *clusterConf, isl::Map relation);

    FieldType *getFieldType() { return fty; }

    /// { cluster[nodecoord] -> fty[indexset] } 
    /// which coordinates are stored at these nodes
    // TODO: obsolete; use getPhysicalNode() instead
    //isl::PwMultiAff getHomeAff() const {
    //  return relation.toPwMultiAff().sublist(relation.getRangeSpace().unwrap().getDomainSpace());
    //}

    isl::PwMultiAff getPrimaryPhysicalNode() const { return primary; }

    isl::Space getLogicalIndexsetSpace() const;

    isl::Space getPhysicalNodeSpace() const {
      return relation.getRangeSpace().unwrap().getDomainSpace();
    }

    isl::Space getPhysicalLocalSpace() const {
      return relation.getRangeSpace().unwrap().getRangeSpace();
    }

    isl::Map getPhysicalNode() const {
      return relation.wrap().reorganizeSubspaces(getLogicalIndexsetSpace(), getPhysicalNodeSpace());
    }

    isl::PwMultiAff getPhysicalLocal() const {
      //return relation.wrap().reorganizeSubspaces(getLogicalIndexsetSpace(), getPhysicalLocalSpace());
      return localPhysical;
    }

    llvm::Value *codegenLocalSize(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator);
    llvm::Value *codegenLocalMaxSize(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator);
    llvm::Value *codegenLocalIndex(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator, isl::MultiPwAff logicalCoord);

    /// { cluster[nodecoord] -> fty[indexset] }
    /// Difference to other getPhysicalXYZ() is that it also returns the elements that are not contained in this->relation, but memory is allocated for because of overapproximation
    // Disabled because it doesn't handle localPhysical
    //isl::Map getIndexableIndices() const;
  }; // class FieldLayout

} // namespace molly
#endif /* MOLLY_FIELDLAYOUT_H */
