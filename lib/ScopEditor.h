#ifndef MOLLY_SCOPEDITOR_H
#define MOLLY_SCOPEDITOR_H
/// Tools for changing statements in SCoPs

#include "islpp/Islfwd.h"
#include "Pollyfwd.h"
#include "LLVMfwd.h"
#include <map>
#include <llvm/IR/InstrTypes.h>
#include <llvm/ADT/ArrayRef.h>
#include <string>
#include <polly/ScopPass.h>


namespace molly {
  class ScopEditor;
  class StmtEditor;

  /// Somehow analogous to IRBuilder, but also deletes stuff
  class ScopEditor {
  private:
    polly::Scop *scop;
    polly::ScopPass *pass;

  protected:
    template<typename AnalysisType>
    AnalysisType *getAnalysisIfAvailable() const { 
      if (!pass)
        return nullptr;
      return pass->getAnalysisIfAvailable<AnalysisType>();
    }

  public:
    explicit ScopEditor(polly::Scop *scop): scop(scop), pass(nullptr) { assert(scop); }
    explicit ScopEditor(polly::Scop *scop, polly::ScopPass *pass): scop(scop), pass(pass) { assert(scop); }
    static ScopEditor create(polly::Scop *scop) { return ScopEditor(scop); }
    static ScopEditor create(polly::Scop *scop, polly::ScopPass *pass) { return ScopEditor(scop, pass); }

    static ScopEditor newScop(llvm::Instruction *insertBefore, llvm::FunctionPass *p);

    polly::Scop *getScop() { return scop; }

    llvm::Function *getParentFunction();
    llvm::Module *getParentModule();
    llvm::LLVMContext &getLLVMContext();

    isl::Id getScatterTupleId();
    isl::Space getScatterSpace();

    void getParamsMap(std::map<isl_id *, llvm::Value *> &params, polly::ScopStmt *stmt);// TODO: Move to StmtEditor

    //polly::ScopStmt *createBlock(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name);
    StmtEditor createStmt(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name);
    StmtEditor replaceStmt(polly::ScopStmt *model, isl::Map &&whereToReplace, const std::string &name);
  }; // class ScopEditor


  class StmtEditor {
    polly::ScopStmt *stmt;

  public:
    explicit StmtEditor(polly::ScopStmt *stmt): stmt(stmt) { assert(stmt); }
    static StmtEditor create(polly::ScopStmt *stmt) { return StmtEditor(stmt); }

    polly::Scop *getParentScop();
    ScopEditor getParentScopEditor();

    llvm::Function *getParentFunction();
    llvm::Module *getParentModule();
    llvm::LLVMContext &getLLVMContext();

    polly::ScopStmt *getStmt() { return stmt; }
    llvm::BasicBlock *getBasicBlock();

    /// { stmt[] }
    isl::Space getDomainSpace();
    isl::Id getDomainTupleId();

    /// { stmt[domain] }
    isl::Set getIterationDomain();
    void setIterationDomain(isl::Set &&domain);

    /// { scattering[] }
    isl::Space getScatterSpace();

    /// { stmt[domain] -> scattering[scatter] }
    isl::Map getScattering();
    void setScattering(isl::Map &&scatter);

    /// { stmt[domain] -> cluster[coord] }
    isl::Map getWhere(); 
    void setWhere(isl::Map &&where);

    llvm::TerminatorInst *getTerminator();

    std::vector<llvm::Value*> getDomainValues();
  }; // class StmtEditor


  //polly::ScopStmt *replaceScopStmt(polly::ScopStmt *model, llvm::BasicBlock *bb, const std::string &baseName, isl::Map &&);
  //polly::ScopStmt *createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering);

} // namespace molly
#endif /* MOLLY_SCOPEDITOR_H */
