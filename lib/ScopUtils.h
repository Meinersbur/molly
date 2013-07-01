#ifndef MOLLY_SCOPUTILS_H
#define MOLLY_SCOPUTILS_H

#include "islpp/Union.h"

namespace isl {
class Set;
class Map;
} // namespace isl

namespace polly {
  class ScopStmt;
  class Dependences;
} // namespace polly


namespace molly {
     isl::Set getIterationDomain(polly::ScopStmt *);
     isl::Map getScattering(polly::ScopStmt *);
     isl::Map getWhereMap(polly::ScopStmt *);

     isl::UnionMap getFlowDependences(polly::Dependences *);
} // namespace molly

#endif /* MOLLY_SCOPUTILS_H */
