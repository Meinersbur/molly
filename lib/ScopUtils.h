#ifndef MOLLY_SCOPUTILS_H
#define MOLLY_SCOPUTILS_H

#include "islpp/Set.h"
#include <polly/ScopInfo.h>

namespace molly {

     isl::Set getIterationDomain(polly::ScopStmt *);

} // namespace molly

#endif /* MOLLY_SCOPUTILS_H */
