#include "islpp/Aff.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Int.h"
#include "islpp/Id.h"
#include "islpp/Set.h"
#include "islpp/MultiAff.h"
#include "islpp/BasicSet.h"
#include "islpp/PwAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/Map.h"

#include <isl/aff.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallBitVector.h>

using namespace isl;
using namespace llvm;
using namespace std;


Aff isl::div(Aff &&aff1, const Int &divisor) {
  return div(std::move(aff1), aff1.getDomainSpace().createConstantAff(divisor));
}


ISLPP_EXSITU_ATTRS PwAff Aff::toPwAff() ISLPP_EXSITU_FUNCTION{
  return PwAff::enwrap(isl_pw_aff_from_aff(takeCopy()));
}


Aff Aff::createZeroOnDomain(LocalSpace &&space) {
  return Aff::enwrap(isl_aff_zero_on_domain(space.take()));
}


Aff Aff::createVarOnDomain(LocalSpace &&space, isl_dim_type type, unsigned pos) {
  return Aff::enwrap(isl_aff_var_on_domain(space.take(), type, pos));
}


Aff Aff::readFromString(Ctx *ctx, const char *str) {
  return Aff::enwrap(isl_aff_read_from_str(ctx->keep(), str));
}


void Aff::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


void Aff::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


bool Aff::involvesDims(isl_dim_type type, unsigned first, unsigned n) const{
  return isl_aff_involves_dims(keep(), type, first, n);
}
Space Aff::getDomainSpace() const {
  return Space::enwrap(isl_aff_get_domain_space(keep()));
}


LocalSpace Aff::getDomainLocalSpace() const{
  return LocalSpace::enwrap(isl_aff_get_domain_local_space(keep()));
}


Int Aff::getCoefficient(isl_dim_type type, unsigned pos) const {
  Int result;
  isl_aff_get_coefficient(keep(), type, pos, result.change());
  return result;
}


Int Aff::getDenominator() const {
  Int result;
  isl_aff_get_denominator(keep(), result.change());
  return result;
}

void Aff::setConstant_inplace(const Int &v)  ISLPP_INPLACE_FUNCTION{
  give(isl_aff_set_constant(take(), v.keep()));
}
ISLPP_INPLACE_ATTRS void Aff::setCoefficient_inplace(isl_dim_type type, unsigned pos, int v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_set_coefficient_si(take(), type, pos, v));
}
ISLPP_INPLACE_ATTRS void Aff::setCoefficient_inplace(isl_dim_type type, unsigned pos, const Int &v)ISLPP_INPLACE_FUNCTION{
  give(isl_aff_set_coefficient(take(), type, pos, v.keep()));
}
ISLPP_INPLACE_ATTRS void Aff::setDenominator_inplace(const Int &v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_set_denominator(take(), v.keep()));
}

ISLPP_INPLACE_ATTRS void Aff::addConstant_inplace(const Int &v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_constant(take(), v.keep()));
}
ISLPP_INPLACE_ATTRS void Aff::addConstant_inplace(int v)  ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_constant_si(take(), v));
}
ISLPP_INPLACE_ATTRS void Aff::addConstantNum_inplace(const Int &v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_constant_num(take(), v.keep()));
}
ISLPP_INPLACE_ATTRS void Aff::addConstantNum_inplace(int v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_constant_num_si(take(), v));
}
ISLPP_INPLACE_ATTRS void Aff::addCoefficient_inplace(isl_dim_type type, unsigned pos, int v)  ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_coefficient_si(take(), type, pos, v));
}
ISLPP_INPLACE_ATTRS void Aff::addCoefficient_inplace(isl_dim_type type, unsigned pos, const Int &v) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_add_coefficient(take(), type, pos, v.keep()));
}


bool Aff::isCst() const {
  return isl_aff_is_cst(keep());
}


bool Aff::isPlainZero() const {
  return isl_aff_plain_is_zero(keep());
}
Aff Aff::getDiv(int pos) const {
  return Aff::enwrap(isl_aff_get_div(keep(), pos));
}


void Aff::ceil() {
  give(isl_aff_ceil(take()));
}



void Aff::mod(const Int &v) {
  give(isl_aff_mod(take(), v.keep()));
}

void Aff::scale(const Int &f) {
  give(isl_aff_scale(take(), f.keep()));
}
void Aff::scaleDown(const Int &f) {
  give(isl_aff_scale_down(take(), f.keep()));
}
void Aff::scaleDown(unsigned f) {
  give(isl_aff_scale_down_ui(take(), f));
}

void Aff::insertDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_aff_insert_dims(take(), type, first, n));
}
void Aff::addDims(isl_dim_type type, unsigned n) {
  give(isl_aff_add_dims(take(), type, n));
}
void Aff::dropDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_aff_drop_dims(take(), type, first, n));
}

void Aff::projectDomainOnParams() {
  give(isl_aff_project_domain_on_params(take()));
}

void Aff::alignParams(Space &&model) {
  give(isl_aff_align_params(take(), model.take()));
}


void Aff::gistParams(Set &&context) {
  give(isl_aff_gist_params(take(), context.take()));
}


void Aff::pullback_inplace(const Multi<Aff> &ma) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_pullback_multi_aff(take(), ma.takeCopy()));
}

PwAff Aff::pullback(const PwMultiAff &pma) ISLPP_EXSITU_FUNCTION{
  auto resultSpace = pma.getDomainSpace().mapsTo(1);
  auto result = resultSpace.createEmptyPwAff();

  pma.foreachPiece([&result, this](Set &&set, MultiAff &&maff) -> bool {
    auto backpulled = this->pullback(maff);
    result.unionMin_inplace(PwAff::create(set, backpulled));
    return false;
  });

  return result;
}


ISLPP_EXSITU_ATTRS BasicMap isl::Aff::toBasicMap() ISLPP_EXSITU_FUNCTION{
  return BasicMap::enwrap(isl_basic_map_from_aff(takeCopy()));
}


ISLPP_EXSITU_ATTRS Map isl::Aff::toMap() ISLPP_EXSITU_FUNCTION{
  return Map::enwrap(isl_map_from_aff(takeCopy()));
}


ISLPP_EXSITU_ATTRS Aff isl::Aff::cast(Space space) ISLPP_EXSITU_FUNCTION{
  assert(getInDimCount() == space.getInDimCount());
  assert(getOutDimCount() == space.getOutDimCount());
  assert(::matchesSpace(getRangeSpace(), space.getRangeSpace()));

  return castDomain(space.getDomainSpace());
}


ISLPP_INPLACE_ATTRS void Aff::castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION{
  assert(getInDimCount() == domainSpace.getSetDimCount());

  auto transformDomainSpace = domainSpace.mapsTo(getDomainSpace());
  auto transformDomain = transformDomainSpace.createIdentityMultiAff();
  pullback_inplace(transformDomain);
}


ISLPP_EXSITU_ATTRS MultiAff isl::Aff::toMultiAff() ISLPP_EXSITU_FUNCTION{
  return MultiAff::enwrap(isl_multi_aff_from_aff(takeCopy()));
}


ISLPP_EXSITU_ATTRS PwMultiAff isl::Aff::toPwMultiAff() ISLPP_EXSITU_FUNCTION{
  return PwMultiAff::enwrap(isl_pw_multi_aff_from_multi_aff(isl_multi_aff_from_aff(takeCopy())));
}


void isl::Aff::dump() const {
  isl_aff_dump(keep());
}


ISLPP_INPLACE_ATTRS void isl::Aff::gist_inplace(Set context) ISLPP_INPLACE_FUNCTION{
  give(isl_aff_gist(take(), context.take()));
}


ISLPP_EXSITU_ATTRS Aff isl::Aff::gist(Set context) ISLPP_EXSITU_FUNCTION{
  return Aff::enwrap(isl_aff_gist(takeCopy(), context.take()));
}


ISLPP_PROJECTION_ATTRS Int isl::Aff::getDivCoefficient(const Aff &aff) ISLPP_PROJECTION_FUNCTION{
  auto nDivDims = getDivDimCount();
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    auto mydiv = getDiv(i);
    if (isPlainEqual(mydiv, aff))
      return getCoefficient(isl_dim_div, i);
  }
  return 0;
}


ISLPP_CONSUME_ATTRS PwAff isl::Aff::toPwAff_consume() ISLPP_CONSUME_FUNCTION{
  return PwAff::enwrap(isl_pw_aff_from_aff(take()));
}







static void addRoundedCoefficients(Aff &target, const Aff &div, const Int &globalNom, const Int &globalDenom, bool ceil) {
  if (globalNom.isZero())
    return;
  auto denom = globalDenom * div.getDenominator();

  auto nParamDims = isl_aff_dim(div.keep(), isl_dim_param);
  for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
    auto coeff = div.getCoefficient(isl_dim_param, i);
    auto addCoeff = ceil ? ceild(globalNom * coeff, denom) : floord(globalNom * coeff, denom);
    target.addCoefficient_inplace(isl_dim_param, i, addCoeff);
  }

  auto nInDims = isl_aff_dim(div.keep(), isl_dim_in);
  for (auto i = nInDims - nInDims; i < nInDims; i += 1) {
    auto coeff = div.getCoefficient(isl_dim_in, i);
    auto addCoeff = ceil ? ceild(globalNom * coeff, denom) : floord(globalNom * coeff, denom);
    target.addCoefficient_inplace(isl_dim_in, i, addCoeff);
  }

  auto nDivDims = isl_aff_dim(div.keep(), isl_dim_div);
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    auto coeff = div.getCoefficient(isl_dim_div, i);
    if (coeff.isZero())
      continue;

    // nested divs seem rather odd; we'll know if something is wrong if this is an endless recursion
    auto innerDiv = Aff::enwrapCopy(isl_aff_get_div(div.keep(), i));
    addRoundedCoefficients(target, innerDiv, globalNom * coeff, denom, ceil);
  }
}

