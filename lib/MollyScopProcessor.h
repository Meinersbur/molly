#ifndef MOLLY_MOLLYSCOPPROCESSOR_H
#define MOLLY_MOLLYSCOPPROCESSOR_H

#include <llvm/Support/Compiler.h>
#include "molly/Mollyfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include <vector>
#include <map>
#include <llvm/ADT/SmallVector.h>


namespace molly {

  class MollyScopProcessor {
  protected:
    MollyScopProcessor() {}
    /* implicit */ MollyScopProcessor(const MollyScopProcessor &that) LLVM_DELETED_FUNCTION;

  public:
    virtual void dump() const = 0;
    virtual void validate() const = 0;

    virtual const llvm::SCEV *getClusterCoordinate(unsigned i) = 0;
    virtual std::vector<const llvm::SCEV *> getClusterCoordinates() = 0;
    virtual isl::MultiAff getCurrentNodeCoordinate() = 0;

    virtual llvm::Region *getRegion() = 0;
    virtual llvm::Function *getParentFunction() = 0;

    virtual bool hasFieldAccess() = 0;
    virtual llvm::Pass *asPass() = 0;

    virtual void computeScopDistibution() = 0;
    virtual void genCommunication() = 0;
    virtual void pollyCodegen() = 0;

    //virtual std::map<isl_id *, llvm::Value *> *getValueMap() = 0;
    //virtual polly::ScopPass *asPass() = 0;

    //virtual std::vector<isl::Id> getAllIds();

    virtual isl::Space getParamsSpace() = 0;
    virtual const llvm::SmallVector<const llvm::SCEV *, 8> &getParamSCEVs() = 0;

    // Lowering: isl::Id -> llvm::SCEV* -> llvm::Value*
    virtual llvm::Value *codegenScev(const llvm::SCEV *scev, llvm::Instruction *insertBefore) = 0;
    //virtual llvm::Value *allocStackSpace(llvm::Type *ty);

    // Raising: llvm::Value* -> llvm::SCEV* -> isl::Id
    virtual const llvm::SCEV *scevForValue(llvm::Value *value) = 0;
    virtual isl::Id idForSCEV(const llvm::SCEV *scev) = 0;
    virtual isl::Id getIdForLoop(const llvm::Loop *loop) = 0;

    virtual polly::ScopStmt *getStmtForBlock(llvm::BasicBlock *bb) = 0;

  public:
    static MollyScopProcessor *create(MollyPassManager *pm, polly::Scop *scop);
  }; // class MollyScopProcessor

} // namespace molly
#endif /* MOLLY_MOLLYSCOPPROCESSOR_H */
