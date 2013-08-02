#ifndef MOLLY_SCOPEDITOR_H
#define MOLLY_SCOPEDITOR_H
/// Tools for changing statements in SCoPs

#include "islpp/Islfwd.h"
#include "Pollyfwd.h"
#include <llvm/ADT/ArrayRef.h>
#include <string>
#include "LLVMfwd.h"
#include <map>
#include <llvm/IR/InstrTypes.h>

namespace llvm {
  class BasicBlock;
  class Region;
  class Loop;
} // namespace llvm


namespace molly {
  class ScopEditor;
  class StmtEditor;

  /// Somehow analogous to IRBuilder, but also deletes stuff
  class ScopEditor {
  private:
    polly::Scop *scop;

    llvm::LLVMContext &getLLVMContext();
    llvm::Module *getParentModule();
    llvm::Function *getParentFunction();

  public:
    ScopEditor(polly::Scop *scop): scop(scop) { assert(scop); }
    static ScopEditor newScop(llvm::Instruction *insertBefore, llvm::FunctionPass *p);

    polly::Scop *getScop() { return scop; }

    void getParamsMap(std::map<isl_id *, llvm::Value *> &params, polly::ScopStmt *stmt);

    polly::ScopStmt *createBlock(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name);
    StmtEditor createStmt(isl::Set &&domain, isl::Map &&scattering, isl::Map &&where, const std::string &name);

    StmtEditor replaceStmt(polly::ScopStmt *model, isl::Map &&whereToReplace, const std::string &name);
  }; // class ScopEditor


  class StmtEditor {
    polly::ScopStmt *stmt;

  public:
    StmtEditor(polly::ScopStmt *stmt): stmt(stmt) { assert(stmt); }

    polly::ScopStmt *getStmt() {return stmt;}
    llvm::BasicBlock *getBasicBlock();
    //llvm::instr

    llvm::TerminatorInst *getTerminator();

    std::vector<llvm::Value*> getDomainValues();
    //void finish();
  }; // class StmtEditor


  polly::ScopStmt *replaceScopStmt(polly::ScopStmt *model, llvm::BasicBlock *bb, const std::string &baseName, isl::Map &&);
  polly::ScopStmt *createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering);

} // namespace molly
#endif /* MOLLY_SCOPEDITOR_H */
