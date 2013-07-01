#include "ScopUtils.h"

#include <polly/ScopInfo.h>
#include <polly/Dependences.h>
#include <islpp/Set.h>
#include <islpp/Map.h>
#include <islpp/UnionMap.h>

using namespace molly;
using namespace polly;

using isl::enwrap;


isl::Set molly::getIterationDomain(polly::ScopStmt *stmt) {
  auto dom = stmt->getDomain();
  return enwrap(dom);
}


isl::Map molly::getScattering(polly::ScopStmt *stmt) {
  auto scat = stmt->getScattering();
  return enwrap(scat);
}


isl::Map molly::getWhereMap(polly::ScopStmt *stmt) {
  auto where = stmt->getWhereMap();
  return enwrap(where);
}


isl::UnionMap molly::getFlowDependences(polly::Dependences *deps) {
  auto umap = deps->getDependences(Dependences::TYPE_RAW);
  return enwrap(umap);
}
