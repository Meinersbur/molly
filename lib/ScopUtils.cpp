#include "ScopUtils.h"

using isl::enwrap;


isl::Set getIterationDomain(polly::ScopStmt *stmt) {
 auto dom = stmt->getDomain();
 return enwrap(dom);
}