ISLPP_INPLACE_ATTRS void Aff::removeDivsUsingFloor_inplace() ISLPP_INPLACE_FUNCTION{
  auto nDivs = isl_aff_dim(keep(), isl_dim_div);
  for (auto i = nDivs - nDivs; i < nDivs; i += 1) {
    auto coeff = getCoefficient(isl_dim_div, i);
    auto div = Aff::enwrap(isl_aff_get_div(keep(), i));
    addRoundedCoefficients(*this, div, coeff, 1, false);
  }
  for (auto i = nDivs - nDivs; i < nDivs; i += 1) {
    setCoefficient_inplace(isl_dim_div, i, 0);
  }
}


ISLPP_INPLACE_ATTRS void Aff::removeDivsUsingCeil_inplace() ISLPP_INPLACE_FUNCTION{
  auto nDivs = isl_aff_dim(keep(), isl_dim_div);
  for (auto i = nDivs - nDivs; i < nDivs; i += 1) {
    auto coeff = getCoefficient(isl_dim_div, i);
    auto div = Aff::enwrap(isl_aff_get_div(keep(), i));
    addRoundedCoefficients(*this, div, coeff, 1, true);
  }
  for (auto i = nDivs - nDivs; i < nDivs; i += 1) {
    setCoefficient_inplace(isl_dim_div, i, 0);
  }
}



BasicSet isl::zeroBasicSet(Aff &&aff) {
  return BasicSet::wrap(isl_aff_zero_basic_set(aff.take()));
}
BasicSet isl::negBasicSet(Aff &&aff) {
  return BasicSet::wrap(isl_aff_neg_basic_set(aff.take()));
}
BasicSet isl::leBasicSet(Aff &aff1, Aff &aff2) {
  return BasicSet::wrap(isl_aff_le_basic_set(aff1.take(), aff2.take()));
}
BasicSet isl::geBasicSet(Aff &aff1, Aff &aff2) {
  return BasicSet::wrap(isl_aff_ge_basic_set(aff1.take(), aff2.take()));
}


static int linearOps(const Aff &aff, bool countMuls, llvm::SmallBitVector &divUsed) {
  // if !countMuls, only count nonzero coefficients (and fdiv_q)
  auto nParmDims = aff.dim(isl_dim_param);
  auto nInDims = aff.dim(isl_dim_in);
  auto nDivDims = aff.dim(isl_dim_div);

  int ops = 0;

  Int cst = aff.getConstant();
  if (!cst.isZero())
    ops += 1; // add/sub
  for (auto i = nParmDims - nParmDims; i < nParmDims; i += 1) {
    Int coeff = aff.getCoefficient(isl_dim_param, i);
    if (coeff.isZero())
      continue;
    if (countMuls && !coeff.isAbsOne())
      ops += 1; // mul
    ops += 1; // add/sub
  }
  for (auto i = nInDims - nInDims; i < nInDims; i += 1) {
    Int coeff = aff.getCoefficient(isl_dim_in, i);
    if (coeff.isZero())
      continue;
    if (countMuls && !coeff.isAbsOne())
      ops += 1; // mul
    ops += 1; // add/sub
  }
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    Int coeff = aff.getCoefficient(isl_dim_div, i);
    if (coeff.isZero())
      continue;
    if (!divUsed[i]) {
      divUsed[i] = true;
      auto div = Aff::enwrap(isl_aff_get_div(aff.keep(), i));
      ops += linearOps(div, false, divUsed);
    }
    if (countMuls && !coeff.isAbsOne())
      ops += 1; // mul
    ops += 1; // add/sub
  }

  if (countMuls && ops > 0)
    ops -= 1; // Only additions/subtraction between, so we added one too much

  Int denom = aff.getDenominator();
  if (!denom.isAbsOne())
    ops += 6; // fdiv_q; this is an expensive operation so it costs multiple operations: (divident < 0) ? (-(-divident+divisor-1)/divisor) : (divident/divisor); TODO: 2^n divisions are cheaper

  return ops;
}


ISLPP_PROJECTION_ATTRS uint32_t Aff::getComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDivDims = dim(isl_dim_div);
  llvm::SmallBitVector divsUsed;
  divsUsed.resize(nDivDims);
  return linearOps(*this, false, divsUsed);
}


ISLPP_PROJECTION_ATTRS uint32_t Aff::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDivDims = dim(isl_dim_div);
  llvm::SmallBitVector divsUsed;
  divsUsed.resize(nDivDims);
  return linearOps(*this, true, divsUsed);
}


/// Remove unnecessary complexities from aff, 
static void normalizeDim(const BasicSet &set, Aff &aff, isl_dim_type type) {
  Int val;
  auto nDims = aff.dim(type);

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto coeff = aff.getCoefficient(type, i);
    if (coeff.isZero())
      continue;

    if (set.isFixed(type == isl_dim_param ? isl_dim_param : isl_dim_set, i, val)) {
      aff.addConstant_inplace(val * coeff);
      aff.setCoefficient_inplace(type, i, 0);
      //TODO: also set to zero in divs; currently we consider different divs unmergable, so this would make merging them more difficult
    }
    // TODO: For divs, even a range of input value may result to the same value
    // e.g. { [i] -> [(floor(i/2)) : 0 <= i and i < 2 ] }
  }
}


ISLPP_INPLACE_ATTRS void Aff::normalize_inplace(const BasicSet &domain) ISLPP_INPLACE_FUNCTION{
  normalizeDim(domain, *this, isl_dim_param);
  normalizeDim(domain, *this, isl_dim_in);
}



/// Given that all coefficients of maff and baff are equal, but not the constants; find a common expression that safisfies maff on mset and baff on bset
/// True is returned if succeeded and maff contains the common aff
/// If failing, returns false. maff is undefined in this case
template<typename BasicSet>
static bool tryCombineConstant(const Set &mset, const AllBounds &mBounds, Aff &maff, const BasicSet &bset, const AllBounds &bBounds, Aff &baff, isl_dim_type type) {
  Int mval, bval; // For reuse
  auto nDims = maff.dim(type);

  auto affspace = maff.getDomainLocalSpace();
  auto mcst = maff.getConstant();
  auto bcst = baff.getConstant();
  if (mcst == bcst)
    return true;

  auto domainType = type == isl_dim_in ? isl_dim_set : type;

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    auto mcoeff = maff.getCoefficient(type, i);
    auto bcoeff = baff.getCoefficient(type, i);

    if (!bBounds.isFixed(domainType, i, bval)) // TODO: Can also work with fixed range bounds that div to different constants
      continue;
    if (!mBounds.isFixed(domainType, i, mval))
      continue;
    if (bval == mval)
      continue; // No chance to make a difference

    // goal: find coeff s.t.
    // new_mcst + coeff*mval = mcst 
    // new_bcst + coeff*bval = bcst 
    // new_mcst = new_bcst <=> mcst - coeff*mval = bcst - coeff*bval
    // <=> mcst - bcst = coeff*mval - coeff*bval <=> mcst - bcst = coeff*(mval - bval)
    // <=> coeff = (mcst - bcst)/(mval - bval) 
    auto dcst = mcst - bcst;
    auto dval = mval - bval;
    if (mcoeff.isZero() && bcoeff.isZero() && isDivisibleBy(dcst, dval)) {
      auto coeff = divexact(dcst, dval);
      maff.setCoefficient_inplace(type, i, coeff);
      maff.addConstant_inplace(-coeff*mval);

      baff.setCoefficient_inplace(type, i, coeff);
      baff.addConstant_inplace(-coeff*bval);
      assert(maff == baff);

      return true;
    }
    //TODO: Non-div solution might be possible using multiple such dimensions
  }

  return false;
}


static bool overlappingBounds(const AllBounds &thisBounds, const AllBounds &thatBounds, isl_dim_type type, pos_t pos) {
  if (thisBounds.hasUpperBound(type, pos) && thatBounds.hasLowerBound(type, pos) &&
    (thisBounds.getUpperBound(type, pos) < thatBounds.getLowerBound(type, pos))
    ) {
    // [? .. thisUpperBound] < [thatLowerBound .. ?]
    return false;
  }

  if (thatBounds.hasUpperBound(type, pos) && thisBounds.hasLowerBound(type, pos) &&
    (thatBounds.getUpperBound(type, pos) < thisBounds.getLowerBound(type, pos))
    ) {
    // [? .. thisUpperBound] > [thatLowerBound .. ?]
    return false;
  }

  return true;
}


// Create an expression that is 0 on [mLowerBound,mUpperBound] and 1 on [bLowerBound..bUpperBound]
// Undefined everywhere else
static Aff makeIndicatorFunction(const LocalSpace &ls, isl_dim_type type, pos_t pos, const Int &mLowerBound, const Int &mUpperBound, const Int &bLowerBound, const Int &bUpperBound) {
  bool mIsSmaller = mLowerBound < bLowerBound;
  assert(mLowerBound <= mUpperBound);
  assert(bLowerBound <= bUpperBound);

  // Bounds may not intersect
  if (mIsSmaller) {
    assert(mUpperBound < bLowerBound);
  } else {
    assert(bUpperBound < mLowerBound);
  }

  auto mIntervalLength = mUpperBound - mLowerBound + 1;
  auto bIntervalLength = bUpperBound - bLowerBound + 1;
  bool mIsLonger = mIntervalLength >= bIntervalLength;

  Aff result;
  bool swap = false;
  auto x = ls.createVarAff(type, pos);
  // TODO: Depending on what is more efficient we might use smaller divisors or avoid the translation

  // x is in [mLowerBound .. mUpperBound] or [bLowerBound .. bUpperBound], disjoint

  if (mLowerBound < bLowerBound) {
    assert(mUpperBound < bLowerBound);
    // x is in [mLowerBound .. mUpperBound] or [bLowerBound .. bUpperBound], disjoint, in this order

    auto mid = cdiv_q(mLowerBound + bUpperBound, 2);
    mid = min(bLowerBound, max(mid, mUpperBound + 1));
    // mid is somewhere in (mUpperBound, bLowerBound]
    assert(mUpperBound < mid && mid <= bLowerBound);

    auto midbased = x - mid;
    // midbased is in [mLowerBound-mid .. mUpperBound-mid] or [bLowerBound-mid .. bUpperBound-mid], first all negative, second all non-negative
    assert(0 <= bLowerBound - mid);
    assert(mUpperBound - mid < 0);

    auto bLength = bUpperBound - mid + 1; 
    assert(bLength >= 0);
    auto mLength = mid - mLowerBound;
    assert(mLength>0);
    if (bLength >= mLength) {
      // second range is longer, use it as divisor
      result = 1+floord(midbased, bLength);
    } else {
      // first range is longer, use it as divisor
      result = -floord(-1 - midbased, mLength);
    }
  } else {
    assert(bUpperBound < mLowerBound);
    // x is in [bLowerBound .. bUpperBound] or [mLowerBound .. mUpperBound], disjoint, in this order

    auto mid = cdiv_q(bUpperBound + mLowerBound, 2);
    mid = min(mLowerBound, max(mid, bUpperBound + 1));
    // mid is somewhere in (bUpperBound, mLowerBound]
    assert(bUpperBound < mid && mid <= mLowerBound);

    auto midbased = x - mid;
    // midbased is in [bLowerBound-mid .. bUpperBound-mid] or [mLowerBound-mid .. mUpperBound-mid], first all negative, second all non-negative
    assert(0 <= mLowerBound - mid);
    assert(bUpperBound - mid < 0);

    auto mLength = mUpperBound - mid + 1; 
    assert(mLength>=0);
    auto bLength = mid - bLowerBound; 
    assert(bLength > 0);
    if (mLength >= bLength) {
      // m range is longer, use it as divisor
      result = -floord(midbased, mLength);
    } else {
      // b range is longer, use it as divisor
      auto g = -1 - midbased;
      result = 1 + floord(g, bLength);
    }
  }

  return result; // NRVO
}


