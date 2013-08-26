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

  /// A utility class to embrace anything that is to know about acessing a field
  /// Polly requires a subset of this so it knows what are field accesses and what they access
  /// deprecated; use MollyScopStmtProcessor instead
  class MollyFieldAccess : public polly::FieldAccess {
    friend class FieldDetectionAnalysis;
  private:
    FieldVariable *fieldvar;

    polly::MemoryAccess *scopAccess;
    polly::ScopStmt *scopStmt;

  protected:
    MollyFieldAccess(const polly::FieldAccess &that) : polly::FieldAccess(that) {
      this->fieldvar = nullptr;
      this->scopAccess = nullptr; 
      this->scopStmt  = nullptr;
    }

  private:
    void clear() {
      this->fieldvar = nullptr;
      this->scopAccess = nullptr; 
      this->scopStmt  = nullptr;
    }


  public:
    MollyFieldAccess() {
      clear();
    }

    void loadFromInstruction(llvm::Instruction *instr) LLVM_OVERRIDE {
      clear();
      FieldAccess::loadFromInstruction(instr);
    }

    MollyFieldAccess(llvm::Instruction *instr, polly::MemoryAccess *acc, polly::ScopStmt * scopStmt, FieldVariable *fvar) {
      loadFromInstruction(instr);

      this->fieldvar = nullptr;
      this->scopAccess = acc;
      this->scopStmt = scopStmt;
    }

  public:
    void loadFromInstruction(llvm::Instruction *instr, FieldVariable *fvar)  { loadFromInstruction(instr); this->fieldvar = fvar; }
    void loadFromMemoryAccess(polly::MemoryAccess *acc, FieldVariable * =nullptr);
    void loadFromScopStmt(polly::ScopStmt *stmt, FieldVariable * =nullptr);
    void setFieldVariable(FieldVariable *fvar) { assert(fvar); assert(!this->fieldvar); this->fieldvar= fvar; }

    static MollyFieldAccess fromAccessInstruction(llvm::Instruction *instr, FieldVariable * =nullptr);
    //static MollyFieldAccess create(llvm::Instruction *instr, polly::MemoryAccess *acc, polly::ScopStmt * scopStmt, FieldVariable *fvar);
    static MollyFieldAccess fromMemoryAccess(polly::MemoryAccess *acc, FieldVariable * =nullptr);
    static MollyFieldAccess fromScopStmt(polly::ScopStmt *acc, FieldVariable * =nullptr);

    //void augmentFieldDetection(FieldDetectionAnalysis *fields);
    //void augmentMemoryAccess(polly::MemoryAccess *acc);
    //void augmentFieldVariable(FieldVariable *fieldvar);
    //void augmentScEv(llvm::ScalarEvolution *se) {}

    FieldVariable *getFieldVariable() { assert(fieldvar); return fieldvar; }
    FieldType *getFieldType() const;

    isl::Space getLogicalSpace(isl::Ctx *); // returns a set space with getNumDims() dimensions

    polly::MemoryAccess *getPollyMemoryAccess() const;
    polly::ScopStmt *getPollyScopStmt() const;
    isl::MultiPwAff getAffineAccess(llvm::ScalarEvolution *se);
    isl::Map/*iteration coord -> field coord*/ getAccessRelation() const;
    isl::Set getAccessedRegion();

    // Currently this is taken from polly::MemoryAccess
    // But in the lang term, we want to get it from molly::FieldType
    isl::Space getIndexsetSpace();

    isl::Id getAccessTupleId() const;


    // { memacc[domain] -> scattering[scatter] }
    // Mapped to the same location as the Stmt's scattering
    // Later could also add another dimension to represent the order of the access relative to others
    isl::Map getAccessScattering() const;

    isl::PwMultiAff getHomeAff() const;

    llvm::StoreInst *getLoadUse() const;
  }; // class MollyFieldAccess

} // namespace molly

#endif /* MOLLY_MOLLYFIELDACCESS_H */
