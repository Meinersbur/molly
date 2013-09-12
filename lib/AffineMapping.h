#ifndef MOLLY_AFFINEMAPPING_H
#define MOLLY_AFFINEMAPPING_H

//#include "LayoutMapping.h"
#include "islpp/Islfwd.h"
#include "LLVMfwd.h"
//#include "islpp/MultiPwAff.h"
#include "islpp/PwAff.h"
#include <map>


namespace molly {

  class AffineMapping {
  private:
    isl::PwAff mapping;
    isl::PwAff countElts;

  public:
    AffineMapping(isl::PwAff &&mapping, isl::PwAff &&countElts) : mapping(std::move(mapping)), countElts(std::move(countElts)) { }

    /// { [indexset] -> size_t }
    const isl::PwAff &getMapping() const { return mapping; }

    isl::Set getIndexset() const;

    llvm::Value *codegen(DefaultIRBuilder &builder, std::map<isl_id *, llvm::Value *> &params, llvm::ArrayRef<llvm::Value*> coords);
  }; // class AffineMapping

} // namespace molly
#endif /* MOLLY_AFFINEMAPPING_H */
