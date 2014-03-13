#include "FieldLayout.h"

#include "RectangularMapping.h"
#include "ClusterConfig.h"
#include "islpp/PwMultiAff.h"
#include "FieldType.h"
#include "islpp/Map.h"

using namespace molly;
using namespace llvm;


molly::FieldLayout::~FieldLayout() {
  delete linearizer;
}


llvm::Value *FieldLayout::codegenLocalIndex(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator, isl::MultiPwAff coords) {
  assert(linearizer);
  return linearizer->codegenIndex(codegen, domaintranslator, coords);

  //auto physicalcoords = relation.map(logicalCoord);

  // Pick one physical coordinate
  //auto picked = physicalcoords.lexmin();

  //return linearizer->codegenIndex(codegen, domaintranslator, physicalcoords);
}


llvm::Value *FieldLayout::codegenLocalSize(MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator) {
  assert(linearizer);
  return linearizer->codegenSize(codegen, domaintranslator);
}


FieldLayout * molly::FieldLayout::create(FieldType *fty, ClusterConfig *clusterConf, isl::Map relation) {
  // Rebuild relation to fit the environment
  auto logicalSpan = fty->getLogicalIndexset();
  relation.castDomain_inplace(logicalSpan.getSpace());
  assert(relation.domain() >= logicalSpan);
  relation.intersectDomain_inplace(logicalSpan);
  auto nodeRel = relation.wrap().reorderSubspaces(relation.getDomainSpace(), relation.getRangeSpace().unwrap().domain());
  nodeRel.castRange_inplace(clusterConf->getClusterSpace());
  nodeRel.intersectRange_inplace(clusterConf->getClusterShape());
  auto localRel = relation.wrap().reorderSubspaces(relation.getDomainSpace(), relation.getRangeSpace().unwrap().range());
  localRel.resetOutTupleId_inplace();
  auto mappedNodeSpan = nodeRel.domain();
  auto mappedLocalSpan = localRel;

  relation = rangeProduct(nodeRel, localRel);
  auto linearizer = RectangularMapping::createRectangualarHullMapping(relation.range().unwrap());
  return new FieldLayout(fty, relation, linearizer);
}


llvm::Value * molly::FieldLayout::codegenLocalMaxSize( MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator) {
  return linearizer->codegenMaxSize(codegen, domaintranslator);
}


isl::Space molly::FieldLayout::getLogicalIndexsetSpace() const {
  assert(relation.getDomainSpace() == fty->getIndexsetSpace());
  return relation.getDomainSpace();
}
