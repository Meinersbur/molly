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


  /// A pointer with a offset annotation
  /// Tells where the pointer is located relative to a base pointer or a field
  /// Use to add access information when the pointer is read or written, which is again used to compute dependencies
  class AnnotatedPtr {
    llvm::Value *ptr;
    isl::PwMultiAff coord; // { [domain] -> [index] } // TODO: Define and enforce tuple ids
    
    // ptr into array
    llvm::Value *base;

    // ptr into field
    FieldVariable *fvar;

    llvm::Value *val;

  protected:
    AnnotatedPtr(llvm::Value *ptr, isl::PwMultiAff coord, llvm::Value *base, FieldVariable *fvar, llvm::Value *val)
    : ptr(ptr), coord(coord),base(base),fvar(fvar),val(val) { }
  public:
  //  AnnotatedPtr() : 
    //  ptr(nullptr), base(nullptr), fvar(nullptr) { }

    static AnnotatedPtr createScalarPtr(llvm::Value * ptr, isl::Space domainspace) {
      auto islctx = domainspace.getCtx();
      auto space = isl::Space::createMapFromDomainAndRange(domainspace, islctx->createSetSpace(0));
      return AnnotatedPtr(ptr, space.createZeroMultiAff(), ptr, nullptr, nullptr);
    }

    static AnnotatedPtr createFieldPtr(llvm::Value *ptr, FieldVariable *fvar, isl::PwMultiAff coord) {
      return AnnotatedPtr(ptr, coord, nullptr, fvar, nullptr);
    }

    static AnnotatedPtr createArrayPtr(llvm::Value *ptr, llvm::Value *base, isl::PwMultiAff coord) {
      return AnnotatedPtr(ptr, coord, base, nullptr, nullptr);
    }

    static AnnotatedPtr createUnannotated(llvm::Value *ptr) {
      return AnnotatedPtr(ptr, isl::PwMultiAff(), nullptr, nullptr, nullptr);
    }

    /// Content not actually stored at a ptr, but directly in a register
    /// Cannot be written to
    static AnnotatedPtr createRegister(llvm::Value *val) {
      return AnnotatedPtr(nullptr, isl::PwMultiAff(), nullptr, nullptr, val);
    }


    bool isFieldPtr() const {
      return fvar!=nullptr;
    }

    bool isScalar() const { 
      return coord.getOutDimCount() == 0; 
    }

    bool isRegister() const {
      return val!=nullptr;
    }

    bool isAnnotated() const { return getDiscriminator() && coord.isValid(); }

    llvm::Value *getPtr() const { assert(!isRegister()); return ptr;}
    isl::PwMultiAff getCoord() const { return coord; } // or offset

    FieldVariable *getFieldVar() const{ return fvar; }

    llvm::Value *getRegister() const { return val; }

    /// Used to identify which block of memory/field is accessed
    llvm::Value *getDiscriminator() const;

    // coord: { old_domain[] -> field[] }
    // mapper: { new_domain[] -> old_domain[] }
    // { new_domain[] -> old_domain[]-> field[] }
    AnnotatedPtr pullbackDomain(isl::PwMultiAff newToOldMapper) {
      return AnnotatedPtr(ptr, coord.isValid() ? coord.pullback(newToOldMapper) : coord, base, fvar, val);
    }

    void dump() const;
  }; // class AnnotatedPtr


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

    llvm::Value *getValueOf(const llvm::SCEV *scev);
    llvm::Value *getValueOf(Value *val) { return val; }
    void fillIdToValueMap();

    llvm::Module *getModule();
    clang::CodeGen::MollyRuntimeMetadata *getRtMetadata();

    isl::AstBuild &initAstBuild();

    llvm::Value *copyOperandTree(llvm::Value *val);

  protected:
    Function *getRuntimeFunc( llvm::StringRef name, llvm::Type *retTy, llvm::ArrayRef<llvm::Type*> tys);

  public:
    llvm::LLVMContext &getLLVMContext() { return irBuilder.getContext(); }

    const llvm::DataLayout *getDataLayout();

    llvm::Value *allocStackSpace(llvm::Type *ty, const llvm::Twine &name = Twine());
    llvm::Value *allocStackSpace(llvm::Type *ty, int count, const llvm::Twine &name = Twine());
    llvm::Value *allocStackSpace(llvm::Type *ty, llvm::Value *count, const llvm::Twine &name = Twine());
    llvm::Value *createPointerCast(llvm::Value *val, llvm::Type *type);

    void setInsertBefore(llvm::Instruction *beforeInstr) {
      irBuilder.SetInsertPoint(beforeInstr);
    }
    void setInsertAfter(llvm::Instruction *afterInstr) {
      auto beforeInstr = afterInstr->getNextNode();
      assert(beforeInstr);
      irBuilder.SetInsertPoint(beforeInstr);
    }

    llvm::BasicBlock *getInsertBlock() const {
    return irBuilder.GetInsertBlock();
    }


    llvm::CallInst *callCombufLocalAlloc(llvm::Value *size, llvm::Value *eltSize);
    llvm::CallInst *callCombufLocalFree(llvm::Value *combufLocal);
    llvm::CallInst *callCombufLocalDataPtr(llvm::Value *combufLocal);

    llvm::CallInst *callRuntimeCombufLocalAlloc(llvm::Value *size, llvm::Value *eltSize);
    llvm::CallInst *callRuntimeCombufLocalFree(llvm::Value *combufvar);
    llvm::CallInst *callRuntimeCombufLocalDataptr(llvm::Value *combufvar);



    llvm::CallInst *callLocalInit(llvm::Value *fvar,  llvm::Value *elts, llvm::Function *rankoffunc, llvm::Function *indexoffunc);
    llvm::CallInst *callRuntimeLocalInit(llvm::Value *fvar,  llvm::Value *elts, llvm::Function *rankoffunc, llvm::Function *indexoffunc);
    llvm::CallInst *callRuntimeLocalIndexof(llvm::Value *fvar, llvm::ArrayRef<llvm::Value *> coords);

    llvm::CallInst *callRuntimeClusterCurrentCoord(llvm::Value *d);
    llvm::CallInst *callRuntimeClusterCurrentCoord(uint64_t d) {
      auto intTy = Type::getInt64Ty(getLLVMContext());
      return callRuntimeClusterCurrentCoord(llvm::ConstantInt::get(intTy, d)); 
    }

    llvm::CallInst *callRuntimeCombufSendAlloc(llvm::Value *nDst, llvm::Value *eltSize, llvm::Value *tag);
    llvm::CallInst *callRuntimeCombufRecvAlloc(llvm::Value *nDst, llvm::Value *eltSize, llvm::Value *tag);

    llvm::CallInst *callRuntimeCombufSendDstInit(llvm::Value *combufSend, llvm::Value *dst, llvm::Value *nClusterDims, llvm::Value *dstCoords, llvm::Value *countElts, llvm::Value *tag);
    llvm::CallInst *callRuntimeCombufRecvSrcInit(llvm::Value *combufSend, llvm::Value *src, llvm::Value *nClusterDims, llvm::Value *srcCoords, llvm::Value *countElts, llvm::Value *tag);

    llvm::CallInst *callRuntimeCombufSendPtr(llvm::Value *combufSend, llvm::Value *dstRank);
    llvm::CallInst *callRuntimeCombufRecvPtr(llvm::Value *combufRecv, llvm::Value *srcRank);

    llvm::CallInst *callRuntimeCombufSend(llvm::Value *combufSend, llvm::Value *dstRank);
    llvm::CallInst *callRuntimeCombufRecv(llvm::Value *combufRecv, llvm::Value *srcRank);

    llvm::CallInst *callRuntimeCombufSendWait(llvm::Value *combufSend, llvm::Value *dstRank);
    llvm::CallInst *callRuntimeCombufRecvWait(llvm::Value *combufRecv, llvm::Value *srcRank);

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
    //void codegenValueLoadPtr(FieldVariable *fvar, llvm::Value *rank, llvm::Value *idx, llvm::Value *writeIntoPtr);

    llvm::CallInst *callValueStore(FieldVariable *fvar, llvm::Value *valueToStore, llvm::Value *rank, llvm::Value *idx);
    llvm::CallInst *callRuntimeValueStore(FieldVariable *fvar,  llvm::Value *dstbufptr, llvm::Value *rank, llvm::Value *idx);
    void codegenValueStore(FieldVariable *fvar, llvm::Value *val, llvm::Value *rank, llvm::Value *idx);

    llvm::CallInst *callFieldRankof(FieldVariable *layout, llvm::ArrayRef<llvm::Value *> coords);
    llvm::CallInst *callLocalIndexof(FieldVariable *layout, llvm::ArrayRef<llvm::Value *> coords);

    llvm::CallInst *callFieldInit(llvm::Value *field, llvm::MDNode *metadata);

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

    void codegenAssignLocalFromScalar(FieldVariable *dstFvar, isl::PwMultiAff dstWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff dstIndex/* [domain] -> field[indexset] */, llvm::Value *srcPtr);

    llvm::Value *getLocalBufPtr(FieldVariable *fvar);
    llvm::Value *getSendBufPtrs(CommunicationBuffer *combuf);
    llvm::Value *getSendbufPtrPtr(CommunicationBuffer *combuf, llvm::Value *index);
    llvm::Value *getSendbufPtr(CommunicationBuffer *combuf, llvm::Value *index);
    llvm::Value *getRecvBufPtrs(CommunicationBuffer *combuf);
    llvm::Value *getRecvbufPtrPtr(CommunicationBuffer *combuf, llvm::Value *index);
    llvm::Value *getRecvbufPtr(CommunicationBuffer *combuf, llvm::Value *index);

    llvm::Value *codegenLoadLocalPtr(FieldVariable *fvar, isl::PwMultiAff where/* [domain] -> curNode[cluster] */, isl::MultiPwAff index/* [domain] -> field[indexset] */);
    llvm::Value *codegenLoadLocal(FieldVariable *fvar, isl::PwMultiAff where/* [domain] -> curNode[cluster] */, isl::MultiPwAff index/* [domain] -> field[indexset] */);

    llvm::MemCpyInst *codegenAssignPtrPtr(llvm::Value *dstPtr, llvm::Value *srcPtr);
    llvm::MemCpyInst *codegenAssignPtrPtr(llvm::Value *dstPtr, llvm::Value *dstBase, isl::Map dstIndex, llvm::Value *srcPtr, llvm::Value *srcBase, isl::Map srcIndex);

    void codegenAssignPtrVal(llvm::Value *dstPtr, llvm::Value *srcVal);
    void codegenAssignValPtr(llvm::Value *dstVal, llvm::Value *srcPtr);
    void codegenAssignValVal(llvm::Value *dstVal, llvm::Value *srcVal);
    void codegenAssignScalarFromLocal(llvm::Value *destPtr, FieldVariable *srcFvar, isl::PwMultiAff srcWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff srcIndex/* [domain] -> field[indexset] */);

    void codegenAssign(const AnnotatedPtr &dst, const AnnotatedPtr &src);

    AnnotatedPtr codegenFieldLocalPtr(FieldVariable *srcFvar, isl::PwMultiAff srcWhere/* [domain] -> curNode[cluster] */, isl::MultiPwAff srcIndex/* [domain] -> field[indexset] */);

    //void codegenStore(llvm::Value *val, llvm::Value *ptr);

    //  llvm::Value *codegenGetPtrSendBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &dst, const isl::MultiPwAff &index);
    //  llvm::Value *codegenGetPtrRecvBuf(molly::CommunicationBuffer *combuf, const isl::MultiPwAff &src, const isl::MultiPwAff &index);

    llvm::LoadInst *createScalarLoad(llvm::Value *ptr, const llvm::Twine &name) {
      auto result = irBuilder.CreateLoad(ptr, name);
      addScalarLoadAccess(ptr, result);
      return result;
    }

    llvm::StoreInst *createScalarStore( llvm::Value *val, llvm::Value *ptr) {
      auto result = irBuilder.CreateStore(val, ptr);
      addScalarStoreAccess(ptr, result);
      return result;
    }
    llvm::StoreInst *createArrayStore( llvm::Value *val, llvm::Value *baseptr, llvm::Value *idxVal, isl::PwAff idxAff) {
      auto ptr = irBuilder.CreateGEP(baseptr, idxVal);
      auto result = irBuilder.CreateStore(val, ptr);
      addStoreAccess(baseptr, idxAff.toMap(), result);
      return result;
    }
    llvm::StoreInst *createArrayStore(llvm::Value *val, llvm::Value *baseptr, int idx);


    llvm::Function *getParentFunction() const {
      return irBuilder.GetInsertBlock()->getParent();
    }

    llvm::CallInst *callBeginMarker(StringRef str);
    llvm::CallInst *callEndMarker(StringRef str);

#if 0
    void beginMark(StringRef str) {
      if (!MollyMarkStmts)
        return;

      auto bb = irBuilder.GetInsertBlock();
      irBuilder.SetInsertPoint(bb, bb->begin());
      callBeginMarker(str);
    }

    void endMark(StringRef str) {
      if (!MollyMarkStmts)
        return;

      auto bb = irBuilder.GetInsertBlock();
      auto term = bb->getTerminator();
      if (term)
        irBuilder.SetInsertPoint(bb, term);
      else
        irBuilder.SetInsertPoint(bb, bb->end());
      callEndMarker(str);
    }
#endif


    void markBlock(StringRef str);
    void markBlock(StringRef str, isl::MultiPwAff coord);
  }; // class MollyCodeGenerator

} // namespace molly
#endif /* MOLLY_CODEGEN_H */
