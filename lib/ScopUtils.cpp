#include "ScopUtils.h"

using isl::enwrap;


isl::Set molly::getIterationDomain(polly::ScopStmt *stmt) {
 auto dom = stmt->getDomain();
 return enwrap(dom);
}