// TODO: For better performance, we may pass pointers arrays instead of the Ints themselves
static Aff makePiecewiseFunction(const LocalSpace &ls, isl_dim_type type, pos_t pos, ArrayRef<const Int> lowerBounds, ArrayRef<const Int> upperBounds, ArrayRef<const Int> values) {
  auto nPieces = lowerBounds.size();
  assert(lowerBounds.size() == nPieces);
  assert(upperBounds.size() == nPieces);
  assert(values.size() == nPieces);
  assert(nPieces >= 1);

  SmallBitVector done;
  done.resize(nPieces);
  size_t nDone = 0;
  Aff result;
  Int lowerDone;
  Int upperDone;
  Int lower;

  // Permutation vector for ascending lowerBounds
  SmallVector<size_t, 4> order;
  order.reserve(nPieces);
  for (auto i = nPieces - nPieces; i < nPieces; i += 1) {
    order.push_back(i);
  }
  std::sort(order.begin(), order.end(), [&lowerBounds, &upperBounds](size_t a, size_t b) -> bool {
    return lowerBounds[a] < lowerBounds[b];
  });

  const auto &lowest = lowerBounds[order[0]];
  const auto &highest = upperBounds[order[nPieces - 1]];
  Int lastUpperBound;
  Int lastVal;

  size_t next = 0;
  while (next < nPieces) {
    auto i = order[next];
    const auto &lowerBound = lowerBounds[i];
    auto upperBound = upperBounds[i];
    const auto &value = values[i];
    next += 1;

    // If two neighbor ranges map to the same value, treat them as one
    for (auto lookahead = next; lookahead<nPieces; lookahead += 1) {
      auto j = order[lookahead];
      if (value != values[j])
        break;
      upperBound = upperBounds[j];
      next += 1;
    }
    if (lowerBound > upperBound)
      continue; // piece contains nothing

    if (result.isNull()) {
      result = ls.createConstantAff(value);
    } else {
      assert(lastUpperBound < lowerBound);
      //if (lowerBound == highest && lowest == lastUpperBound && isDivisibleBy(value-lastVal,highest-lowest)) {
      //  // Very special case where we can avoid a div
      //  result.addConstant_inplace(lastVal);
      //  result.addCoefficient_inplace(type, pos, divexact(value - lastVal, highest - lowest));
      //} else {
      auto indicator = makeIndicatorFunction(ls, type, pos, lowest, lastUpperBound, lowerBound, highest);
      result += (value - lastVal) * indicator;
      //}
    }
    lastUpperBound = upperBound;
    lastVal = value;
  }
  return result; // NRVO


#if 0
  Int lastVal;
  while (nDone < nPieces) {
    // searches smallest undone lowerbound
    int smallestIdx = -1;
    for (auto i = nPieces - nPieces; i < nPieces; i += 1) {
      if (done[i])
        continue;
      if ((smallestIdx == -1) || (lowerBounds[i] < lowerBounds[smallestIdx])) {
        smallestIdx = i;
      }
    }

    const auto &lowerBound = lowerBounds[smallestIdx];
    const auto &upperBound = upperBounds[smallestIdx];
    assert(lowerBound <= upperBound);
    const auto &value = values[smallestIdx];
    if (result.isNull()) {
      result = ls.createConstantAff(value);
      lowerDone = lowerBound;
    } else {
      assert(upperDone < lowerBound);
      auto indicator = makeIndicatorFunction(ls, type, pos, lowerBound, globalMax, lowerDone, upperDone);
      result += (value - lastVal)*indicator;
    }
    lastVal = value;
    upperDone = upperBound;
    done.set(smallestIdx);
    nDone += 1;
  }
  return result; // NRVO
#endif
}


static bool tryCombineConstantUsingDiv(const Set &mset, const AllBounds &mBounds, Aff &maff, const Set &bset, const AllBounds &bBounds, Aff &baff, isl_dim_type type) {
  Int mval, bval; // For reuse
  auto nDims = maff.dim(type);

  auto affspace = maff.getDomainLocalSpace();
  auto mcst = maff.getConstant();
  auto bcst = baff.getConstant();
  if (mcst == bcst)
    return true;

  auto domainType = type == isl_dim_in ? isl_dim_set : type;
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    if (!mBounds.isBounded(domainType, i))
      continue;
    if (!bBounds.isBounded(domainType, i))
      continue;
    if (overlappingBounds(mBounds, bBounds, domainType, i))
      continue;

    const auto &mLowerBound = mBounds.getLowerBound(domainType, i);
    const auto &mUpperBound = mBounds.getUpperBound(domainType, i);
    const auto &bLowerBound = bBounds.getLowerBound(domainType, i);
    const auto &bUpperBound = bBounds.getUpperBound(domainType, i);

    const Int lowerBounds[] = { mLowerBound, bLowerBound };
    const Int upperBounds[] = { mUpperBound, bUpperBound };
    const Int values[] = {
      0,
      bcst - mcst
    };
    auto pw = makePiecewiseFunction(affspace, domainType, i, lowerBounds, upperBounds, values);
    maff += pw;
    baff += pw;
    baff -= bcst - mcst; // Remove again what has been added by pw
    assert(maff == baff); // heuristic comparison, might fail even if correct
    return true;
  }

  return false;
}


#if 0
// mAff must have the longer interval
static void unifyUsingDiv(const Int &mLowerBound, const Int & mUpperBound, const Int &mcoeff, Aff &maff, const Int & bLowerBound, const Int & bUpperBound, const Int &bcoeff, Aff &baff, isl_dim_type type, pos_t pos) {
  auto mInterval = mUpperBound - mLowerBound + 1;
  auto bInterval = bUpperBound - bLowerBound + 1;

  auto ls = maff.getLocalSpace();
  auto x = ls.createVarAff(type, pos);

  Aff bDiv;
  Aff mDiv;
  if (mUpperBound < bLowerBound) {
    // [mLowerBound,mUpperBound] ... [bLowerBound,bUpperBound]
    // map [mLowerBound,mUpperBound] to 0
    // map [bLowerBound,bUpperBound] to 1
    // floor((x - mLowerBound)/(bLowerBound - mLowerBound))
    bDiv = isl::floor((x - mLowerBound) / (bLowerBound - mLowerBound));
    mDiv = 1 - bDiv;


  } else {
    // [bLowerBound,bUpperBound] ... [mLowerBound,mUpperBound]
    // map [bLowerBound,bUpperBound] to 0
    // map [mLowerBound,mUpperBound] to 1
    // floor((x - bLowerBound)/(mUpperBound - bUpperBound))
    mDiv = isl::floor((x - bLowerBound) / (mUpperBound - bUpperBound));

    // invert
    // 1 - floor((x - bLowerBound)/(mUpperBound - bUpperBound))
    bDiv = 1 - mDiv;
  }

  // Add bcoeff*x to maff
  maff += bcoeff*x;

  // Add mcoeff*x to baff
  baff += mcoeff*x;
}
#endif


static bool analyzeDiv(const Aff &div, isl_dim_type &type, pos_t &pos, Int &slope, Int &offset, Int &denominator) {
  auto nDivAffParamDims = div.dim(isl_dim_param);
  int coeffs = 0;
  for (auto k = nDivAffParamDims - nDivAffParamDims; k < nDivAffParamDims; k += 1) {
    auto coeff = div.getCoefficient(isl_dim_param, k);
    if (coeff.isZero())
      continue;
    slope = coeff;
    type = isl_dim_param;
    pos = k;
    coeffs += 1;
  }
  if (coeffs > 1)
    return false;
  auto nDivAffInputDims = div.dim(isl_dim_in);
  for (auto k = nDivAffInputDims - nDivAffInputDims; k < nDivAffInputDims; k += 1) {
    auto coeff = div.getCoefficient(isl_dim_in, k);
    if (coeff.isZero())
      continue;
    slope = coeff;
    type = isl_dim_in;
    pos = k;
    coeffs += 1;
  }
  if (coeffs > 1)
    return false;
  auto nDivAffDivDims = div.dim(isl_dim_div);
  for (auto k = nDivAffDivDims - nDivAffDivDims; k < nDivAffDivDims; k += 1) {
    auto coeff = div.getCoefficient(isl_dim_div, k);
    if (coeff.isZero())
      continue;
    slope = coeff;
    type = isl_dim_div;
    pos = k;
    coeffs += 1;
  }
  //if (coeffs > 1)
  //   return false;

#if 0
  offset = div.getConstant();
  auto divDenom = div.getDenominator();
  // div is floor((divCst+nom*x)/divDenom) where x is the variable of dimension <pos> of type <type>
  bool isNegDenm = divDenom.isNeg();
  divDenom.abs_inplace();
  step = fdiv_r(nom, divDenom); // i.e. 0 <= step < divDenom
  assert(0 <= step && step < divDenom); if (step * 2 > divDenom) {
    // counting down interval is larger
    step -= divDenom;
  }

  repeat = fdiv_q(divDenom, step.abs());
  // the function f(x):=floor(div(x)) is periodic:  f(offset)=f(offset+repeat*s) for any integer s
  return repeat > 1;
#endif
  offset = div.getConstant();

  // ensure positive denominator
  denominator = div.getDenominator();
  if (denominator.isNeg()) {
    denominator.neg_inplace();
    offset.neg_inplace();
    slope.neg_inplace();
  }

  //offset += denominator*fdiv_q(slope, denominator); // because of normalization of slope
  //slope = fdiv_r(slope, denominator); // normalize slope to be positive

  return coeffs == 1;
}


