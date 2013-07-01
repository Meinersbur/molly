#ifndef MOLLY_SCOPUTILS_H
#define MOLLY_SCOPUTILS_H

#include "islpp/Union.h"

namespace isl {
  class Set;
  class Map;
  class Space;
} // namespace isl

namespace polly {
  class Scop;
  class ScopStmt;
  class Dependences;
} // namespace polly


namespace molly {
  isl::Set getIterationDomain(polly::ScopStmt *);
  isl::Map getScattering(polly::ScopStmt *);
  isl::Space getScatteringSpace(polly::Scop *scop);
  isl::Space getScatteringSpace(polly::ScopStmt *stmt);
  isl::Map getWhereMap(polly::ScopStmt *);

  isl::UnionMap getFlowDependences(polly::Dependences *);
} // namespace molly

#endif /* MOLLY_SCOPUTILS_H */
