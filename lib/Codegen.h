#ifndef MOLLY_CODEGEN_H
#define MOLLY_CODEGEN_H

#include "LLVMfwd.h"
#include "Pollyfwd.h"
#include "islpp/Islfwd.h"
#include "molly/Mollyfwd.h"
#include <map>
#include <llvm/IR/IRBuilder.h>
#include "ScopEditor.h"
#include "Clangfwd.h"
#include "islpp/AstBuild.h"

namespace molly {

  /// Everything needed to generate code into a ScopStmt
  class MollyCodeGenerator {
  private:
    /// Stmt to generate
    MollyScopStmtProcessor *stmtCtx;

    /// At which position to insert the code
    DefaultIRBuilder irBuilder;
    
    /// ISL code generator for affine expressions
    isl::AstBuild astBuild;

    /// Required by polly::IslExprBuilder to map params and domain dims to variables that contain these values
    //std::map<isl_id *, llvm::Value *> idToValue;

  protected:
    std::map<isl_id *, llvm::Value *> &getIdToValueMap();

    llvm::Module *getModule();
    clang::CodeGen::MollyRuntimeMetadata *getRtMetadata();

    isl::AstBuild &initAstBuild();

  public:
    llvm::CallInst *callCombufSend(molly::CommunicationBuffer *combuf);
    llvm::CallInst *callCombufRecv(molly::CommunicationBuffer *combuf);

    llvm::CallInst *callCombufSendbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *dst);
    llvm::CallInst *callCombufRecvbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *src);

  public:
    MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore);
    MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx);

    DefaultIRBuilder &getIRBuilder() { return irBuilder; }
    StmtEditor getStmtEditor();


    //llvm::Value *codegenAff(const isl::Aff &aff);
    llvm::Value *codegenAff(const isl::PwAff &aff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::MultiAff &maff);
    std::vector<llvm::Value *> codegenMultiAff(const isl::MultiPwAff &maff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::PwMultiAff &maff);
    llvm::Value *codegenScev(const llvm::SCEV *scev);
    llvm::Value *codegenId(const isl::Id &id);

    //llvm::Value *codegenLinearize(const isl::MultiPwAff &coord, const molly::AffineMapping *layout);
    llvm::Type *getIntTy();


    llvm::Value *codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);

    void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, isl::Map accessRelation);
    void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::MultiPwAff index);

    void codegenSend(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst);
    void codegenRecv(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src);
  
  //  llvm::Value *codegenGetPtrSendBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst, const isl::MultiPwAff &index);
  //  llvm::Value *codegenGetPtrRecvBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src, const isl::MultiPwAff &index);
  }; // class MollyCodeGenerator

} // namespace molly
#endif /* MOLLY_CODEGEN_H */