#if 0
static bool analyzeDivLinearPart(const Aff &div, isl_dim_type type, pos_t pos) {
  isl_dim_type divType;
  pos_t divPos;
  Int divSlope;
  if (!analyzeDiv(div, divType, divPos, divSlope))
    return false;
  if (divType != type)
    return false;
  if (divPos != divPos)
    return false;
  auto divOffset = div.getConstant();
  auto divDenom = div.getDenominator();
}
#endif


static bool isDivConstantOnRange(const Aff &div, isl_dim_type type, pos_t pos, const Int &lowerBound, const Int &upperBound, Int &constant) {
  isl_dim_type divType;
  pos_t divPos;
  Int divSlope;
  Int divOffset;
  Int divDenom;
  if (!analyzeDiv(div, divType, divPos, divSlope, divOffset, divDenom))
    return false;
  if (divType != type)
    return false;
  if (divPos != divPos)
    return false;

  // just evaluate both and whether they are equal
  constant = fdiv_q(divOffset + divSlope*lowerBound, divDenom);
  return constant == fdiv_q(divOffset + divSlope*upperBound, divDenom);
}


static bool getDivOnlyStep(Int divSlope, Int divOffset, const Int &divDenom, const Int &start, const Int &upperBound, Int &step) {
  assert(divDenom > 0);
  if (divSlope == 0)
    return false;

  // by normalization of isl_aff_floor()
  assert(-divDenom < 2 * divSlope && 2 * divSlope <= divDenom);
  auto integerSlope = fdiv_q(divSlope, divDenom);
  auto fractionalSlope = fdiv_r(divSlope, divDenom);

  auto nominatorAtStart = divOffset + start*divSlope;
  auto modAtStart = fdiv_r(nominatorAtStart, divDenom); // >= 0

  if (divSlope < 0) {
    assert(-divDenom < 2 * divSlope && divSlope < 0);
    int a = 0;

    // modAtStart + step*divSlope < 0           with smallest possible step
    // modAtStart + step*divSlope <= -1 
    // step*divSlope <= -1 - modAtStart
    // step >= (-1 - modAtStart)/divSlope       since divSlope is negative
    // step >= ceil((-1 - modAtStart)/divSlope) step must be greater if rhs has fractional part
    step = cdiv_q(-1 - modAtStart, divSlope) + start;
    assert(step > start);
    if (step > upperBound)
      return false;

    // modAtStart + step*divSlope < -divDenom
    auto second = cdiv_q(-1 - modAtStart - divDenom, divSlope) + start;
    assert(second > start);
    if (second <= upperBound)
      return false;
  } else {
    assert(0 < divSlope && 2 * divSlope <= divDenom);

    // Get index of next overflow (smallest firstStep):
    // modAtStart + firstStep*divSlope >= divDenom
    step = fdiv_q(divDenom - modAtStart, divSlope) + start;
    assert(step > start);
    if (step > upperBound) {
      // First step is already out of bounds
      return false;
    }

    // Get the second overflow
    auto second = fdiv_q(2 * divDenom - modAtStart, divSlope) + start;
    assert(second >= start);
    if (second <= upperBound) {
      // Second is within the bounds, do not support more than two pieces within the bounds
      return false;
    }
  }

#ifndef NDEBUG
  auto valAtStart = fdiv_q(nominatorAtStart, divDenom);
  auto valBeforeStep = fdiv_q(divOffset + (step - 1)*divSlope, divDenom);
  auto valAtStep = fdiv_q(divOffset + step*divSlope, divDenom); auto valAtUpper = fdiv_q(divOffset + upperBound*divSlope, divDenom);
  assert(valAtStart == valBeforeStep);
  assert(valBeforeStep != valAtStep);  assert(valAtStep == valAtUpper);
#endif /* NDEBUG */
  return true;
}


static bool isDivLinearOnRange(const Int &divSlope, const Int &divOffset, const Int &divDenom, const Int &lowerBound, const Int &upperBound, /*out*/Int &slope) {
  if (lowerBound == upperBound) {
    // Singular case; any slope is valid
    // Better handle in the caller
    slope = 0;
    return true;
  }
  assert(lowerBound <= upperBound);

  auto offsetAtLowerBound = fdiv_q(divOffset + divSlope*lowerBound, divDenom);
  auto offsetAtUpperBound = fdiv_q(divOffset + divSlope*upperBound, divDenom);
  auto boundLen = upperBound - lowerBound;
  if (lowerBound + 1 == upperBound) {
    // Two elements only: simple
    slope = offsetAtUpperBound - offsetAtLowerBound;
    return true;
  }

#if 0
  // normalize div
  assert(divDenom > 0);
  auto integerSlope = fdiv_q(divSlope, divDenom);
  auto fractionalSlope = fdiv_r(divSlope, divDenom); // fractionalSlope \in [1 .. divDenom-1]
  if (fractionalSlope == 0) {
    slope = integerSlope;
    return true;
  }

  if (2 * fractionalSlope <= divDenom) {
    // constant fraction on range?
    slope = integerSlope;
    auto fractionValueAtLowerBound = fdiv_q(divOffset + fractionalSlope*lowerBound, divDenom);
    auto fractionValueAtUpperBound = fdiv_q(divOffset + fractionalSlope*upperBound, divDenom);
    return fractionValueAtLowerBound == fractionValueAtUpperBound;
  }
#endif

  // Formula: floor((divOffset + divSlope*x)/divDenom); 1<divDenom

  // 0. Step: Normalize lowerBound to zero
  auto normOffset = divOffset + divSlope*lowerBound;
  // Formula: floor((normOffset + divSlope*(x-lowerBound)) / divDenom); 1<divDenom
  assert(offsetAtLowerBound == fdiv_q(normOffset, divDenom));
  assert(offsetAtUpperBound == fdiv_q(normOffset + divSlope*(upperBound-lowerBound), divDenom));


  // 1. Step: ensure 0<=normOffset<divDenom
  auto normIntOffset = fdiv_q(normOffset, divDenom);
  auto normFracOffset = fdiv_r(normOffset, divDenom);
  // Formula: floor((normFracOffset + divSlope*(x-lowerBound)) / divDenom) + normIntOffset; 1<divDenom; 0<=fractionalOffset<divDenom
  assert(offsetAtLowerBound == (fdiv_q(normFracOffset, divDenom) + normIntOffset));
  assert(offsetAtUpperBound == (fdiv_q(normFracOffset + divSlope*(upperBound - lowerBound), divDenom) + normIntOffset));

  // 2. Step: ensure 1<=divSlope<divDenom
  auto normIntSlope = fdiv_q(divSlope, divDenom);
  auto normFracSlope = fdiv_r(divSlope, divDenom);
  // Formula: floor((normFracOffset + normFracSlope*(x-lowerBound)) / divDenom) + normIntOffset + normIntSlope*(x-lowerBound)
  assert(offsetAtLowerBound == (fdiv_q(normFracOffset, divDenom) + normIntOffset));
  assert(offsetAtUpperBound == (fdiv_q(normFracOffset + normFracSlope*(upperBound - lowerBound), divDenom) + normIntOffset + normIntSlope*(upperBound - lowerBound)));

  if (normFracOffset + boundLen*normFracSlope < divDenom) {
   slope = 0;
   assert(offsetAtLowerBound == offsetAtUpperBound);
   return true;
  }

  auto revSlope = divDenom - normFracSlope;
  auto revOffset = divDenom - normFracOffset;
  if (revSlope*boundLen + revOffset-1 < divDenom) {
    slope = normIntSlope + 1;
    auto residue = offsetAtLowerBound - slope*lowerBound;
    assert(offsetAtUpperBound == slope*(upperBound - lowerBound) + residue);
    return true;
  }
  return false;


#if 0
  auto integerOffset = fdiv_q(divOffset, divDenom);
  auto fractionalOffset = fdiv_r(divOffset, divDenom);

  auto reverseSlope = divDenom - fractionalSlope; // [1 .. divDenom-1]
  auto offsetMod = fdiv_r(divOffset, divDenom);
  slope = integerSlope;
  slope += fractionalSlope.sgn(); // always 1

  // first overflow happend when divOffset(mod divDenom) + reverseSlope * s >= divDenom with smallest s possible
  // i.e. s = ceil((divDenom - offsetMod)/reverseSlope)
  auto firstPeriodEnd = cdiv_q(divDenom - offsetMod, reverseSlope);

  // second overflow happend when divOffset(mod divDenom) + reverseSlope * s >= 2*divDenom with smalles s possible
  auto secondPeriodEnd = cdiv_q(2 * divDenom - offsetMod, reverseSlope);

  // divOffset + reverseSlope*x >= timesOverflow*divDenom   with largest possible timesOverflow

  // find timesOverflow s.t.
  // timesOverflow*divDenom <= divOffset + reverseSlope*x < (timesOverflow+1)*divDenom
  auto lowerTimesOverflow = fdiv_q(divOffset + reverseSlope*(lowerBound - 1), divDenom);
  auto upperTimesOverflow = fdiv_q(divOffset + reverseSlope*(upperBound - 1), divDenom);

  bool samePeriod = divOffset + reverseSlope*(upperBound - 1) <= (lowerTimesOverflow + 1)*divDenom;
  bool samePeriod2 = lowerTimesOverflow == upperTimesOverflow;
  assert(samePeriod == samePeriod2);
  // first overflow when divOffset(mod divDenom) + reverseSlope * s >= divDenom
  // hence:  s <= floor((divDenom - offsetMod)/reverseSlope)
  auto periodicity = fdiv_q(divDenom - offsetMod, reverseSlope);

  // period starts before the first wraparound
  // offsetMod + divSlope * x < divDenom
  // hence: ceil(divDenom - offsetMod-1)/divSlope
  auto periodStart = cdiv_q(divDenom - offsetMod - 1, divSlope);

  // period of lowerBound
  auto s = fdiv_q(lowerBound - periodStart, periodicity);

  // period of upperBound
  auto t = fdiv_q(upperBound - periodStart, periodicity);

  //return s == t;
  return samePeriod;
#endif
}


