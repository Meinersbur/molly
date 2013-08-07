#ifndef MOLLY_RECTANGULARMAPPING_H
#define MOLLY_RECTANGULARMAPPING_H

//#include "LayoutMapping.h"
#include "LLVMfwd.h"
#include <llvm/ADT/SmallVector.h>
#include "islpp/Islfwd.h"
#include <llvm/ADT/ArrayRef.h>


namespace molly {

  /// Row-major order
  class RectangularMapping {
  private:
    llvm::SmallVector<unsigned,4> lengths;
    llvm::SmallVector<unsigned,4> offsets;

  public:
    RectangularMapping(llvm::ArrayRef<unsigned> lengths) : lengths(lengths.begin(), lengths.end()), offsets( lengths.size(), 0) { }
    RectangularMapping(llvm::ArrayRef<unsigned> lengths, llvm::ArrayRef<unsigned> offsets) : lengths(lengths.begin(), lengths.end()), offsets(offsets.begin(), offsets.end()) { }

    unsigned getInputDims() { return lengths.size(); }
    llvm::ArrayRef<unsigned> getLengths() {return lengths;}
    llvm::ArrayRef<unsigned> getOffsets() {return offsets;}

    int map(llvm::ArrayRef<unsigned> coords) const;
    llvm::Value *codegen(DefaultIRBuilder &builder, llvm::ArrayRef<llvm::Value*> coords);

  }; // class RectangularMapping

} // namespace molly
#endif /* MOLLY_RECTANGULARMAPPING_H */
