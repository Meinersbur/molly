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
  /// One-to-one mapping
  /// Row-major order
  // Other possibilities to implement
  // - Column-major (or preapply a dimension reversal)
  // - Indirect array index
  // - Exact counting qpolynomial using Barvinok
  class RectangularMapping {
  private:
    isl::MultiPwAff lengths; // { [] -> lengths[nDims] }
    isl::MultiPwAff offsets; // { [] -> offset[nDims] }

  public:
    RectangularMapping(isl::MultiPwAff lengths, isl::MultiPwAff offsets) : lengths(std::move(lengths)), offsets(std::move(offsets)) {}

    static RectangularMapping *create(isl::MultiPwAff lengths, isl::MultiPwAff offsets);
    // static RectangularMapping *create(llvm::ArrayRef<int> lengths, llvm::ArrayRef<int> offsets, isl::Ctx *islctx);


    //ISLPP_DEPRECATED_MSG("RectangularMapping should be exclusive row-major mapping, nothing affine going on") 
    static RectangularMapping *createRectangualarHullMapping(isl::Map map);
    // static RectangularMapping *createRectangualarHullMapping(isl::Set set);

    llvm::Value *codegenIndex(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator, const isl::MultiPwAff &coords);
    llvm::Value *codegenSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator);
    llvm::Value *codegenMaxSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator);

    /// The dimensions the rectangle may depend on
    /// Typical: current node, current chunk
    isl::Space getDomainSpace() {
      auto result = lengths.getDomainSpace();
      assert(result == offsets.getDomainSpace());
      return result;
    }
  }; // class RectangularMapping

} // namespace molly
#endif /* MOLLY_RECTANGULARMAPPING_H */
