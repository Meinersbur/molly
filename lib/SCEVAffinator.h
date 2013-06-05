#ifndef MOLLY_SCEVAFFINATOR_H
#define MOLLY_SCEVAFFINATOR_H

#include "islpp/MultiPwAff.h"

namespace polly {
  class ScopStmt;
} // namespace polly

namespace llvm {
  class SCEV;
} // namespace llvm


namespace molly {

  isl::PwAff convertScEvToAffine(polly::ScopStmt *, const llvm::SCEV *);

} // namespace molly
#endif /* MOLLY_SCEVAFFINATOR_H */
