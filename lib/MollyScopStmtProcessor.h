#ifndef MOLLY_MOLLYSCOPSTMTPROCESSOR_H
#define MOLLY_MOLLYSCOPSTMTPROCESSOR_H

#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include <map>
#include <vector>


namespace molly {

  class MollyScopStmtProcessor {
  public:
    // Queries
    virtual llvm::BasicBlock *getBasicBlock() = 0;
    virtual polly::ScopStmt *getStmt() = 0;
    virtual llvm::Pass *asPass() = 0;

    // is this ScopStmt an access to a field?
    virtual bool isFieldAccess() const = 0;
    //virtual isl::MultiPwAff getAccessIndex() = 0;

    // Related code code generation
    virtual std::map<isl_id *, llvm::Value *> &getIdToValueMap() = 0;
    virtual std::vector<llvm::Value *> getDomainValues() = 0;
    virtual MollyCodeGenerator makeCodegen() = 0;

    // Process
    virtual void applyWhere() = 0;

  public:
    static MollyScopStmtProcessor *create(MollyPassManager *pm, polly::ScopStmt *scop);
    virtual void dump() = 0;
  }; // class MollyScopStmtProcessor

} // namespace molly
#endif /* MOLLY_MOLLYSCOPSTMTPROCESSOR_H */
