#include "RectangularMapping.h"

#include "Codegen.h"
#include <llvm/IR/IRBuilder.h>

using namespace molly;
using namespace llvm;


RectangularMapping *RectangularMapping::create(isl::MultiPwAff lengths, isl::MultiPwAff offsets) {
  return new RectangularMapping(lengths.move(), offsets.move());
}


RectangularMapping *RectangularMapping::createRectangualarHullMapping(isl::Map map) { //FIXME: spelling
  auto nDims = map.getOutDimCount();
  auto lengths = isl::Space::createMapFromDomainAndRange(map.getDomainSpace(), map.getRangeSpace()).createEmptyMultiPwAff();
  auto offsets = lengths.copy();

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto min = map.dimMin(i);
    auto max = map.dimMax(i);
    auto length = max - min + 1;

    auto zeroLen = lengths[i];
    auto zeroIfUndef = unionMax(zeroLen, length);

    zeroIfUndef.simplify_inplace();
    //zeroIfUndef.gistUndefined_inplace();  /* have to keep lengths domain bounded for codegenMaxSize() */
    lengths.setPwAff_inplace(i, zeroIfUndef);

    min.simplify_inplace();
    min.gistUndefined_inplace();
    offsets.setPwAff_inplace(i, min);
  }

  //lengths.gistUndefined_inplace();
  //lengths.simplify_inplace();
  //lengths.gistUndefined_inplace();

  //offsets.simplify_inplace();
  //offsets.gistUndefined_inplace();

  assert(lengths.toMap().imageIsBounded());
#if 0
  auto d = lengths.getDomain();
 auto sample = d.sample();
auto x= lengths.toMap().intersectDomain(sample);
assert(x.imageIsBounded());
#endif

  return new RectangularMapping(lengths.move(), offsets.move());
}


llvm::Value *RectangularMapping::codegenIndex(MollyCodeGenerator &codegen, const isl::MultiPwAff &domain, const isl::MultiPwAff &coords) {
  auto nDims = lengths.getOutDimCount();
  assert(coords.getOutDimCount() == offsets.getOutDimCount());
  assert(nDims == offsets.getOutDimCount());
  auto &irBuilder = codegen.getIRBuilder();

  auto myoffsets = domain.isValid() ? offsets.pullback(domain) : offsets; // TODO: Instead of this pullback (case explosion), we can also generate the llvm::Value* for domain first, then use it for offsets and length
  auto mylengths = domain.isValid() ? lengths.pullback(domain) : lengths;

  Value *result = nullptr;
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto idx = coords.getPwAff(i) - myoffsets.getPwAff(i);

    if (!result) {
      result = codegen.codegenAff(idx);
      continue;
    }

    auto len = codegen.codegenAff(mylengths.getPwAff(i));
    auto prevScaled = irBuilder.CreateNSWMul(result, len);

    auto idxValue = codegen.codegenAff(idx);
    result = irBuilder.CreateNSWAdd(prevScaled, idxValue);
    //TODO: Codegen runtime checks
  }

  if (!result)
    return Constant::getNullValue(codegen.getIntTy());
  return result;
}


llvm::Value *RectangularMapping::codegenSize(MollyCodeGenerator &codegen, const isl::MultiPwAff &domain) {
  auto nDims = lengths.getOutDimCount();
  auto &irBuilder = codegen.getIRBuilder();

  auto mylengths = lengths.pullback(domain);

  Value *result=nullptr;
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto term = codegen.codegenAff(mylengths.getPwAff(i));
    result = result ? irBuilder.CreateNSWMul(result, term, "size") : term;
  }
  return result ? result : ConstantInt::get(Type::getInt64Ty(codegen.getLLVMContext()), 1);
}


llvm::Value *RectangularMapping::codegenMaxSize(MollyCodeGenerator &codegen, const isl::MultiPwAff &domaintranslator) {
  //domaintranslator; // { [domain] -> srcNode[cluster],dstNode[cluster] }
  //lengths; // { [chunk],srcNode[cluster],dstNode[cluster] -> field[indexset] }
  assert(lengths.toMap().imageIsBounded());
  auto mylengths = lengths.toMap().wrap().reorderSubspaces(domaintranslator.getRangeSpace(), lengths.getRangeSpace()); // { srcNode[cluster],dstNode[cluster] -> field[indexset] }
  assert(mylengths.imageIsBounded());
  auto lenout = domaintranslator.applyRange(mylengths); // { [domain] -> field[indexset] }

  // We cannot optimize over the non-linear size function, so we do it per dimension which hence is an overapproximation
  auto nDims = mylengths.getOutDimCount();
  auto &irBuilder = codegen.getIRBuilder();
  Value *result = ConstantInt::get(codegen.getIntTy(), 1, true);
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto max = lenout.dimMax(i);
    max.simplify_inplace();
    auto term = codegen.codegenAff(max);
    result = irBuilder.CreateNSWMul(result, term, "size");
  }
  return result;
}


isl::Map molly::RectangularMapping::getIndexableIndices() const {
  auto domain = lengths.getDomain();
  domain.intersect_inplace(offsets.getDomain());
  auto domainSpace = domain.getSpace();

  auto rangeSpace = lengths.getRangeSpace();
  assert(rangeSpace == offsets.getRangeSpace());

  auto resultSpace = isl::Space::createMapFromDomainAndRange(domain.getSpace(), rangeSpace);

  auto lowerBounds = offsets;
  auto upperBounds = offsets + lengths;

  auto lowerLimits = lowerBounds.applyRange(isl::BasicMap::createAllLe(rangeSpace));
  auto upperLimits = upperBounds.applyRange(isl::BasicMap::createAllGt(rangeSpace));
  auto result = intersect(lowerLimits, upperLimits);
  assert(result.imageIsBounded());
  return result;
}
