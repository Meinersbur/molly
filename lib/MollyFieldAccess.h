#ifndef MOLLY_MOLLYFIELDACCESS_H
#define MOLLY_MOLLYFIELDACCESS_H

#include <polly/FieldAccess.h>

namespace isl {
  class Map;
  class MultiAff;
} // namespace isl


namespace molly {
  class MollyFieldAccess : public polly::FieldAccess {
    friend class FieldDetectionAnalysis;
  private:
    FieldVariable *fieldvar;

    polly::MemoryAccess *scopAccess;

  protected:
    MollyFieldAccess(const polly::FieldAccess &that) : polly::FieldAccess(that) { this->fieldvar = NULL; this->scopAccess = NULL; }

  public:
    MollyFieldAccess() {
      this->fieldvar = NULL;
    }

    static MollyFieldAccess fromAccessInstruction(llvm::Instruction *instr);
    static MollyFieldAccess fromMemoryAccess(polly::MemoryAccess *acc);

    void augmentMemoryAccess(polly::MemoryAccess *acc);

    FieldVariable *getFieldVariable() { return fieldvar; }
    FieldType *getFieldType();

    isl::Space getLogicalSpace(isl::Ctx *); // returns a set space with getNumDims() dimensions

    polly::MemoryAccess *getPollyMemoryAccess();
    polly::ScopStmt *getPollyScopStmt();
    isl::MultiAff getAffineAccess();
    isl::Map getAccessedRegion();
  }; // class MollyFieldAccess

} // namespace molly

#endif /* MOLLY_MOLLYFIELDACCESS_H */
