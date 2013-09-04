#ifndef MOLLY_COMMUNICATIONBUFFER_H
#define MOLLY_COMMUNICATIONBUFFER_H

#include "LLVMfwd.h"
#include "islpp/Islfwd.h"
#include "islpp/Map.h"
#include "RectangularMapping.h"
#include "AffineMapping.h"

namespace molly {
  class FieldType;
} // namespace molly


namespace molly {

  class CommunicationBuffer {
  private:
    llvm::GlobalVariable *varsend;
    llvm::GlobalVariable *varrecv;
    //uint64_t baseOffset;
    FieldType *fty;
    isl::Map relation; /* { (src[coord] -> dst[coord]) -> field[indexset] } */

    AffineMapping *mapping;
    isl::PwAff countElts;

    AffineMapping *dstMapping;
    AffineMapping *srcMapping;

    llvm::Value *getSendBufferBase(DefaultIRBuilder &builder);
    llvm::Value *getRecvBufferBase(DefaultIRBuilder &builder);

  protected:
    CommunicationBuffer() : fty(nullptr), mapping(nullptr) { }
    ~CommunicationBuffer() { delete mapping; }

  public:
    static CommunicationBuffer *create(llvm::GlobalVariable *varsend, llvm::GlobalVariable *varrecv,  FieldType *fty, isl::Map &&relation) {
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

    /// { (src[coord] -> dst[coord]) -> field[indexset] }
    const isl::Map &getRelation() const {
      return relation;
    }

    void doLayoutMapping();
     const AffineMapping *getMapping() { return mapping; }
     isl::PwAff getEltCount() { return countElts; }

     /// Mapping from sets of cluster nodes to a linearized rank
     const AffineMapping *getDstMapping() { return dstMapping; }
     const AffineMapping *getSrcMapping() { return srcMapping; }

    llvm::Value *codegenReadFromBuffer(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value *> indices);
    void codegenWriteToBuffer(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::Value *value, llvm::ArrayRef<llvm::Value *> indices);

  }; // class CommunicationBuffer
} // namespace molly
#endif /* MOLLY_COMMUNICATIONBUFFER_H */
