#ifndef MOLLY_RECTANGULARMAPPING_H
#define MOLLY_RECTANGULARMAPPING_H

//#include "LayoutMapping.h"
#include "LLVMfwd.h"
#include <llvm/ADT/SmallVector.h>
#include "islpp/Islfwd.h"
//#include <llvm/ADT/ArrayRef.h>
#include "molly/Mollyfwd.h"
#include "islpp/MultiPwAff.h"


namespace molly {

  /// Row-major order
  class RectangularMapping {
  private:
    isl::MultiPwAff lengths;
    isl::MultiPwAff offsets;

    //llvm::SmallVector<unsigned,4> lengths;
    //llvm::SmallVector<unsigned,4> offsets;

  public:
    RectangularMapping(isl::MultiPwAff &&lengths, isl::MultiPwAff &&offsets) : lengths(std::move(lengths)), offsets(std::move(offsets)) {}

    static RectangularMapping *createRectangualarHullMapping(const isl::Map &map);

    //RectangularMapping(llvm::ArrayRef<unsigned> lengths) : lengths(lengths.begin(), lengths.end()), offsets( lengths.size(), 0) { }
    //RectangularMapping(llvm::ArrayRef<unsigned> lengths, llvm::ArrayRef<unsigned> offsets) : lengths(lengths.begin(), lengths.end()), offsets(offsets.begin(), offsets.end()) { }

    //unsigned getInputDims() { return lengths.size(); }
    //llvm::ArrayRef<unsigned> getLengths() {return lengths;}
    //llvm::ArrayRef<unsigned> getOffsets() {return offsets;}

    //int map(llvm::ArrayRef<unsigned> coords) const;
    //llvm::Value *codegen(DefaultIRBuilder &builder, llvm::ArrayRef<llvm::Value*> coords);

    llvm::Value *codegenIndex(MollyCodeGenerator &codegen, const isl::MultiPwAff &coords);
    llvm::Value *codegenSize(MollyCodeGenerator &codegen);

  }; // class RectangularMapping

} // namespace molly
#endif /* MOLLY_RECTANGULARMAPPING_H */
