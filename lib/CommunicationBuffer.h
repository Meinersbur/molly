#ifndef MOLLY_COMMUNICATIONBUFFER_H
#define MOLLY_COMMUNICATIONBUFFER_H

#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include "islpp/Map.h"
#include "molly/Mollyfwd.h"
//#include "RectangularMapping.h"
//#include "AffineMapping.h"
#include <map>
#include "Codegen.h"

namespace molly {
  class FieldType;
} // namespace molly


namespace molly {

  class CommunicationBuffer {
  private:
    FieldType *fty;

    /// { chunk[domain] -> (src[cluster], dst[cluster], field[indexset]) }
    isl::Map relation;

    /// Buffer data layout
    RectangularMapping *mapping; // Maps from { (chunk[domain], src[cluster], dst[cluster]) }
    RectangularMapping *sendbufMapping; // Maps from { (chunk[domain], dst[cluster]) }
    RectangularMapping *recvbufMapping; // Maps from { (chunk[domain], src[cluster]) }

    llvm::GlobalVariable *varsend;
    llvm::GlobalVariable *varrecv;


  protected:
    CommunicationBuffer() : fty(nullptr), mapping(nullptr), sendbufMapping(nullptr), recvbufMapping(nullptr) { }
    ~CommunicationBuffer();


  private:
    isl::Ctx *getIslContext() { return relation.getCtx(); }

    llvm::Value *getSendBufferBase(DefaultIRBuilder &builder);
    llvm::Value *getRecvBufferBase(DefaultIRBuilder &builder);

    isl::Space getIndexsetSpace() {
      return relation.getSpace().findNthSubspace(isl_dim_out, 2);
    }


  public:
    static CommunicationBuffer *create(llvm::GlobalVariable *varsend, llvm::GlobalVariable *varrecv, FieldType *fty, isl::Map relation) {
      auto result = new CommunicationBuffer();
      result->varsend = varsend;
      result->varrecv = varrecv;
      //result->baseOffset = baseOffset;
      result->fty = fty;
      result->relation = std::move(relation);
      return result;
    }

    llvm::GlobalVariable *getVariableSend() const {
      return varsend;
    }
    llvm::GlobalVariable *getVariableRecv() const {
      return varrecv;
    }

    FieldType *getFieldType() const {
      return fty;
    }

    llvm::Type *getEltType() const;
    llvm::PointerType *getEltPtrType() const;

    /// { (src[coord] -> dst[coord]) -> field[indexset] }
    const isl::Map &getRelation() const {
      return relation;
    }

    isl::Space getDstNodeSpace();
    isl::Space getSrcNodeSpace();

    isl::Space getDstNamedDims();
    isl::Space getSrcNamedDims();

    void doLayoutMapping();
    const RectangularMapping *getMapping() { return mapping; }

     /// Mapping from sets of cluster nodes to a linearized rank
     //const AffineMapping *getDstMapping() { return dstMapping; }
     //const AffineMapping *getSrcMapping() { return srcMapping; }

    //llvm::Value *codegenReadFromBuffer(MollyCodeGenerator *codegen, const isl::MultiPwAff &indices);
    //void codegenWriteToBuffer(MollyCodeGenerator *codegen, const isl::MultiPwAff &indices);

    // TODO: Move all the codegen functions somewhere else; this class represents a communication buffer, but is not responsible to generate code for it
    void codegenInit(MollyCodeGenerator &codegen, MollyPassManager *pm, MollyFunctionProcessor *funcCtx);

    llvm::Value *codegenPtrToSendBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index);
   void codegenStoreInSendbuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index, llvm::Value *val);
    
    llvm::Value *codegenPtrToRecvBuf(MollyCodeGenerator &codegen, const isl::MultiPwAff &chunk, const isl::MultiPwAff &srcCoord, const isl::MultiPwAff &dstCoord, const isl::MultiPwAff &index);
     llvm::Value *codegenLoadFromRecvBuf(MollyCodeGenerator &codegen,  isl::MultiPwAff chunk,  isl::MultiPwAff srcCoord,  isl::MultiPwAff dstCoord,  isl::MultiPwAff index);

    llvm::Value *codegenSendWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord);
    void codegenSend(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord);
    llvm::Value *codegenRecvWait(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord);
    void codegenRecv(MollyCodeGenerator &codegen, isl::MultiPwAff chunk, isl::MultiPwAff srcCoord, isl::MultiPwAff dstCoord);

    llvm::Value *codegenPtrToSendbufObj(MollyCodeGenerator &codegen);
    llvm::Value *codegenPtrToRecvbufObj(MollyCodeGenerator &codegen);

  }; // class CommunicationBuffer
} // namespace molly
#endif /* MOLLY_COMMUNICATIONBUFFER_H */
