#ifndef MOLLY_SCOPDISTRIBUTION_H
#define MOLLY_SCOPDISTRIBUTION_H

namespace llvm {
  class Pass;
} // namespace llvm

namespace polly {
  class ScopPass;
} // namespace polly


namespace molly {
  extern char &ScopDistributionPassID;

  polly::ScopPass *createScopDistributionPass(); 
} // namespace molly

#endif // MOLLY_SCOPDISTRIBUTION_H
