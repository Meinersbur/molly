#ifndef MOLLY_CODEGEN_H
#define MOLLY_CODEGEN_H

#include "LLVMfwd.h"
#include "Pollyfwd.h"
#include "islpp/Islfwd.h"
#include "molly/Mollyfwd.h"
#include <map>
#include <llvm/IR/IRBuilder.h>
#include "ScopEditor.h"


namespace molly {

  class MollyCodeGenerator {
  private:
    MollyScopStmtProcessor *stmtCtx;
    DefaultIRBuilder irBuilder;
    //StmtEditor stmtEditor;
    //llvm::Pass *pass;

  protected:
    std::map<isl_id *, llvm::Value *> &getIdToValueMap();

  public:
    MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore) : stmtCtx(stmtCtx), irBuilder(insertBefore) {}
    MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx);

    DefaultIRBuilder &getIRBuilder() { return irBuilder; }
    StmtEditor getStmtEditor();


    //llvm::Value *codegenAff(const isl::Aff &aff);
    llvm::Value *codegenAff(const isl::PwAff &aff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::MultiAff &maff);
    std::vector<llvm::Value *> codegenMultiAff(const isl::MultiPwAff &maff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::PwMultiAff &maff);


    llvm::Value *codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);

    void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, isl::Map accessRelation);
    void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::MultiPwAff index);
  }; // class MollyCodeGenerator

} // namespace molly
#endif /* MOLLY_CODEGEN_H */
