#include "FieldLayout.h"

#include "RectangularMapping.h"
#include "ClusterConfig.h"
#include "Codegen.h"
#include "FieldType.h"

#include "islpp/PwMultiAff.h"
#include "islpp/Map.h"

using namespace molly;
using namespace llvm;


molly::FieldLayout::~FieldLayout() {
  delete linearizer;
}


llvm::Value *FieldLayout::codegenLocalIndex(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator/* { ??? -> rank[cluster] } */, isl::MultiPwAff logicalCoords/* { ??? -> logical[indexset] } */) {
  assert(linearizer);
  auto physMap = getPhysicalLocal(); // { logical[coords], rank[cluster] -> physical[local] }
  auto place = rangeProduct(logicalCoords, domaintranslator).castRange(physMap.getDomainSpace()); /* { ??? -> logical[indexset],rank[cluster] } */
  auto physicalCoords = place.applyRange(physMap); /* { ??? -> physical[local] } */
  auto val = linearizer->codegenIndex(codegen, domaintranslator/* { ??? -> rank[cluster] } */, physicalCoords/* { ??? -> logical[indexset] } */);
  codegen.marker("local index", val);
  return val;
}


llvm::Value *FieldLayout::codegenLocalSize(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator) {
  assert(linearizer);
  return linearizer->codegenSize(codegen, domaintranslator);
}


FieldLayout *molly::FieldLayout::create(FieldType *fty, ClusterConfig *clusterConf, isl::Map relation) {
  // Rebuild relation to fit the environment
  auto logicalSpan = fty->getLogicalIndexset();
  relation.castDomain_inplace(logicalSpan.getSpace());
  assert(relation.domain() >= logicalSpan);
  relation.intersectDomain_inplace(logicalSpan);
  auto nodeRel = relation.wrap().reorderSubspaces(relation.getDomainSpace(), relation.getRangeSpace().unwrap().domain());
  assert(nodeRel.getRangeDimCount() == clusterConf->getClusterDims() && "Layout must match cluster dimensions");
  nodeRel.castRange_inplace(clusterConf->getClusterSpace());
  assert(nodeRel.range() <= clusterConf->getClusterShape() && "Map to nodes in the cluster only!"); 
  //nodeRel.intersectRange_inplace(clusterConf->getClusterShape());
  auto localRel = relation.wrap().reorderSubspaces(relation.getDomainSpace(), relation.getRangeSpace().unwrap().range());
  localRel.resetOutTupleId_inplace();
  auto mappedNodeSpan = nodeRel.domain();
  auto mappedLocalSpan = localRel;

  relation = rangeProduct(nodeRel, localRel).coalesce();
  auto linearizer = RectangularMapping::createRectangualarHullMapping(relation.range().unwrap());
  return new FieldLayout(fty, relation, linearizer);
}


llvm::Value *molly::FieldLayout::codegenLocalMaxSize(MollyCodeGenerator &codegen, isl::MultiPwAff domaintranslator) {
  return linearizer->codegenMaxSize(codegen, domaintranslator);
}


isl::Space molly::FieldLayout::getLogicalIndexsetSpace() const {
  assert(relation.getDomainSpace() == fty->getIndexsetSpace());
  return relation.getDomainSpace();
}


//isl::Map molly::FieldLayout::getIndexableIndices() const {
//  auto physIndexable = linearizer->getIndexableIndices();
//  return physIndexable;
//}
