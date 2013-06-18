#include "ScopUtils.h"

#include <polly/ScopInfo.h>
#include <polly/Dependences.h>

using namespace molly;
using namespace polly;

using isl::enwrap;


isl::Set molly::getIterationDomain(polly::ScopStmt *stmt) {
  auto dom = stmt->getDomain();
  auto result = enwrap(dom);
  //result.dump();
  return result;
}


isl::UnionMap molly::getFlowDependences(polly::Dependences *deps) {
  auto umap = deps->getDependences(Dependences::TYPE_RAW);
  return enwrap(umap);
}
