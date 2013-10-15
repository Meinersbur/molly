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
    MollyScopStmtProcessor *stmtCtx;//TODO: Remove, should not be directly dependent on this

    /// At which position to insert the code
    DefaultIRBuilder irBuilder;

    Pass *pass;

    isl::Set context;

    /// ISL code generator for affine expressions
    isl::AstBuild astBuild;

    /// Required by polly::IslExprBuilder to map params and domain dims to variables that contain these values
    std::map<isl_id *, llvm::Value *> idtovalue;

  protected:
    isl::Ctx *getIslContext();
    llvm::LLVMContext &getLLVMContext() { return irBuilder.getContext(); }

    llvm::Value *getValueOf(const llvm::SCEV *scev);
    llvm::Value *getValueOf(Value *val) { return val; }
   void fillIdToValueMap();

    llvm::Module *getModule();
    clang::CodeGen::MollyRuntimeMetadata *getRtMetadata();

    isl::AstBuild &initAstBuild();

    llvm::Value *allocStackSpace(llvm::Type *ty);

  protected:
     Function *getRuntimeFunc( llvm::StringRef name, llvm::Type *retTy, llvm::ArrayRef<llvm::Type*> tys);

  public:
    llvm::CallInst *callRuntimeLocalInit(llvm::Value *fvar,  llvm::Value *elts, llvm::Function *rankoffunc, llvm::Function *indexoffunc);
    llvm::CallInst *callRuntimeLocalIndexof(llvm::Value *fvar, llvm::ArrayRef<llvm::Value *> coords);

    llvm::CallInst *callRuntimeClusterCurrentCoord(llvm::Value *d);
    llvm::CallInst *callRuntimeClusterCurrentCoord(uint64_t d) {
      auto intTy = Type::getInt64Ty(getLLVMContext());
      return callRuntimeClusterCurrentCoord(llvm::ConstantInt::get(intTy, d)); 
    }

    llvm::CallInst *callRuntimeCombufSendPtr(llvm::Value *combufSend, llvm::Value *dstRank);
    llvm::CallInst *callRuntimeCombufRecvPtr(llvm::Value *combufRecv, llvm::Value *srcRank);

    llvm::CallInst *callRuntimeCombufSend(llvm::Value *combufSend, llvm::Value *dstRank);
    llvm::CallInst *callRuntimeCombufRecv(llvm::Value *combufRecv, llvm::Value *srcRank);

    llvm::CallInst *callRuntimeLocalPtr(llvm::Value *localobj);


    llvm::CallInst *callClusterCurrentCoord(llvm::Value *d);
    llvm::CallInst *callClusterCurrentCoord(uint64_t d) {
      auto intTy = Type::getInt64Ty(getLLVMContext());
      return callClusterCurrentCoord( llvm::ConstantInt::get(intTy, d) ); 
    }

    llvm::CallInst *callLocalPtr(FieldVariable *fvar);

    llvm::CallInst *callCombufSend(molly::CommunicationBuffer *combuf, llvm::Value *dstRank);
    llvm::CallInst *callCombufRecv(molly::CommunicationBuffer *combuf, llvm::Value *srcRank);

    llvm::CallInst *callCombufSendWait(molly::CommunicationBuffer *combuf, llvm::Value *dstRank);
    llvm::CallInst *callCombufRecvWait(molly::CommunicationBuffer *combuf, llvm::Value *srcRank);

    llvm::CallInst *callCombufSendbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *dst);
    llvm::CallInst *callCombufRecvbufPtr(molly::CommunicationBuffer *combuf, llvm::Value *src);


    llvm::CallInst *callValueLoad(FieldVariable *fvar, llvm::Value *valptr, llvm::Value *rank, llvm::Value *idx);
    llvm::CallInst *callRuntimeValueLoad(FieldVariable *fvar, llvm::Value *srcbufptr, llvm::Value *rank, llvm::Value *idx);
    llvm::LoadInst *codegenValueLoad(FieldVariable *fvar, llvm::Value *rank, llvm::Value *idx);

    llvm::CallInst *callValueStore(FieldVariable *fvar, llvm::Value *valueToStore, llvm::Value *rank, llvm::Value *idx);
    llvm::CallInst *callRuntimeValueStore(FieldVariable *fvar,  llvm::Value *dstbufptr, llvm::Value *rank, llvm::Value *idx);
    void codegenValueStore(FieldVariable *fvar, llvm::Value *val, llvm::Value *rank, llvm::Value *idx);

    llvm::CallInst *callFieldRankof(FieldVariable *layout, llvm::ArrayRef<llvm::Value *> coords);
    llvm::CallInst *callLocalIndexof(FieldVariable *layout, llvm::ArrayRef<llvm::Value *> coords);

  public:
    /// Basic constructor
    /// Usable: getIRBulder(), callXXX()
    MollyCodeGenerator(llvm::BasicBlock *insertBB, llvm::Instruction *insertBefore, Pass *pass);
    MollyCodeGenerator(llvm::BasicBlock *insertBB, llvm::Instruction *insertBefore, llvm::Pass *pass, isl::Set context, const std::map<isl_id *, llvm::Value *> &idtovalue);

    explicit MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx);
    MollyCodeGenerator(MollyScopStmtProcessor *stmtCtx, llvm::Instruction *insertBefore);
  

    DefaultIRBuilder &getIRBuilder() { return irBuilder; }
    StmtEditor getStmtEditor();

    void addParam(isl::Id id, llvm::Value *val);

    //llvm::Value *codegenAff(const isl::Aff &aff);
    llvm::Value *codegenAff(const isl::PwAff &aff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::MultiAff &maff);
    std::vector<llvm::Value *> codegenMultiAff(const isl::MultiPwAff &maff);
    //std::vector<llvm::Value *> codegenMultiAff(const isl::PwMultiAff &maff);
    llvm::Value *codegenScev(const llvm::SCEV *scev);
    llvm::Value *codegenId(const isl::Id &id);



    bool isDependent(llvm::Value *val);
    llvm::Value *getScalarAlloca(llvm::Value *val);
    llvm::Value *materialize(llvm::Value *val);

    void updateScalar(llvm::Value *ptr, llvm::Value *val);

     void addScalarStoreAccess(llvm::Value *base, llvm::Instruction *instr);
    void addStoreAccess(llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr);
     void addScalarLoadAccess(llvm::Value *base, llvm::Instruction *instr);
    void addLoadAccess(llvm::Value *base, isl::Map accessRelation, llvm::Instruction *instr);

    //llvm::Value *codegenLinearize(const isl::MultiPwAff &coord, const molly::AffineMapping *layout);
    llvm::Type *getIntTy();


    llvm::Value *codegenPtrLocal(FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices);

    //void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, llvm::ArrayRef<llvm::Value*> indices, isl::Map accessRelation);
    void codegenStoreLocal(llvm::Value *val, FieldVariable *fvar, isl::PwMultiAff where/* [domain] -> curNode[cluster] */, isl::MultiPwAff index/* [domain] -> field[indexset] */);

    //void codegenStore(llvm::Value *val, llvm::Value *ptr);

    //  llvm::Value *codegenGetPtrSendBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst, const isl::MultiPwAff &index);
    //  llvm::Value *codegenGetPtrRecvBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src, const isl::MultiPwAff &index);
  }; // class MollyCodeGenerator

} // namespace molly
#endif /* MOLLY_CODEGEN_H */
