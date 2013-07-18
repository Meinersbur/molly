#ifndef MOLLY_SCOPEDITOR_H
#define MOLLY_SCOPEDITOR_H
/// Tools for changing statements in SCoPs

#include "islpp/Islfwd.h"
#include "Pollyfwd.h"
#include <llvm/ADT/ArrayRef.h>
#include <string>

namespace llvm {
  class BasicBlock;
  class Region;
  class Loop;
} // namespace llvm


namespace molly {

  polly::ScopStmt *createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering);

} // namespace molly
#endif /* MOLLY_SCOPEDITOR_H */