static bool isDivLinearOnRange(const Aff &div, isl_dim_type type, pos_t pos, const Int &lowerBound, const Int &upperBound, Int &offsetAtLowerBound, Int &slope) {
  assert(lowerBound != upperBound); // Singular case; any slope is valid
  assert(lowerBound <= upperBound);
  isl_dim_type divType;
  pos_t divPos;
  Int divSlope;
  Int divOffset;
  Int divDenom;
  if (!analyzeDiv(div, divType, divPos, divSlope, divOffset, divDenom))
    return false;
  if (divType != type)
    return false;
  if (divPos != divPos)
    return false;


  offsetAtLowerBound = fdiv_q(divOffset + divSlope*lowerBound, divDenom);
  return isDivLinearOnRange(divSlope, divOffset, divDenom, lowerBound, upperBound, slope);
#if 0
  if (lowerBound + 1 == upperBound) {
    // Two elements only: simple
    slope = fdiv_q(divOffset + divSlope*upperBound, divDenom) - offsetAtLowerBound;
    return true;
  }

  // normalize div
  assert(divDenom > 0);
  auto integerSlope = fdiv_q(divSlope, divDenom);
  auto fractionalSlope = fdiv_r(divSlope, divDenom); // fractionalSlope \in [1 .. divDenom-1]
  if (fractionalSlope == 0) {
    slope = integerSlope;
    return true;
  }

  if (2 * fractionalSlope < divDenom) {
    // constant fraction on range?
    slope = integerSlope;
    auto fractionValueAtLowerBound = fdiv_q(divOffset + fractionalSlope*lowerBound, divDenom);
    auto fractionValueAtUpperBound = fdiv_q(divOffset + fractionalSlope*upperBound, divDenom);
    return fractionValueAtLowerBound == fractionValueAtUpperBound;
  }

  auto integerOffset = fdiv_q(divOffset, divDenom);
  auto fractionalOffset = fdiv_r(divOffset, divDenom); // fractionalOffset \in [0 .. divDenom-1]


  auto reverseSlope = divDenom - fractionalSlope; // [1 .. divDenom-1]
  auto offsetMod = fdiv_r(divOffset, divDenom);
  slope = integerSlope;
  slope += fractionalSlope.sgn(); // always 1

  // first overflow happend when divOffset(mod divDenom) + reverseSlope * s >= divDenom with smalles s possible
  // i.e. s = ceil((divDenom - offsetMod)/reverseSlope)
  auto firstPeriodEnd = cdiv_q(divDenom - offsetMod, reverseSlope);

  // second overflow happend when divOffset(mod divDenom) + reverseSlope * s >= 2*divDenom with smalles s possible
  auto secondPeriodEnd = cdiv_q(2 * divDenom - offsetMod, reverseSlope);

  // divOffset + reverseSlope*x >= timesOverflow*divDenom   with largest possible timesOverflow

  // find timesOverflow s.t.
  // timesOverflow*divDenom <= divOffset + reverseSlope*x < (timesOverflow+1)*divDenom
  auto lowerTimesOverflow = fdiv_q(divOffset + reverseSlope*(lowerBound - 1), divDenom);
  auto upperTimesOverflow = fdiv_q(divOffset + reverseSlope*(upperBound - 1), divDenom);

  bool samePeriod = divOffset + reverseSlope*(upperBound - 1) <= (lowerTimesOverflow + 1)*divDenom;
  bool samePeriod2 = lowerTimesOverflow == upperTimesOverflow;
  assert(samePeriod == samePeriod2);
  // first overflow when divOffset(mod divDenom) + reverseSlope * s >= divDenom
  // hence:  s <= floor((divDenom - offsetMod)/reverseSlope)
  auto periodicity = fdiv_q(divDenom - offsetMod, reverseSlope);

  // period starts before the first wraparound
  // offsetMod + divSlope * x < divDenom
  // hence: ceil(divDenom - offsetMod-1)/divSlope
  auto periodStart = cdiv_q(divDenom - offsetMod - 1, divSlope);

  // period of lowerBound
  auto s = fdiv_q(lowerBound - periodStart, periodicity);

  // period of upperBound
  auto t = fdiv_q(upperBound - periodStart, periodicity);

  //return s == t;
  return samePeriod;
#endif
}


static bool matchesPart(const Int &lowerBound, const Int &upperBound, const Int &offset, const Int &repeat, Int &multipleOfRepeat) {
  assert(repeat > 0);
  auto rangeLower = cdiv_q(lowerBound - offset, repeat);
  auto rangeUpper = rangeLower + repeat;
  assert(lowerBound <= upperBound);
  return (rangeLower <= lowerBound) && (upperBound <= rangeUpper);
}


static bool getDivAffinePart(const Aff &div, isl_dim_type type, pos_t pos, Int &offset, Int &repeat, Int &step) {
  auto nDivAffParamDims = div.dim(isl_dim_param);
  for (auto k = nDivAffParamDims - nDivAffParamDims; k < nDivAffParamDims; k += 1) {
    if (type == isl_dim_param&&k == pos)
      continue;
    auto coeff = div.getCoefficient(isl_dim_param, k);
    if (coeff.isZero())
      continue;
    return false;
  }
  auto nDivAffInputDims = div.dim(isl_dim_in);
  for (auto k = nDivAffInputDims - nDivAffInputDims; k < nDivAffInputDims; k += 1) {
    if (type == isl_dim_in&&k == pos)
      continue;
    auto coeff = div.getCoefficient(isl_dim_param, k);
    if (coeff.isZero())
      continue;
    return false;
  }
  auto nDivAffDivDims = div.dim(isl_dim_div);
  for (auto k = nDivAffDivDims - nDivAffDivDims; k < nDivAffDivDims; k += 1) {
    if (type == isl_dim_in&&k == pos)
      continue;
    auto coeff = div.getCoefficient(isl_dim_param, k);
    if (coeff.isZero())
      continue;
    return false;
  }

  auto divCst = div.getConstant();
  auto divDenom = div.getDenominator();
  bool isNegDenm = divDenom.isNeg();
  divDenom = divDenom.abs();
  auto divMulti = div.getCoefficient(type, pos);

  divMulti = fdiv_r(divMulti, divDenom); // i.e. 0 <= divMulti  < divDenom
  if (divMulti * 2 > divDenom) {
    // counting down interval is larger
    divMulti -= divDenom;
  }

  repeat = fdiv_q(divMulti.abs(), divDenom);
  step = divMulti;

  return repeat > 1; // there are more effective ways than single-value solutions 
}


#if 0
static bool unifyUsingDiv(const Aff &div, Aff &aff, isl_dim_type type, pos_t pos, const Int &missingCoeff, const Int &lowerBound, const Int &upperBound, bool tryDivs) {
  Int offset;
  Int step;
  Int interval;
  if (!getDivAffinePart(div, type, pos, offset, interval, step))
    return false;

  if (!isDivisibleBy(missingCoeff, step))
    return false;
  auto multi = divexact(missingCoeff, step);


  // offset + x*interval <= lowerBound <= upperBound <= offset + (x+1)*interval
  // solve for x

  auto ls = aff.getLocalSpace();
  auto lowX = fdiv_q(lowerBound - offset, interval);
  auto lowerInterval = offset + lowX*interval;
  auto hiX = lowX + 1;
  auto upperInterval = offset + hiX*interval;
  assert(lowerInterval <= lowerBound);
  assert(lowerBound <= upperBound);
  if (upperBound <= upperInterval) {
    // Some interval completely fills the bounds
    auto var = ls.createVarAff(type, pos);
    aff += missingCoeff*var;
    aff += missingCoeff*multi*floor(div);
    assert(!"Must add some constant offset");
    return true;
  }

  if (!tryDivs)
    return false;

  if (upperBound - 1 == upperInterval) {
    // interval fits bound except a single value, which we handle separately
    if (!unifyUsingDiv(div, aff, type, pos, missingCoeff, lowerBound, upperBound - 1, tryDivs))
      return false;

    auto indicator = makeIndicatorFunction(ls, type, pos, lowerBound, upperBound - 1, upperBound, upperBound);
    auto offset = indicator*missingCoeff;
    aff += offset;
    return true;
  }

  if (lowerBound + 1 == upperInterval) {
    // Other way around
    if (!unifyUsingDiv(div, aff, type, pos, missingCoeff, lowerBound + 1, upperBound, tryDivs))
      return false;

    auto indicator = makeIndicatorFunction(ls, type, pos, lowerBound, lowerBound, lowerBound + 1, upperBound);
    auto offset = indicator*missingCoeff;
    aff += offset;
    return true;
  }

  return false;
}
#endif


