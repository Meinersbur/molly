#include "RectangularMapping.h"

#include <llvm/IR/IRBuilder.h>
#include "Codegen.h"
//#include "islpp/MultiPwAff.h"

using namespace molly;
using namespace llvm;


RectangularMapping *RectangularMapping::create(isl::MultiPwAff lengths, isl::MultiPwAff offsets) {
  return new RectangularMapping(lengths.move(), offsets.move());
}


RectangularMapping *RectangularMapping::createRectangualarHullMapping(isl::Map map) { //FIXME: spelling
  auto nDims = map.getOutDimCount();
  auto lengths = isl::Space::createMapFromDomainAndRange(map.getDomainSpace(), map.getRangeSpace()).createEmptyMultiPwAff();
  auto offsets = lengths.copy();

  for (auto i = nDims-nDims; i < nDims; i+=1) {
    auto min = map.dimMin(i);
    auto max = map.dimMax(i);
    auto length = max - min + 1;

    auto zeroLen = lengths[i];
    lengths.setPwAff_inplace(i, unionMax(zeroLen, length));
    offsets.setPwAff_inplace(i, min);
  }

  lengths.coalesce_inplace();
  offsets.coalesce_inplace();
  return new RectangularMapping(lengths.move(), offsets.move());
}


 //RectangularMapping *RectangularMapping::createRectangualarHullMapping(isl::Set set) {
 //  return createRectangualarHullMapping( alltoall( set.getParamsSpace().createUniverseBasicSet() , set) );
 //}


llvm::Value *RectangularMapping::codegenIndex(MollyCodeGenerator &codegen, const isl::PwMultiAff &domain, const isl::MultiPwAff &coords) {
  auto nDims = lengths.getOutDimCount();
  assert(nDims == coords.getOutDimCount());
  assert(nDims == offsets.getOutDimCount());
  auto &irBuilder = codegen.getIRBuilder();

  auto myoffsets = domain.isValid() ? offsets.pullback(domain) : offsets;
  auto mylengths = domain.isValid() ? lengths.pullback(domain) : lengths;
  //assert(matchesSpace(coords, mylengths));
  //assert(matchesSpace(coords, myoffsets));

  Value *result = nullptr;
  for (auto i = nDims-nDims; i < nDims; i+=1) {
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


llvm::Value *RectangularMapping::codegenSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domain) {
  auto nDims = lengths.getOutDimCount();
  auto &irBuilder = codegen.getIRBuilder();

  auto mylengths = lengths.pullback(domain);

  Value *result = ConstantInt::get(codegen.getIntTy(), 1, true);
  for (auto i = nDims-nDims; i < nDims; i+=1) {
    auto term = codegen.codegenAff(mylengths.getPwAff(i));
    result = irBuilder.CreateNSWMul(result, term, "size");
  }
  return result;
}


llvm::Value *RectangularMapping::codegenMaxSize(MollyCodeGenerator &codegen, const isl::PwMultiAff &domaintranslator) {
  //domaintranslator; // { [domain] -> srcNode[cluster],dstNode[cluster] }
  //lengths; // { [chunk],srcNode[cluster],dstNode[cluster] -> field[indexset] }

  auto mylengths = lengths.toMap().wrap().reorderSubspaces( domaintranslator.getRangeSpace(), lengths.getRangeSpace() ); // { srcNode[cluster],dstNode[cluster] -> field[indexset] }
  auto lenout = domaintranslator.applyRange(mylengths); // { [domain] -> field[indexset] }
 

 //auto wantedSpace = lengths.getDomainSpace();
 //auto underdefined = domaintranslator.toMultiPwAff().embedIntoRangeSpace(wantedSpace); // { [A,C] -> [A,B,C] } 
 //auto mylengths = lengths.pullback(underdefined.toPwMultiAff()); // { [A,C] -> len[] }, now get the max size of all [A,B]

 //auto lenout = mylengths.projectOut(isl_dim_in, 0, mylengths.getInDimCount()); // { -> len[] }
 //auto lengthsMap = mylengths.reverse(); // { len[] -> [A,C] }

 // We cannot optimize over the non-linear size function, so we do it per dimension which hence is an overapproximation
 auto nDims = mylengths.getOutDimCount();
 auto &irBuilder = codegen.getIRBuilder();
 Value *result = ConstantInt::get(codegen.getIntTy(), 1, true);
 for (auto i = nDims-nDims; i < nDims; i+=1) {
   auto max = lenout.dimMax(i);
   auto term = codegen.codegenAff(max);
   result = irBuilder.CreateNSWMul(result, term, "size");
 }
 return result;
}


#if 0
int RectangularMapping:: map(ArrayRef<unsigned> coords) const {
  assert(coords.size() == lengths.size());

  // Row-major
  auto nDims = lengths.size();
  int result = coords[0]- offsets[0];
  for (auto i = 1; i<nDims;i+=1) {
    assert(coords[i] >= offsets[i]);
    result = result * lengths[i-1] + (coords[i] - offsets[i]);
  }
  return result;
}


Value *RectangularMapping::codegen(llvm::IRBuilder<> &builder, llvm::ArrayRef<llvm::Value*> coords) {
  auto &llvmContext = builder.getContext();
  auto intTy = Type::getInt32Ty(llvmContext);
  auto nDims = getInputDims();
  assert(coords.size() == nDims);

  auto result = builder.CreateSub(coords[0], ConstantInt::get(intTy, offsets[0] ));
  for (auto i = 1; i<nDims; i+=1) {
    auto horn = builder.CreateMul(result, ConstantInt::get(intTy, lengths[i-1]));
    auto idx = builder.CreateSub(coords[i], ConstantInt::get(intTy, offsets[i]));
    result = builder.CreateAdd(horn, idx);
  }
  return result;
}
#endif
