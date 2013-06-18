#include "ClusterConfig.h"

#include "islpp/Space.h"

using namespace molly;


isl::Space ClusterConfig::getClusterSpace() const {
  auto result = getIslContext()->createSetSpace(0, clusterLengths.size());
  return result;
}


unsigned  ClusterConfig::getClusterSize() const {
  	int result = 1;
	for (auto it = clusterLengths.begin(), end = clusterLengths.end(); it != end; ++it) {
		auto len = *it;
		result *= len;
	}
	assert(result >= 1);
  return result;
}
