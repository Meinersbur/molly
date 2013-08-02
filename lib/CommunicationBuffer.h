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
    llvm::GlobalVariable *var;
    //uint64_t baseOffset;
    FieldType *fty;
    isl::Map relation; /* { (src[coord] -> dst[coord]) -> field[indexset] } */

    AffineMapping *mapping;
    isl::PwAff countElts;

    llvm::Value *getBufferBase(llvm::IRBuilder<> &builder);

  protected:
    CommunicationBuffer() : fty(nullptr), mapping(nullptr) { }
    ~CommunicationBuffer() { delete mapping; }

  public:
    static CommunicationBuffer *create(llvm::GlobalVariable *var, /*uint64_t baseOffset,*/ FieldType *fty, isl::Map &&relation) {
      auto result = new CommunicationBuffer();
      result->var = var;
      //result->baseOffset = baseOffset;
      result->fty = fty;
      result->relation = std::move(relation);
      return result;
    }

    llvm::GlobalVariable *getVariable() const {
      return var;
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

    llvm::Value *codegenReadFromBuffer(llvm::IRBuilder<> &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value *> indices);
    void codegenWriteToBuffer(llvm::IRBuilder<> &builder, std::map<isl_id *, llvm::Value *> &params, llvm::Value *value, llvm::ArrayRef<llvm::Value *> indices);

  }; // class CommunicationBuffer
} // namespace molly
#endif /* MOLLY_COMMUNICATIONBUFFER_H */
