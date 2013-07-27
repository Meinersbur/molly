#include "ClusterConfig.h"

#include "islpp/Space.h"
#include "islpp/MultiAff.h"

using namespace molly;


isl::Space ClusterConfig::getClusterSpace() const {
  auto result = getIslContext()->createSetSpace(0, clusterLengths.size());
  result.setSetTupleId_inplace(getClusterTuple());
  return result;
}


unsigned ClusterConfig::getClusterSize() const {
  int result = 1;
  for (auto it = clusterLengths.begin(), end = clusterLengths.end(); it != end; ++it) {
    auto len = *it;
    result *= len;
  }
  assert(result >= 1);
  return result;
}


isl::MultiAff ClusterConfig::getMasterRank() const {
  auto islctx = getIslContext();
  auto nDims = getClusterDims();

  return isl::MultiAff::createZero(getClusterSpace());
}
