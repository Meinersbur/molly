#ifndef MOLLY_MOLLYSCOPSTMTPROCESSOR_H
#define MOLLY_MOLLYSCOPSTMTPROCESSOR_H

#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include <map>
#include <vector>
#include "polly/ScopInfo.h"

namespace {
  class MollyScopStmtProcessorImpl;
}

namespace molly {

  class MollyScopStmtProcessor {
  protected:
#ifndef NDEBUG
    MollyScopStmtProcessorImpl *pimpl;
    explicit MollyScopStmtProcessor(MollyScopStmtProcessorImpl *pimpl) : pimpl(pimpl) {}
#else
    explicit MollyScopStmtProcessor(MollyScopStmtProcessorImpl *pimpl) {}
#endif

  public:
    virtual ~MollyScopStmtProcessor() {}
    virtual void dump() const = 0;
    virtual void validate() const = 0;

    // Queries
    virtual llvm::LLVMContext &getLLVMContext() const = 0;
    virtual isl::Ctx *getIslContext() const = 0;
    virtual llvm::BasicBlock *getBasicBlock() = 0;
    virtual polly::ScopStmt *getStmt() = 0;
    virtual llvm::Pass *asPass() = 0;
    virtual molly::MollyPassManager *getPassManager() = 0;
    virtual molly::MollyScopProcessor *getScopProcessor() const = 0;
    virtual const llvm::Region *getRegion() = 0;

    // { stmt[domain] }
    virtual isl::Set getDomain() const = 0;
    virtual isl::Space getDomainSpace() const = 0;
    virtual isl::Set getDomainWithNamedDims() const = 0;

    // { stmt[domain] -> scattering[scatter] }
    virtual isl::Map getScattering() const = 0;
    virtual isl::PwMultiAff getScatteringAff() const = 0;

    // is this ScopStmt an access to a field?
    virtual bool isFieldAccess() const = 0;
    virtual bool isReadAccess() const = 0;
    virtual bool isWriteAccess() const = 0;

    virtual FieldVariable *getFieldVariable() const = 0;
    virtual FieldType *getFieldType() const = 0;

    virtual llvm::Instruction *getAccessor() = 0;
    virtual llvm::LoadInst *getLoadAccessor() = 0;
    virtual llvm::StoreInst *getStoreAccessor() = 0;

    virtual llvm::Value *getAccessedCoordinate(unsigned i) = 0;
    virtual isl::MultiPwAff getAccessed() = 0; // { [domain] -> [indexset] }

    // { stmt[domain] -> node[cluster] }
    virtual isl::Map getWhere() const = 0;
    virtual void setWhere(isl::Map instances) = 0;
    virtual void addWhere(isl::Map newInstances) = 0;
    virtual isl::Map getInstances() const = 0; // Like getWhere(), but domain is exact

    // { stmt[domain] -> field[index] }
    virtual isl::Map getAccessRelation() const = 0;

    // Related code code generation
    virtual std::map<isl_id *, llvm::Value *> &getIdToValueMap() = 0;

    virtual llvm::Value *getDomainValue(unsigned i) = 0;
    virtual const llvm::SCEV *getDomainSCEV(unsigned i) = 0;
    virtual isl::Id getDomainId(unsigned i) = 0;
    virtual isl::Aff getDomainAff(unsigned i) = 0;

    virtual std::vector<llvm::Value *> getDomainValues() = 0;
    virtual isl::MultiAff getDomainMultiAff() = 0;
    virtual isl::MultiAff getClusterMultiAff() = 0;

    //virtual void identifyDomainDims() = 0;
    virtual MollyCodeGenerator makeCodegen() = 0;
    virtual MollyCodeGenerator makeCodegen(llvm::Instruction *insertBefore) = 0;
    virtual StmtEditor getEditor() = 0;
    virtual void addMemoryAccess(polly::MemoryAccess::AccessType type, const llvm::Value *base, isl::Map accessRelation, llvm::Instruction *accInstr) = 0;

    // Process
    virtual void applyWhere() = 0;

  public:
    static MollyScopStmtProcessor *create(MollyPassManager *pm, polly::ScopStmt *scop);
  }; // class MollyScopStmtProcessor

} // namespace molly
#endif /* MOLLY_MOLLYSCOPSTMTPROCESSOR_H */
