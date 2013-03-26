#ifndef MOLLY_MOLLYFIELDACCESS_H
#define MOLLY_MOLLYFIELDACCESS_H

#include <polly/FieldAccess.h>

namespace molly {
  class MollyFieldAccess : public polly::FieldAccess {
    friend class FieldDetectionAnalysis;
  private:
    FieldVariable *fieldvar;

  protected:
    MollyFieldAccess(const polly::FieldAccess &that) : polly::FieldAccess(that) { }

  public:
    MollyFieldAccess() {
      this->fieldvar = NULL;
    }

    static MollyFieldAccess fromAccessInstruction(llvm::Instruction *instr);

    FieldVariable *getFieldVariable() { return fieldvar; }
    FieldType *getFieldType();

    isl::Space getLogicalSpace(isl::Ctx *); // returns a set space with getNumDims() dimensions
  }; // class MollyFieldAccess

} // namespace molly

#endif /* MOLLY_MOLLYFIELDACCESS_H */