static bool unifyDimPos(const AllBounds &mBounds, Aff &maff, const AllBounds &bBounds, Aff &baff, isl_dim_type type, pos_t i, bool tryDivs) {
  auto mcoeff = maff.getCoefficient(type, i);
  auto bcoeff = baff.getCoefficient(type, i);
  if (mcoeff == bcoeff)
    return true; // Already equal, Nothing to do

  auto domainType = type == isl_dim_in ? isl_dim_set : type;
#if 0
  if (bcoeff.isZero() && mBounds.isFixed(domainType, i)) {
    //auto ls = baff.getLocalSpace();
    //auto val = ls.createVarAff(type, i);
    const auto &val = mBounds.getFixed(domainType, i);

    // baff is available for change
    baff.setCoefficient_inplace(type, i, mcoeff);

    // Adapt the constant
    baff.addConstant_inplace(-mcoeff*val);
    return true;
  }

  if (mcoeff.isZero() && bBounds.isFixed(domainType, i)) {
    //auto ls = maff.getLocalSpace();
    //auto val = ls.createVarAff(type, i);
    const auto &val = bBounds.getFixed(domainType, i);

    // maff is available for change
    maff.setCoefficient_inplace(type, i, bcoeff);

    // Adapt the constant
    maff.addConstant_inplace(-bcoeff*val);
    return true;
  }
#endif

  if (bBounds.isFixed(domainType, i)) {
    const auto &bFixed = bBounds.getFixed(domainType, i);

    // 1) baff is avail
    baff.addCoefficient_inplace(type, i, mcoeff);

    // 2) With change of 1) we effectively added a constant to baff at its only domain value
    auto val = (mcoeff - bcoeff)*bFixed;
    baff -= val;

    return true;
  }

  if (mBounds.isFixed(domainType, i)) {
    const auto &mFixed = mBounds.getFixed(domainType, i);

    // 1) maff is avail
    maff.addCoefficient_inplace(type, i, bcoeff);

    // 2) With change of 1) we effectively added a constant to maff at its only domain value
    auto val = (bcoeff - mcoeff)*mFixed;
    maff -= val;

    return true;
  }

#if 0
  if (!tryDivs)
    return false;

  if (bBounds.isBounded(domainType, i) && mBounds.isBounded(domainType, i) && !overlappingBounds(bBounds, mBounds, domainType, i)) {
    const auto &bLowerBound = bBounds.getLowerBound(domainType, i);
    const auto &bUpperBound = bBounds.getUpperBound(domainType, i);
    const auto &mLowerBound = mBounds.getLowerBound(domainType, i);
    const auto &mUpperBound = mBounds.getUpperBound(domainType, i);

    Int pwLowerBounds[] = { bLowerBound, mLowerBound };
    Int pwUpperBounds[] = { bUpperBound, mUpperBound };
    Int pwValues[] = {
    };
    auto pw = makePiecewiseFunction(maff.getDomainLocalSpace(), type, pos, pwLowerBounds, pwUpperBounds, pwValues);
    maff += pw;
  }
#endif

#if 0
  return false;
  if (!hasFlag(mHasBounds, HasBounds::Bounded))
    return false;

  // Intersecting bounds
  if (bLowerBound <= mLowerBound && mLowerBound <= bUpperBound)
    return false;
  if (bLowerBound <= mUpperBound && mUpperBound <= bUpperBound)
    return false;
  if (bLowerBound <= mLowerBound && mUpperBound <= bUpperBound)
    return false;
  if (mLowerBound <= bLowerBound && bUpperBound <= mUpperBound)
    return false;

  auto mInterval = mUpperBound - mLowerBound + 1;
  auto bInterval = bUpperBound - bLowerBound + 1;

#if 0
  // Try find some div that can be used 
  auto mDivDims = maff.dim(isl_dim_div);
  for (auto j = mDivDims - mDivDims; j < mDivDims; j += 1) {
    auto div = maff.getDiv(j);

    if (unifyUsingDiv(div, baff, type, i, mcoeff/*this assumes */, bLowerBound, bUpperBound, tryDivs))
      return true;
  }
#endif
#endif
  // TODO: with disjoint,bounded bounds, we might add some makePiecewiseFunction to both
  return false;
}


/// Given two affine expressions, try unifying the coefficients of the the type dimensions s.t. the expression results are still the same on their dimensions
/// Returns true if succeeded, maff and baff contains the changed expressions in this case, with the tuple coefficients equal, the other coefficients unchanged, but the the constant offset might be changed
/// If failed, returns false; maff and baff are undefined
static bool tryConstantCombineTuple(const AllBounds &mBounds, Aff &maff, const AllBounds &bBounds, Aff &baff, isl_dim_type type, bool tryDivs) {
  auto nDims = maff.dim(type);

  for (auto i = nDims - nDims; i < nDims; i += 1) {
    if (!unifyDimPos(mBounds, maff, bBounds, baff, type, i, tryDivs))
      return false;
  }

  return true;
}



static bool unifyDivsDisjountDomains(const Int &divCoeff, const Aff &div, isl_dim_type type, pos_t pos, const Int &lowerBound, const Int &upperBound, const Int &preserveLowerBound, const Int &preserveUpperBound, Aff &addToAff, Aff &addToBoth,bool tryCoeffs, bool tryDivs) {
  assert(lowerBound <= upperBound);
  assert(preserveLowerBound <= preserveUpperBound);
  assert((upperBound < preserveLowerBound) || (lowerBound > preserveUpperBound));

  isl_dim_type divType;
  pos_t divPos;
  Int divSlope;
  Int divOffset;
  Int divDenom;
  if (!analyzeDiv(div, divType, divPos, divSlope, divOffset, divDenom))
    return false;
  if (divType != type)
    return false;
  if (divPos != pos)
    return false;
  auto domainType = (type == isl_dim_in) ? isl_dim_set : isl_dim_param;

  Int constant;
  Int slope;
  if (tryCoeffs) {
  // Also possible without known bounds to preserve
  if (isDivLinearOnRange(div, divType, divPos, lowerBound, upperBound, constant, slope)) {
    // 1) make <aff> and <other> equal 
    addToAff = divCoeff*floor(div);

    // 2) subtract the linear part that we added from aff again
    // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
    addToAff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);

    // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
    addToAff.addConstant_inplace(constant - divCoeff*slope*lowerBound);

    addToBoth = div.getDomainLocalSpace().createZeroAff();
    return true;
  }
  }

  if (!tryDivs)
    return false;

  // TODO: Use getDivOnlyStep()
  auto lowerBoundp1 = lowerBound + 1;
  auto upperBoundm1 = upperBound - 1;

  // lowerBound [lowerBoundp1 .. upperBound]
  if (isDivLinearOnRange(div, divType, divPos, lowerBoundp1, upperBound, constant, slope)) {
    // 1) make <aff> and <other> equal 
    addToAff = divCoeff*floor(div);
    auto singularOffset = divCoeff*fdiv_q(divOffset + divSlope*lowerBound, divDenom);

    // 2) subtract the linear part that we added from aff again
    // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
    addToAff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);
    singularOffset -= slope*divCoeff*lowerBound;

    // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
    addToAff.addConstant_inplace(constant - divCoeff*slope*lowerBoundp1);
    singularOffset += constant - divCoeff*slope*lowerBoundp1;

    // 4) Finally ensure that the single value of lowerBound does not change
    // makePiecewiseFunction returns divs itself, so we cannot rely on any later phase to correct it again, i.e. the same divs must be added to <aff> and <other>
    Int pwLowerBounds[] = { lowerBound, lowerBoundp1, preserveLowerBound };
    Int pwUpperBounds[] = { lowerBound, upperBound, preserveUpperBound };
    Int pwValues[] = {
      -singularOffset, // the difference between the linear value and the true value at lowerBound
      0, // do not change other in range [otherLowerBound .. otherUpperBound]
      0, // has been coped with in 2)
    };
    addToBoth = makePiecewiseFunction(div.getDomainLocalSpace(), domainType, divPos, pwLowerBounds, pwUpperBounds, pwValues);
    return true;
  }

  // [lowerBound .. upperBoundm1] upperBound
  if (isDivLinearOnRange(div, divType, divPos, lowerBound, upperBoundm1, constant, slope)) {
    // 1) make <aff> and <other> equal
    addToAff = divCoeff*floor(div);
    auto singularOffset = divCoeff*fdiv_q(divOffset + divSlope*upperBound, divDenom);

    // 2) subtract the linear part that we added from aff again
    // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
    addToAff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);
    singularOffset -= slope*divCoeff*upperBound;

    // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
    addToAff.addConstant_inplace(constant - divCoeff*slope*lowerBound);
    singularOffset += constant - divCoeff*slope*upperBound;

    // 4) Finally ensure that the single value of lowerBound does not change
    // makePiecewiseFunction returns divs itself, so we cannot rely on any later phase to correct it again, i.e. the same divs must be added to <aff> and <other>
    Int pwLowerBounds[] = { lowerBound, upperBound, preserveLowerBound };
    Int pwUpperBounds[] = { upperBoundm1, upperBound, preserveUpperBound };
    Int pwValues[] = {
      0,
      -singularOffset,
      0,
    };
    addToBoth = makePiecewiseFunction(div.getDomainLocalSpace(), domainType, divPos, pwLowerBounds, pwUpperBounds, pwValues);
    return true;
  }

  return false;
}


