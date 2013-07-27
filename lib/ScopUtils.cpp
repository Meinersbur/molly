#include "ScopUtils.h"

#include <polly/ScopInfo.h>
#include <polly/Dependences.h>
#include "islpp/Set.h"
#include "islpp/Map.h"
#include "islpp/UnionMap.h"
#include "islpp/Space.h"
#include "FieldType.h"
#include "islpp/Ctx.h"

using namespace molly;
using namespace polly;
using isl::enwrap;


 isl::Id molly::getScatterTuple(polly::Scop *scop) {
 return isl::enwrap(scop->getIslCtx())->createId("scattering");
 }


isl::Id molly::getDomainTuple(ScopStmt *scopStmt) {
  return enwrap(scopStmt->getTupleId());
}


isl::Set molly::getIterationDomain(polly::ScopStmt *stmt) {
  auto dom = stmt->getDomain();
  return enwrap(dom);
}


isl::Map molly::getScattering(polly::ScopStmt *stmt) {
  auto scat = stmt->getScattering();
  return enwrap(scat);
}


 isl::Space molly::getScatteringSpace(polly::Scop *scop) {
   return enwrap(scop->getScatteringSpace());
 }


  isl::Space molly::getScatteringSpace(polly::ScopStmt *stmt) {
    return getScatteringSpace(stmt->getParent());
  }


isl::Map molly::getWhereMap(polly::ScopStmt *stmt) {
  auto where = stmt->getWhereMap();
  return enwrap(where);
}


  isl::Set molly::getGlobalIndexset(FieldType *fty) {
    return fty->getGlobalIndexset();
  }


isl::UnionMap molly::getFlowDependences(polly::Dependences *deps) {
  auto umap = deps->getDependences(Dependences::TYPE_RAW);
  return enwrap(umap);
}


