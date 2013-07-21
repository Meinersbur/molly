#ifndef MOLLY_MOLLYFIELDACCESS_H
#define MOLLY_MOLLYFIELDACCESS_H

#include "islpp/MultiPwAff.h"
#include <polly/FieldAccess.h>

namespace isl {
  class Map;
} // namespace isl

namespace molly {
  class FieldDetectionAnalysis;
} // namespace molly


namespace molly {
  class MollyFieldAccess : public polly::FieldAccess {
    friend class FieldDetectionAnalysis;
  private:
    FieldVariable *fieldvar;

    polly::MemoryAccess *scopAccess;

  protected:
    MollyFieldAccess(const polly::FieldAccess &that) : polly::FieldAccess(that) {
      this->fieldvar = nullptr;
      this->scopAccess = nullptr; 
    }

   
  public:
    MollyFieldAccess() {
      this->fieldvar = nullptr;
      this->scopAccess = nullptr; 
    }

    static MollyFieldAccess fromAccessInstruction(llvm::Instruction *instr);
    static MollyFieldAccess create(llvm::Instruction *instr, polly::MemoryAccess *acc, FieldVariable *fvar);
    static MollyFieldAccess fromMemoryAccess(polly::MemoryAccess *acc, FieldDetectionAnalysis * = nullptr);

     void augmentFieldDetection(FieldDetectionAnalysis *fields);
    //void augmentMemoryAccess(polly::MemoryAccess *acc);
    //void augmentFieldVariable(FieldVariable *fieldvar);
    //void augmentScEv(llvm::ScalarEvolution *se) {}

    FieldVariable *getFieldVariable() { return fieldvar; }
    FieldType *getFieldType();

    isl::Space getLogicalSpace(isl::Ctx *); // returns a set space with getNumDims() dimensions

    polly::MemoryAccess *getPollyMemoryAccess() const;
    polly::ScopStmt *getPollyScopStmt();
    isl::MultiPwAff getAffineAccess(llvm::ScalarEvolution *se);
    isl::Map/*iteration coord -> field coord*/ getAccessRelation();
    isl::Set getAccessedRegion();

    // Currently this is taken from polly::MemoryAccess
    // But in the lang term, we want to get it from molly::FieldType
    isl::Space getIndexsetSpace();

    isl::Id getAccessTupleId() const;


    // { memacc[domain] -> scattering[scatter] }
    // Mapped to the same location as the Stmt's scattering
    // Later could also add another dimension to represent the order of the access relative to others
    isl::Map getAccessScattering() const;
  }; // class MollyFieldAccess

} // namespace molly

#endif /* MOLLY_MOLLYFIELDACCESS_H */
