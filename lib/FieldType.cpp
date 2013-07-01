#define DEBUG_TYPE "molly"
#include "FieldType.h"

#include "MollyContext.h"
#include "MollyContextPass.h"
#include "islpp/Ctx.h"
#include "islpp/Set.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/BasicSet.h"
#include "islpp/Constraint.h"
#include "islpp/Map.h"
#include "islpp/BasicMap.h"

#include <isl/space.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/IRBuilder.h>


#include <assert.h>

using namespace llvm;
using namespace molly;
using std::move;






isl::Ctx *FieldType::getIslContext() {
  return mollyContext->getIslContext();
}


llvm::LLVMContext *FieldType::getLLVMContext() {
  return mollyContext->getLLVMContext();
}


llvm::Module *FieldType::getModule() {
  return module;
}


FieldType::~FieldType() {
  int a = 0;
}


isl::BasicSet FieldType::getLogicalIndexset() {
  auto ctx = getIslContext();

  auto dims = getNumDimensions();
  auto space = ctx->createSetSpace(0, dims); //TODO: Assign an id to the tuple
  auto set = isl::BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = isl::Constraint::createInequality(space.copy());
    ge.setConstant_inplace(0);
    ge.setCoefficient_inplace(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = isl::Constraint::createInequality(move(space));
    lt.setConstant_inplace(getLengths()[d]);
    lt.setCoefficient_inplace(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }

  return set;
}


isl::Space FieldType::getLogicalIndexsetSpace() {
  auto islctx = getIslContext(); 
  auto dims = getNumDimensions();
  return islctx->createSetSpace(0, dims);
}


void FieldType::dump() {
  getLogicalIndexset().dump();
}


isl::Map FieldType::getDistributionMapping() {
  auto result = getDistributionAff().toMap();
  //TODO: index >= 0 && index < length[d]
  return result;
}


/* global coordinate -> node coordinate */
isl::MultiAff FieldType::getDistributionAff() {
  auto islctx = getIslContext();
  isl::MultiAff result;
  auto indexspace = getLogicalIndexsetSpace();

  auto lengths = getLengths();
  auto dims = lengths.size();
  for (auto d = 0; d < dims; d+=1){
    auto len = lengths[d];
    // gcoord = nodecoord*len + lcoord
    // => nodecoord = (gcoord - lcoord)/len
    // => nodecoord = [gcoord/len]
    auto gcoord = indexspace.createVarAff(isl_dim_set, d);
    auto nodecoord = div(gcoord.move(), indexspace.createConstantAff(len));
    result.push_back(nodecoord.move());
  }
  return result;
}


llvm::PointerType *FieldType::getEltPtrType() {
  auto eltType = getEltType();
  return PointerType::getUnqual(eltType);
}



static Value *emit(IRBuilderBase &builder, Value *value) {
  return value;
}


static Value *emit(IRBuilderBase &builder, uint32_t value) {
  return Constant::getIntegerValue(Type::getInt32Ty(builder.getContext()), APInt(32, (uint64_t)value, false)) ;
}


static Value *emit(IRBuilderBase &builder, bool value) {
  return Constant::getIntegerValue(Type::getInt32Ty(builder.getContext()), APInt(1, (uint64_t)value, false)) ;
}

template<typename T>
SmallVector<T*,4> iplistToSmallVector(iplist<T> &list) {
  SmallVector<T*,4> result;
  for (auto it = list.begin(), end = list.end(); it!=end;++it) {
    result.push_back(&*it);
  }
  return result;
}


isl::Set FieldType::getGlobalIndexset() {
  return getLogicalIndexset();
}


int FieldType::getGlobalLength(int d) {
  assert(0 <= d && d < getNumDimensions());
  return metadata.dimLengths[d];
}


isl::Map FieldType::getLocalIndexset(const isl::BasicSet &clusterSet) {
  auto nDims = getNumDimensions();//FIXME: Assumption cluster dimensions = field dimensions
  //auto space = getIslContext()->createMapSpace(0, nDims, nDims); 
  //auto domainSpace = getIslContext()->createSetSpace(0, nDims); // cluster coordinate
  //auto rangeSpace = getIslContext()->createSetSpace(0, nDims); 

  auto domain = clusterSet;
  auto range = getLogicalIndexset();
  auto space = isl::Space::createMapFromDomainAndRange(domain.getSpace(), range.getSpace());
  auto result = isl::BasicMap::createFromDomainAndRange(domain.copy(), range.copy());


  for (auto d = nDims-nDims; d<nDims; d+=1) {
    auto globalLen = getGlobalLength(d);
    auto localLen = getLocalLength(d);
    auto clusterLen = (globalLen + localLen - 1) / localLen;

    // globalCoord_low <= clustercoord * clusterLen
    auto c_low = isl::Constraint::createInequality(result.getSpace());
    c_low.setCoefficient_inplace(isl_dim_in, d, clusterLen);
    c_low.setCoefficient_inplace(isl_dim_out, d, -1);
    c_low.setConstant_inplace(0);

    // (clustercoord + 1) * clusterLen < globalCoord_hi
    // clustercoord * clusterLen + clusterLen < globalCoord_hi
    auto c_hi = isl::Constraint::createInequality(result.getSpace());
    c_hi.setCoefficient_inplace(isl_dim_in, d, clusterLen);
    c_hi.setCoefficient_inplace(isl_dim_out, d, -1);
    c_hi.setConstant_inplace(clusterLen);
  }

  return result.toMap();
}


/// { globalcoord -> nodecoord } where the value is stored between distributed SCoPs
isl::PwMultiAff FieldType::getHomeAff() {
  assert(isDistributed());
  auto coordSpace = getLogicalIndexsetSpace();
   //auto gcoordSpace = getLogicalIndexsetSpace();

  auto nDims = getNumDimensions();
 auto maff = getIslContext()->createMapSpace(coordSpace, nDims).createZeroMultiAff();
  auto i = 0;
  for (auto d = nDims-nDims; d<nDims; d+=1) {
    auto globalLen = getGlobalLength(d);
    auto localLen = getLocalLength(d);
    auto clusterLen = (globalLen + localLen - 1) / localLen;

    // [globalcoordinate/len]
    auto aff = floor(div(coordSpace.createVarAff(isl_dim_set, d), localLen));
    maff.setAff_inline(i, aff.move());
   i += 1;
  }

  return maff.restrictDomain(getLogicalIndexset());
}
