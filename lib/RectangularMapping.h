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

  /// RectangularMapping maps each point of a rectangular-shaped integer vector space to non-negative integer
  /// Row-major order
  class RectangularMapping {
  private:
    isl::MultiPwAff lengths; // { [] -> lengths[nDims] }
    isl::MultiPwAff offsets; // { [] -> offset[nDims] }

    //llvm::SmallVector<unsigned,4> lengths;
    //llvm::SmallVector<unsigned,4> offsets;

  public:
    RectangularMapping(isl::MultiPwAff lengths, isl::MultiPwAff offsets) : lengths(std::move(lengths)), offsets(std::move(offsets)) {}

    static RectangularMapping *create(isl::MultiPwAff lengths, isl::MultiPwAff offsets);
   // static RectangularMapping *create(llvm::ArrayRef<int> lengths, llvm::ArrayRef<int> offsets, isl::Ctx *islctx);
    static RectangularMapping *createRectangualarHullMapping(const isl::Map &map);

    //RectangularMapping(llvm::ArrayRef<unsigned> lengths) : lengths(lengths.begin(), lengths.end()), offsets( lengths.size(), 0) { }
    //RectangularMapping(llvm::ArrayRef<unsigned> lengths, llvm::ArrayRef<unsigned> offsets) : lengths(lengths.begin(), lengths.end()), offsets(offsets.begin(), offsets.end()) { }

    //unsigned getInputDims() { return lengths.size(); }
    //llvm::ArrayRef<unsigned> getLengths() {return lengths;}
    //llvm::ArrayRef<unsigned> getOffsets() {return offsets;}

    //int map(llvm::ArrayRef<unsigned> coords) const;
    //llvm::Value *codegen(DefaultIRBuilder &builder, llvm::ArrayRef<llvm::Value*> coords);

    llvm::Value *codegenIndex(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator, const isl::MultiPwAff &coords);
    llvm::Value *codegenSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator);
    llvm::Value *codegenMaxSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator);

  }; // class RectangularMapping

} // namespace molly
#endif /* MOLLY_RECTANGULARMAPPING_H */