static bool addDivIfPossibleWithBounds(const Int & lowerBound, const Aff & div, isl_dim_type divType, pos_t divPos, const Int & upperBound, Int constant, Int slope, Aff &aff, const Int & divCoeff, const Int &divOffset, const Int &divSlope, const Int &divDenom, const Int & otherLowerBound, const Int & otherUpperBound, isl_dim_type domainType, Aff &other) {
  if (upperBound < otherLowerBound || lowerBound > otherUpperBound) {
    // disjoint bounds

    auto lowerBoundp1 = lowerBound + 1;
    if (isDivLinearOnRange(div, divType, divPos, lowerBoundp1, upperBound, constant, slope)) {
      // 1) make <aff> and <other> equal 
      aff += divCoeff*floor(div);
      auto singularOffset = divCoeff*fdiv_q(divOffset + divSlope*lowerBound, divDenom);

      // 2) subtract the linear part that we added from aff again
      // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
      aff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);
      singularOffset -= slope*divCoeff*lowerBound;

      // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
      aff.addConstant_inplace(constant - divCoeff*slope*lowerBoundp1);
      singularOffset += constant - divCoeff*slope*lowerBoundp1;

      // 4) Finally ensure that the single value of lowerBound does not change
      // makePiecewiseFunction returns divs itself, so we cannot rely on any later phase to correct it again, i.e. the same divs must be added to <aff> and <other>
      Int pwLowerBounds[] = { lowerBound, lowerBoundp1, otherLowerBound };
      Int pwUpperBounds[] = { lowerBound, upperBound, otherUpperBound };
      Int pwValues[] = {
        -singularOffset, // the difference between the linear value and the true value at lowerBound
        0, // do not change other in range [otherLowerBound .. otherUpperBound]
        0, // has been coped with in 2)
      };
      auto pw = makePiecewiseFunction(aff.getDomainLocalSpace(), domainType, divPos, pwLowerBounds, pwUpperBounds, pwValues);
      aff += pw;
      other += pw;
      return true;
    }

    auto upperBoundm1 = upperBound - 1;
    if (isDivLinearOnRange(div, divType, divPos, lowerBound, upperBoundm1, constant, slope)) {
      // 1) make <aff> and <other> equal
      aff += divCoeff*floor(div);
      auto singularOffset = divCoeff*fdiv_q(divOffset + divSlope*upperBound, divDenom);

      // 2) subtract the linear part that we added from aff again
      // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
      aff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);
      singularOffset -= slope*divCoeff*upperBound;

      // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
      aff.addConstant_inplace(constant - divCoeff*slope*lowerBound);
      singularOffset += constant - divCoeff*slope*upperBound;

      // 4) Finally ensure that the single value of lowerBound does not change
      // makePiecewiseFunction returns divs itself, so we cannot rely on any later phase to correct it again, i.e. the same divs must be added to <aff> and <other>
      Int pwLowerBounds[] = { lowerBound, upperBound, otherLowerBound };
      Int pwUpperBounds[] = { upperBoundm1, upperBound, otherUpperBound };
      Int pwValues[] = {
        0,
        -singularOffset,
        0,
      };
      auto pw = makePiecewiseFunction(aff.getDomainLocalSpace(), domainType, divPos, pwLowerBounds, pwUpperBounds, pwValues);
      aff += pw;
      other += pw;
      return true;
    }
    return false;
  }


  // cannot yet handle nested ranges
  if (lowerBound < otherLowerBound && otherUpperBound < upperBound)
    return false;
  if (otherLowerBound < lowerBound && upperBound < otherUpperBound)
    return false;

  if (lowerBound == otherLowerBound && upperBound == otherLowerBound) {
  }

  if (lowerBound < otherLowerBound && otherLowerBound <= upperBound) {
    // [lowerBound .. otherLowerBound-1] [otherLowerBound .. upperBound] [upperBound+1 .. otherUpperBound]
    auto overlapLower = otherLowerBound;
    auto overlapUpper = upperBound;
    auto affCopy = aff;
    auto otherCopy = other;
    addDivIfPossibleWithBounds(lowerBound, div, divType, divPos, otherLowerBound - 1, constant, slope, aff, divCoeff, divOffset, divSlope, divDenom, otherLowerBound, otherUpperBound, domainType, other);

  }
  if (upperBound > otherUpperBound && otherUpperBound >= lowerBound) {
    // [otherLowerBound .. lowerBound-1] [lowerBound .. otherUpperBound] [otherUpperBound+1 .. upperBound]
    auto overlapLower = lowerBound;
    auto overlapUpper = otherUpperBound;
  }

  llvm_unreachable("forgotten case");
  return false;
}


static bool addDivIfPossible(const Aff &div, isl_dim_type divType, pos_t divPos, const Int &lowerBound, const Int &upperBound, Aff &aff, const Int &divCoeff) {
  Int constant;
  Int slope;
  if (isDivLinearOnRange(div, divType, divPos, lowerBound, upperBound, constant, slope)) {
    // 1) make <aff> and <other> equal 
    aff += divCoeff*floor(div);

    // 2) subtract the linear part that we added from aff again
    // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
    aff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);

    // 3) Correct the offset between divCoeff*floor(div) and slope*divCoeff*x we added implicitely
    aff.addConstant_inplace(constant - divCoeff*slope*lowerBound);

    return true;
  }

  return false;
}


static bool checkExisitingDivs(Aff other, Aff addToBoth) {
  other.alignDivs_inplace(other);
  addToBoth.alignDivs_inplace(other);
  auto nDivDims = addToBoth.getDivDimCount();
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    auto bothCoeff = addToBoth.getCoefficient(isl_dim_div, i);
    if (bothCoeff.isZero())
      continue;

    auto otherCoeff = other.getCoefficient(isl_dim_div, i);
    if (bothCoeff == otherCoeff)
      continue;
    return false;
  }
  return true;
}


/// Add the div without actually modifying the values returned by aff
static bool addDivIfPossible(Aff &aff, Aff &other, const Int &divCoeff, const Aff &div, const AllBounds &bounds, const AllBounds &otherBounds,bool tryCoeffs, bool tryDivs) {
  isl_dim_type divType;
  pos_t divPos;
  Int divSlope;
  Int divOffset;
  Int divDenom;
  if (!analyzeDiv(div, divType, divPos, divSlope, divOffset, divDenom))
    return false; // Not a simple div
  auto domainType = divType == isl_dim_in ? isl_dim_set : divType;
  if (!bounds.isBounded(domainType, divPos))
    return false;
  const auto &lowerBound = bounds.getLowerBound(domainType, divPos);
  const auto &upperBound = bounds.getUpperBound(domainType, divPos);

  //if (addDivIfPossible(div, divType, divPos, lowerBound, upperBound, aff, divCoeff))
  //  return true;


  // TODO: This should be working recursive, trying to unify the subranges independently


  if (overlappingBounds(bounds, otherBounds, domainType, divPos)) {
    // Can only take existing divs and add them to aff
    //auto nOtherDivs = other.getDivDimCount();
    //for (auto i = nOtherDivs - nOtherDivs; i < nOtherDivs; i += 1) {
    //  auto otherDiv = 
    //}
    return false;
  }

  if (!otherBounds.isBounded(domainType, divPos))
    return false;
  const auto &otherLowerBound = otherBounds.getLowerBound(domainType, divPos);
  const auto &otherUpperBound = otherBounds.getUpperBound(domainType, divPos);

  // disjoint
  Aff addToAff;
  Aff addToBoth;
  if (!unifyDivsDisjountDomains(divCoeff, div, divType, divPos, lowerBound, upperBound, otherLowerBound, otherUpperBound, addToAff, addToBoth, tryCoeffs, tryDivs))
    return false;
  aff += addToAff;
  aff += addToBoth;
  other += addToBoth;
  //assert(aff==other);
  return true;
}


static bool addDivFromOther(Aff &addToAff, const Aff &div, isl_dim_type divType, pos_t divPos, const Int &divSlope, const Int &divOffset, const Int &divDenom, const Int &lowerBound, const Int &upperBound, count_t j, const LocalSpace &commonls, SmallVectorImpl<Int>  &coeffDiffs, bool negateCoeffDiff) {
  Int divCoeff = coeffDiffs[j];
  if (negateCoeffDiff)
    divCoeff.neg_inplace();

  Int slope;
  if (isDivLinearOnRange(divSlope, divOffset, divDenom, lowerBound, upperBound, slope)) {
    // just add it, no additional divs required, but need to "undo" the linear part
    addToAff += divCoeff*floor(div);

    // 2) subtract the linear part that we added from aff again
    // this can make <aff> and <other> differ again, but will be coped with in tryConstantCombineTuple()
    addToAff.addCoefficient_inplace(divType, divPos, -slope*divCoeff);

    // 3) Correct the offsets of divCoeff*floor(div) and slope*divCoeff*x we added implicitely
    auto constant = fdiv_q(divOffset + divSlope*lowerBound, divDenom);
    addToAff -= divCoeff*constant;
    addToAff += divCoeff*slope*lowerBound;
    coeffDiffs[j] = 0;
    return true;
  }

  Int step;
  if (getDivOnlyStep(divSlope, divOffset, divDenom, lowerBound, upperBound, step)) {
    auto lowerValue = fdiv_q(divOffset + lowerBound*divSlope, divDenom);
    auto upperValue = fdiv_q(divOffset + upperBound*divSlope, divDenom);
    auto diffValue = upperValue - lowerValue;

    // Try to find a matching div
    auto nCommonDivs = coeffDiffs.size();
    for (auto i = j + 1; i < nCommonDivs; i += 1) {
      auto otherDiffCoeff = coeffDiffs[i];
      if (otherDiffCoeff.isZero())
        continue;
      if (negateCoeffDiff)
        otherDiffCoeff.neg_inplace();
      auto otherDiv = commonls.getDiv(i);
      Int otherDenom;
      Int otherSlope;
      Int otherOffset;
      isl_dim_type otherType;
      pos_t otherPos;
      if (!analyzeDiv(otherDiv, otherType, otherPos, otherSlope, otherOffset, otherDenom))
        continue;
      if (divType != otherType)
        continue;
      if (divPos != otherPos)
        continue;

      Int otherStep;
      if (!getDivOnlyStep(otherSlope, otherOffset, otherDenom, lowerBound, upperBound, otherStep))
        continue;
      if (otherStep != step)
        continue; // No match

      auto otherLowerValue = fdiv_q(otherOffset + lowerBound*otherSlope, otherDenom);
      auto otherUpperValue = fdiv_q(otherOffset + upperBound*otherSlope, otherDenom);
      auto otherDiffValue = otherUpperValue - otherLowerValue;

      if (!isDivisibleBy(diffValue, otherDiffValue))
        continue;

      auto factor = divexact(diffValue, otherDiffValue);
      addToAff += divCoeff*floor(div);
      addToAff -= factor*divCoeff*floor(otherDiv);
      addToAff += otherLowerValue - lowerValue;
      coeffDiffs[i] -= factor*divCoeff;
      coeffDiffs[j] = 0;
      return true;
    }
  }

  return divCoeff.isZero();
}



