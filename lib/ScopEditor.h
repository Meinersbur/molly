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
#include "molly/Mollyfwd.h"


namespace molly {
  class ScopEditor;
  class StmtEditor;

  /// Somehow analogous to IRBuilder, but also deletes stuff
  class ScopEditor {
  private:
    polly::Scop *scop;
    llvm::Pass *pass;

  protected:
    template<typename AnalysisType>
    AnalysisType *getAnalysisIfAvailable() const { 
      if (!pass)
        return nullptr;
      return pass->getAnalysisIfAvailable<AnalysisType>();
    }

  public:
    explicit ScopEditor(polly::Scop *scop): scop(scop), pass(nullptr) { assert(scop); }
    explicit ScopEditor(polly::Scop *scop, llvm::Pass *pass): scop(scop), pass(pass) { assert(scop); }
    static ScopEditor create(polly::Scop *scop) { return ScopEditor(scop); }
    static ScopEditor create(polly::Scop *scop, llvm::Pass *pass) { return ScopEditor(scop, pass); }

    static ScopEditor newScop(llvm::Instruction *insertBefore, llvm::FunctionPass *p);

    polly::Scop *getScop() { return scop; }

    llvm::Function *getParentFunction();
    llvm::Module *getParentModule();
    llvm::LLVMContext &getLLVMContext();

    isl::Id getScatterTupleId();
    isl::Space getScatterSpace();

    void getParamsMap(std::map<isl_id *, llvm::Value *> &params, polly::ScopStmt *stmt);// TODO: Move to StmtEditor

    isl::Space getParamSpace();
    //unsigned getNumParamDims();
    //llvm::Value *getParamDimValue(unsigned pos);
    //isl::Id getParamDimId(unsigned pos);

    StmtEditor createStmt(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name);
    



    /// Create a ScopStmt that are all executed before/after another stmt
    /// Params:
    ///   model     Execute before/after this statement
    ///   subdomain Subset of model's domain; determines how many instances are to be executed
    ///   atLevel   The depth of model's scattering at which to execute the new stmt; i.e.
    ///             0=Put all instances at the beginning/end of the scop; 
    ///             1=Put all instances before/after the topmost statement containing model
    ///             2=Put theinstances at the beginning/ending of the outmost loop that contains model
    ///             etc.
    ///   relative  Constant distance in scattering space at atLevel to put between model and created ScopStmt; it's the caller's responsability to ensure that no other stmt is at that position. For instance, create extra dimensions or multiply the scatterings of the existing ones to ensure some distance
    ///             < 0: Put before model
    ///             = 0: Execute in parallel
    ///             > 0: Put after model
    //StmtEditor createStmtRelative(polly::ScopStmt *model, const isl::Set &subdomain, unsigned atLevel, int relative, const isl::Map &where, const std::string &name);

    //StmtEditor createStmtRelative(polly::ScopStmt *model, const isl::Map &subscattering, int relative, const isl::Map &where, const std::string &name);
    //StmtEditor createStmtBefore(polly::ScopStmt *model, const isl::Map &subscattering, const isl::Map &where, const std::string &name);
    //StmtEditor createStmtAfter(polly::ScopStmt *model, const isl::Map &subscattering, const isl::Map &where, const std::string &name) ;

    //TODO: move to StmtEditor
    StmtEditor replaceStmt(polly::ScopStmt *model, isl::Map &&whereToReplace, const std::string &name);
  }; // class ScopEditor


  class StmtEditor {
    polly::ScopStmt *stmt;
    llvm::Pass *pass;

  public:
    explicit StmtEditor(polly::ScopStmt *stmt): stmt(stmt), pass(nullptr) { assert(stmt); }
    StmtEditor(polly::ScopStmt *stmt,  llvm::Pass *pass): stmt(stmt), pass(pass) { assert(stmt); }
    static StmtEditor create(polly::ScopStmt *stmt) { return StmtEditor(stmt); }
    static StmtEditor create(polly::ScopStmt *stmt,  llvm::Pass *pass) { return StmtEditor(stmt, pass); }

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

    /// { stmt[domain] -> stmt[domain] }
    /// Trivial mapping to the vector of induction variables
    /// To be used in code generation
    isl::MultiAff getCurrentIteration();

    /// { stmt[domain] -> cluster[coord] }
    isl::Map getWhere(); 
    isl::Map getInstances();
    void setWhere(isl::Map where);
    /// remove all instances not in keepInsts from domain and wheremap, such that this stmt is less often executed
    void restrictInstances(const isl::Map &keepInsts);
    void removeInstances(const isl::Map &disableInsts);

    llvm::TerminatorInst *getTerminator();


    void remove();

    void addWriteAccess(llvm::StoreInst *instr, FieldVariable *fvar, isl::Map &&accessRelation);

    /// Change the domain of this stmt
    /// Information on the old domain will be preserved using the given map
    /// The map should better be right-unique (ie a function) and injective
    /// A new environment will be created suitable for the new domain
    //void changeDomain(const isl::Map &oldToNew);

    /// Like ScopEditor::createStmt, but reuse the loops of model instead of createing new one
    /// As a result, the domain must be a subset of model's
    StmtEditor createStmt(const isl::Set &subdomain, const isl::Map &scatter, const isl::Map &where, const std::string &name);

    /// Remove instances from this ScopStmt and create a new ScopStmt that is executed instead
    StmtEditor replaceStmt(const isl::Map &subwhere, const std::string &name);

  }; // class StmtEditor

} // namespace molly
#endif /* MOLLY_SCOPEDITOR_H */
