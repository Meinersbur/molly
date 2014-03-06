#include "FieldLayout.h"

#include "RectangularMapping.h"
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


FieldLayout * molly::FieldLayout::create(FieldType *fty, ClusterConfig *clusterConf, isl::PwMultiAff relation) {
  auto logicalSpan = fty->getLogicalIndexset();
  assert(relation.domain() <= logicalSpan);

  relation.intersectDomain_inplace(logicalSpan);
  auto mappedNodeSpan = relation.range().unwrap().domain();
  auto mappedLocalSpan = relation.range().unwrap();

  auto linearizer = RectangularMapping::createRectangualarHullMapping(mappedLocalSpan);

  return new FieldLayout(fty, relation, linearizer);
}


llvm::Value * molly::FieldLayout::codegenLocalMaxSize( MollyCodeGenerator &codegen, isl::PwMultiAff domaintranslator) {
  return linearizer->codegenMaxSize(codegen, domaintranslator);
}