/// Try to unify two affine expression, s.t. their result on their domain sets do not change
/// If tryDivs is true, it may add additional div dimensions
/// Return true if succeeds; mset and maff contain the combined expression
/// Returns false if failed; mset and ,aff are undefied in this case
template<typename BasicSet>
static bool tryConstantCombine(Set &mset, Aff &maff, const BasicSet &bset, Aff baff, bool tryCoeffs, bool tryDivs) {
  if (maff.keep() == baff.keep()) {
    // These are literally the same!
    mset.unite_inplace(bset);
    return true;
  }
  //Int any;

  auto bBounds = bset.getAllBounds();
  auto mBounds = mset.getAllBounds();

  auto mls = maff.getDomainLocalSpace();
  auto bls = baff.getDomainLocalSpace();
  auto commonls = intersect(mls, bls);
  auto maffcommon = maff.alignDivs(commonls);
  auto baffcommon = baff.alignDivs(commonls);
  assert(maffcommon.getDomainLocalSpace() == baffcommon.getDomainLocalSpace());
  if (isEqual(maffcommon, baffcommon, Accuracy::Plain).isTrue()) { // Let is find out using plain algorithm that all the coefficients are already equal; much faster than the stuff below
    mset.unite_inplace(bset);
    return true;
  }

  if (maff.getDenominator() != baff.getDenominator())
    return false; // Don't handle rational cases

  auto nCommonDivs = commonls.getDivDimCount();
  //SmallVector<Int, 4> bDivCoeffs;
  //SmallVector<Int, 4> mDivCoeffs;
  SmallVector<Int, 4> coeffDiffs;
  //bDivCoeffs.reserve(nCommonDivs);
  //mDivCoeffs.reserve(nCommonDivs);
  coeffDiffs.reserve(nCommonDivs);
  for (auto j = nCommonDivs - nCommonDivs; j < nCommonDivs; j += 1) {
    auto bCoeff = baffcommon.getCoefficient(isl_dim_div, j);
    auto mCoeff = maffcommon.getCoefficient(isl_dim_div, j);
    coeffDiffs.push_back(bCoeff - mCoeff);
    //bDivCoeffs.push_back(std::move(bCoeff));
    //mDivCoeffs.push_back(std::move(mCoeff));
  }

  // Look for div already existing in the other part
  for (auto j = nCommonDivs - nCommonDivs; j < nCommonDivs; j += 1) {
    auto &divCoeff = coeffDiffs[j];
    //const auto &bCoeff = bDivCoeffs[j];
    //const auto &mCoeff = mDivCoeffs[j];
    if (divCoeff.isZero())
      continue; // Already equal
    auto div = commonls.getDiv(j);
    isl_dim_type divType;
    pos_t divPos;
    Int divSlope;
    Int divOffset;
    Int divDenom;
    if (!analyzeDiv(div, divType, divPos, divSlope, divOffset, divDenom))
      return false;
    auto domainType = divType == isl_dim_in ? isl_dim_set : divType;


    if (bBounds.isBounded(domainType, divPos)) {
      const auto &bLowerBound = bBounds.getLowerBound(domainType, divPos);
      const auto &bUpperBound = bBounds.getUpperBound(domainType, divPos);
      if (addDivFromOther(baff, div, divType, divPos, divSlope, divOffset, divDenom, bLowerBound, bUpperBound, j, commonls, coeffDiffs, true))
        continue;
    }


    if (mBounds.isBounded(domainType, divPos)) {
      const auto &mLowerBound = mBounds.getLowerBound(domainType, divPos);
      const auto &mUpperBound = mBounds.getUpperBound(domainType, divPos);
      if (addDivFromOther(maff, div, divType, divPos, divSlope, divOffset, divDenom, mLowerBound, mUpperBound, j, commonls, coeffDiffs, false))
        continue;
    }
  }


 
  for (auto j = nCommonDivs - nCommonDivs; j < nCommonDivs; j += 1) {
    auto div = commonls.getDiv(j);
    assert(div == maffcommon.getDiv(j));
    assert(div == baffcommon.getDiv(j));
    auto &diffCoeff = coeffDiffs[j];
    if (diffCoeff.isZero())
      continue; // No action required

    if (!tryDivs)
      return false;

    do {
      if (addDivIfPossible(maff, baff, diffCoeff, div, mBounds, bBounds, tryCoeffs, tryCoeffs))
        break;

      if (addDivIfPossible(baff, maff, -diffCoeff, div, bBounds, mBounds, tryCoeffs, tryCoeffs))
        break;

      return false;
    } while (0);
    diffCoeff = 0;
  }

  if (!tryConstantCombineTuple(mBounds, maff, bBounds, baff, isl_dim_param, tryDivs))
    return false;

  if (!tryConstantCombineTuple(mBounds, maff, bBounds, baff, isl_dim_in, tryDivs))
    return false;

  auto mcst = maff.getConstant();
  auto bcst = baff.getConstant();

  do {
    if (mcst == bcst)
      break;

    if (tryCoeffs) {
    if (tryCombineConstant(mset, mBounds, maff, bset, bBounds, baff, isl_dim_in))
      break;
    if (tryCombineConstant(mset, mBounds, maff, bset, bBounds, baff, isl_dim_param))
      break;
    }

    if (tryDivs) {
      if (tryCombineConstantUsingDiv(mset, mBounds, maff, bset, bBounds, baff, isl_dim_in))
        break;
      if (tryCombineConstantUsingDiv(mset, mBounds, maff, bset, bBounds, baff, isl_dim_param))
        break;
    }

    return false; // Failed to make constant of both affs equal
  } while (0);
  //assert(maff==baff); // Just plain equality check algorithm, might fail
  mset.unite_inplace(bset);
  return true;
}


bool isl::tryCombineAff(Set lhsContext, Aff lhsAff, const Set &rhsContext, const Aff &rhsAff, bool tryCoeffs, bool tryDivs, Set &resultContext, Aff &resultAff) {
#ifndef NDEBUG
  static bool donetest = false;
  if (!donetest) {
  Int slope1;
  auto test1 = isDivLinearOnRange(-1, -1, 3, 0, 2, slope1);
  assert(test1 && (slope1 == 0));

  Int slope2;
  auto test2 = isDivLinearOnRange(5, 3, 3,   1, 2,   slope2);
  assert(test2 && (slope2 == 2));

  donetest = true;
}
#endif

  
  bool success = tryConstantCombine(lhsContext, lhsAff, rhsContext, rhsAff, tryCoeffs, tryDivs);
  if (&resultContext) { // Compiler may assume that references are never NULL
    resultContext = lhsContext;
  }
  resultAff = lhsAff;
  return success;
}


ISLPP_INPLACE_ATTRS void Aff::normalizeDivs_inplace() ISLPP_INPLACE_FUNCTION{
  auto nDivs = getDivDimCount();
  if (nDivs == 0)
    return;

#ifndef NDEBUG
  auto orig = copy();
#endif

  auto collectedDivs = getDomainSpace().createZeroAff();
  for (auto i = nDivs - nDivs; i < nDivs; i += 1) {
    auto coeff = getCoefficient(isl_dim_div, i);
    if (coeff.isZero())
      continue;

    auto div = getDiv(i);
    setCoefficient_inplace(isl_dim_div, i, 0);
    collectedDivs += coeff*isl::floor(div); //TODO: What about divs within?
  }
  *this += collectedDivs;
  assert(*this==orig);
}


ISLPP_PROJECTION_ATTRS bool Aff::isLinearOnRange(Dim dim, const Int *lowerBound, const Int *upperBound, /*out*/Int &slope)ISLPP_PROJECTION_FUNCTION{
  auto type = dim.getType();
  auto pos = dim.getPos();
  slope = getCoefficient(dim);

  auto nDivDims = getDivDimCount();
  auto nParamDims = getParamDimCount();
  auto nInDims = getDivDimCount();
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    auto div = getDiv(i);

    int nOthers = 0;
    for (auto j = nParamDims - nParamDims; j < nParamDims; j += 1){
      if (type == isl_dim_param && pos == pos)
        continue;
      auto coeff = div.getCoefficient(isl_dim_param, j);
      if (coeff.isZero())
        continue;
      nOthers += 1;
    }
    for (auto j = nInDims - nInDims; j < nInDims; j += 1){
      if (type == isl_dim_in && pos == pos)
        continue;
      auto coeff = div.getCoefficient(isl_dim_in, j);
      if (coeff.isZero())
        continue;
      nOthers += 1;
    }
    for (auto j = nDivDims - nDivDims; j < nDivDims; j += 1){
      // Divs of divs is strange
      auto coeff = div.getCoefficient(isl_dim_in, j);
      if (coeff.isZero())
        continue;
      nOthers += 1;
    }
    auto divSlope = div.getCoefficient(dim);
    if (divSlope.isZero())
      continue;
    if (nOthers != 0)
      return false;
    if (!lowerBound || !upperBound)
      return false;
    auto divOffset = div.getConstant();
    auto divDenom = div.getDenominator();
    if (divDenom.isNeg()) {
      divDenom.neg_inplace();
      divOffset.neg_inplace();
      divSlope.neg_inplace();
    }

    Int effectiveSlope;
    if (!isDivLinearOnRange(divSlope, divOffset, divDenom, *lowerBound, *upperBound, effectiveSlope))
      return false;
    slope += effectiveSlope;
  }

  return true;
}


bool isl::isEqual(const Aff &lhs, const Aff &rhs) {
  if (checkBool(isl_aff_plain_is_equal(lhs.keep(), rhs.keep())))
    return true;
  return isEqual(lhs.toMap(), rhs.toMap());
}


Tribool isl::plainIsEqual(const Aff &lhs, const Aff &rhs) {
  auto retval = checkBool(isl_aff_plain_is_equal(lhs.keep(), rhs.keep()));
  return retval ? Tribool::True : Tribool::Indeterminate;
}


Tribool isl::isEqual(const Aff &lhs, const Aff &rhs, Accuracy accuracy) {
  switch (accuracy) {
  case isl::Accuracy::None:
    return lhs.keep() == rhs.keep() ? Tribool::True : Tribool::Indeterminate;
  case isl::Accuracy::Plain:
    return plainIsEqual(lhs,rhs);
  case isl::Accuracy::Fast:
  case isl::Accuracy::Exact:
  default:
    return isEqual(lhs, rhs);
  }
}
