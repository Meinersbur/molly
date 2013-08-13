#include "MollyScopStmtProcessor.h"
#include "MollyPassManager.h"
#include "polly\ScopInfo.h"
#include "MollyUtils.h"
#include "ScopUtils.h"
#include "ClusterConfig.h"
#include "MollyScopProcessor.h"
#include "islpp/Set.h"
#include "islpp/Map.h"

using namespace molly;
using namespace polly;
using namespace llvm;
using namespace std;
using isl::enwrap;


namespace {

  class MollyScopStmtProcessorImpl : public MollyScopStmtProcessor {
  private:
    MollyPassManager *pm;
    ScopStmt *stmt;

  public:
    MollyScopStmtProcessorImpl(MollyPassManager *pm, ScopStmt *stmt) : pm(pm), stmt(stmt) {}


    void applyWhere() {
      auto scop = stmt->getParent();
      auto func = getParentFunction(stmt);
      //auto funcCtx = pm->getFuncContext(func);
      auto scopCtx = pm->getScopContext(scop);
      // auto se = pm->findOrRunAnalysis<ScalarEvolution>(func);
      auto clusterConf = pm->getClusterConfig();

      auto domain = getIterationDomain(stmt);
      auto where = getWhereMap(stmt);
      where.intersectDomain_inplace(domain);

      auto nCoords = clusterConf->getClusterDims();
      auto coordSpace = clusterConf->getClusterSpace();
      auto coordValues = scopCtx->getClusterCoordinates();
      assert(coordValues.size() == nCoords);

      auto coordMatches = coordSpace.createUniverseBasicSet();
      for (auto i = 0; i < nCoords; i+=1) {
        // auto coordValue = coordValues[i];
        auto scev = coordValues[i];
        auto id = enwrap(scop->getIdForParam(scev));
        coordMatches.alignParams_inplace(pm->getIslContext()->createParamsSpace(1).setParamDimId(0,id)); // Add the param if not exists yet
        auto paramDim = coordMatches.findDim(id);
        assert(paramDim.isValid());
        auto coordDim = coordSpace.getSetDim(i);
        assert(coordDim.isValid());

        coordMatches.equate_inplace(paramDim, coordDim);
      }

      auto newDomain = where.intersectRange(coordMatches).getDomain();

      stmt->setDomain(newDomain.take());
      stmt->setWhereMap(nullptr);
    }
  }; // class MollyScopStmtProcessorImpl
} // namespace 


MollyScopStmtProcessor *MollyScopStmtProcessor::create(MollyPassManager *pm, polly::ScopStmt *stmt) {
  return new MollyScopStmtProcessorImpl(pm, stmt);
}
