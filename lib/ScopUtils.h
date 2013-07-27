#ifndef MOLLY_SCOPUTILS_H
#define MOLLY_SCOPUTILS_H

#include "islpp/Islfwd.h"

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
  class FieldType;
} // namespace molly


namespace molly {
  isl::Id getScatterTuple(polly::Scop *); 

  isl::Id getDomainTuple(polly::ScopStmt *);
  isl::Set getIterationDomain(polly::ScopStmt *);
  
  isl::Map getScattering(polly::ScopStmt *); // rename: getSchedule
  isl::Space getScatteringSpace(polly::Scop *scop);
  isl::Space getScatteringSpace(polly::ScopStmt *stmt);
  
  // write_stmt[domain] -> [nodecoord]
  isl::Map getWhereMap(polly::ScopStmt *);

  isl::Set getGlobalIndexset(FieldType *);

  isl::UnionMap getFlowDependences(polly::Dependences *);
} // namespace molly

#endif /* MOLLY_SCOPUTILS_H */
