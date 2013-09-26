#ifndef MOLLY_FIELDLAYOUT_H
#define MOLLY_FIELDLAYOUT_H

#include "islpp/Islfwd.h"
#include "molly/Mollyfwd.h"
#include "LLVMfwd.h"

#pragma region Forward declarations
namespace isl {
} // namespace isl

namespace molly {
  class FieldType;
  class LayoutMapping;
} // namespace molly
#pragma endregion


namespace molly {

  // FieldLocalLayout ?
  class FieldLayout {
    FieldType *fty;

    // Later we'll chain arbitrary mappings and combine them when possible
    AffineMapping *affine;
    RectangularMapping *linearizer;

  protected:
    explicit FieldLayout(FieldType *fty) : fty(fty), affine(nullptr), linearizer(nullptr) {    }

    FieldLayout(FieldType *fty,AffineMapping *affine,  RectangularMapping *linearizer) : fty(fty), affine(affine), linearizer(linearizer) {}

  public:
    static FieldLayout *create(FieldType *fty){ 
    return new FieldLayout(fty);
    }

    static FieldLayout *create(FieldType *fty, AffineMapping *affine,  RectangularMapping *linearizer){ 
      return new FieldLayout(fty, affine, linearizer);
    }


  public:
    ~FieldLayout();

    llvm::Value *codegenLocalIndex(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator, isl::MultiPwAff coords);
    llvm::Value *codegenLocalSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator);

  }; // class FieldLayout

} // namespace molly
#endif /* MOLLY_FIELDLAYOUT_H */
