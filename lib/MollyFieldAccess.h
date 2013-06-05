#ifndef MOLLY_MOLLYFIELDACCESS_H
#define MOLLY_MOLLYFIELDACCESS_H

#include "islpp/MultiPwAff.h"
#include <polly/FieldAccess.h>

namespace isl {
  class Map;
} // namespace isl


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
    static MollyFieldAccess fromMemoryAccess(polly::MemoryAccess *acc);

    void augmentMemoryAccess(polly::MemoryAccess *acc);
    void augmentFieldVariable(FieldVariable *fieldvar);
    //void augmentScEv(llvm::ScalarEvolution *se) {}

    FieldVariable *getFieldVariable() { return fieldvar; }
    FieldType *getFieldType();

    isl::Space getLogicalSpace(isl::Ctx *); // returns a set space with getNumDims() dimensions

    polly::MemoryAccess *getPollyMemoryAccess();
    polly::ScopStmt *getPollyScopStmt();
    isl::MultiPwAff getAffineAccess(llvm::ScalarEvolution *se);
    isl::Map/*iteration coord -> field coord*/ getAccessRelation();
    isl::Set getAccessedRegion();
  }; // class MollyFieldAccess

} // namespace molly

#endif /* MOLLY_MOLLYFIELDACCESS_H */
