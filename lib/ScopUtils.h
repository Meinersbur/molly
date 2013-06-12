#ifndef MOLLY_SCOPUTILS_H
#define MOLLY_SCOPUTILS_H

#include "islpp/Set.h"
#include "islpp/UnionMap.h"
//#include <polly/ScopInfo.h>

namespace polly {
  class ScopStmt;
  class Dependences;
} // namespace polly

namespace molly {

     isl::Set getIterationDomain(polly::ScopStmt *);
     isl::UnionMap getFlowDependences(polly::Dependences *);

} // namespace molly

#endif /* MOLLY_SCOPUTILS_H */
