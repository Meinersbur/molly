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
    FieldLayout(FieldType *fty) : fty(fty), affine(nullptr), linearizer(nullptr) {
    }

  public:
    static FieldLayout *create(FieldType *fty){ 
    return new FieldLayout(fty);
    }

  public:
    ~FieldLayout();

    llvm::Value *codegenLocalSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator);

  }; // class FieldLayout

} // namespace molly
#endif /* MOLLY_FIELDLAYOUT_H */
