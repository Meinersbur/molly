#include "ScopEditor.h"

#include <polly/ScopInfo.h>
#include "islpp/Set.h"
#include "islpp/Map.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;


namespace molly {
  ScopStmt *createScopStmt(polly::Scop *parent, llvm::BasicBlock *bb, const llvm::Region *region, const std::string &baseName, llvm::ArrayRef<llvm::Loop*> sourroundingLoops, isl::Set &&domain, isl::Map &&scattering) {
    return new ScopStmt(parent, bb, baseName, region, sourroundingLoops, domain.take(), scattering.take());
  }
} // namespace molly
