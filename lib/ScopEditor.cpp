#include "ScopEditor.h"

#include <polly/ScopInfo.h>
#include "islpp/Set.h"
#include "islpp/Map.h"
#include "ScopUtils.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


polly::ScopStmt *molly::replaceScopStmt(polly::ScopStmt *model, llvm::BasicBlock *bb, const std::string &baseName, isl::Map &&replaceDomainWhere) {
  auto scop = model->getParent();
  auto origDomain = getIterationDomain(model);
  auto origWhere = getWhereMap(model);
  origWhere.intersectDomain_inplace(origDomain);
  auto scattering = getScattering(model);

  assert(isSubset(replaceDomainWhere, origWhere));
  auto newModelDomainWhere = origWhere.subtract(replaceDomainWhere);
  auto newModelDomain = newModelDomainWhere.getDomain();
  auto createdModelDomain = replaceDomainWhere.getDomain();

  auto result = createScopStmt(
    scop,
    bb,
    model->getRegion(),
    baseName,
    model->getLoopNests(),
    createdModelDomain.move(),
    scattering.move()
    );
  model->setDomain(newModelDomain.take());
  return result;
}

 
  polly::ScopStmt *molly::createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering) {
    return new ScopStmt(parent, bb, baseName, region, sourroundingLoops, domain.take(), scattering.take());
  }

