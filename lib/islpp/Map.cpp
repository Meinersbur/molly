#include "islpp/Map.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/UnionMap.h"
#include "islpp/MultiPwAff.h"
#include "islpp/DimRange.h"
#include "islpp/Point.h"
#include "islpp/Mat.h"

#include <isl/map.h>
#include <llvm/Support/raw_ostream.h>


using namespace isl;
using namespace llvm;
using namespace std;






UnionMap Map::toUnionMap() const {
  return UnionMap::enwrap(isl_union_map_from_map(takeCopy()));
}
#if ISLPP_HAS_RVALUE_REFERENCE_THIS
UnionMap Map::toUnionMap() && {
  return UnionMap::enwrap(isl_union_map_from_map(take()));
}
#endif


Map Map::readFrom(Ctx *ctx, const char *str) {
  return Map::enwrap(isl_map_read_from_str(ctx->keep(), str));
}


void Map::print(llvm::raw_ostream &out) const {
  // isl_map_print requires to print to FILE*
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void Map::dump() const {
  isl_map_dump(keep());
}


Map Map::createFromUnionMap(UnionMap &&umap) {
  return Map::enwrap(isl_map_from_union_map(umap.take()));
}


static int foreachBasicMapCallback(__isl_take isl_basic_map *bmap, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(BasicMap&&)>*>(user);
  auto retval = func(BasicMap::enwrap(bmap));
  return retval ? -1 : 0;
}
bool Map::foreachBasicMap(std::function<bool(BasicMap&&)> func) const {
  auto retval = isl_map_foreach_basic_map(keep(), foreachBasicMapCallback, &func);
  return retval != 0;
}


static int enumBasicMapCallback(__isl_take isl_basic_map *map, void *user) {
  auto list = static_cast<std::vector<BasicMap> *>(user);
  list->push_back(BasicMap::enwrap(map));
  return 0;
}
std::vector<BasicMap> Map::getBasicMaps() const {
  std::vector<BasicMap> result;
  //result.reserve(isl_map_n_basic_maps(keep()));
  auto retval = isl_map_foreach_basic_map(keep(), enumBasicMapCallback, &result);
  assert(retval == 0);
  return result;
}


Map BasicMap::toMap() const {
  return Map::enwrap(isl_map_from_basic_map(takeCopy()));
}

#if 0
Map Map::chainNested(const Map &map) const {
  return this->wrap().chainNested(map);
}
#endif

Map Map::chainNested(const Map &map, unsigned tuplePos) const {
  return this->wrap().chainNested(map, tuplePos);
}


Map Map::chainNested(isl_dim_type type, const Map &map) const {
  // assume type==isl_dim_in
  // this = { (A, B, C) -> D }
  // map = { B -> E }
  // return { ((A, B, C) -> E) -> D }

  auto space = getSpace(); // { (A, B, C) -> D }
  auto seekNested = map.getDomainSpace(); // { B }

  auto dims = space.findSubspace(type, seekNested);
  assert(dims.isValid());

  auto tupleSpace = getTupleSpace(type);
  auto expandDomainSpace = Space::createMapFromDomainAndRange(map.getDomainSpace(), tupleSpace); // { B -> (A, B', C) }
  auto expandDomain = expandDomainSpace.equalBasicMap(isl_dim_in, 0, dims.getCount(), isl_dim_out, dims.getBeginPos()); // { B -> (A, B', C) | B=B' }

  auto resultSpace = Space::createMapFromDomainAndRange(tupleSpace, map.getRangeSpace()).wrap(); // { ((A, B, C) -> E) }
  auto expandRangeSpace = Space::createMapFromDomainAndRange(map.getRangeSpace(), resultSpace); // { E -> ((A, B, C) -> E') }
  auto expandRange = expandRangeSpace.equalBasicMap(isl_dim_in, 0, map.getOutDimCount(), isl_dim_out, tupleSpace.getSetDimCount()); // { E -> ((A, B, C) -> E') | E=E' }

  auto expandedMap = map.applyDomain(expandDomain).applyRange(expandRange); // { (A, B, C) -> ((A, B, C) -> E)  }
  expandedMap.intersect_inplace(expandedMap.getSpace().equalBasicMap(tupleSpace.getSetDimCount()));
  if (type == isl_dim_in)
    return applyDomain(expandedMap);
  else if (type == isl_dim_out)
    return applyRange(expandedMap);
  else
    llvm_unreachable("invalid dim type");
}


Map Map::applyNested(isl_dim_type type, const Map &map) const {
  auto space = getSpace();
  auto typeSpace = space.extractTuple(type);
  auto seekNested = map.getDomainSpace();

  auto dims = space.findSubspace(type, seekNested);
  assert(dims.isValid());

  auto nPrefixDims = dims.getBeginPos();
  auto nDomainDims = map.getInDimCount();
  auto nRangeDims = map.getOutDimCount();
  auto nPosfixDims = typeSpace.getSetDimCount() - dims.getEndPos();
  assert(nPrefixDims + nDomainDims + nPosfixDims == typeSpace.getSetDimCount());

  //auto tuple = space.extractTuple(type);
  auto applySpace = Space::createMapFromDomainAndRange(typeSpace, typeSpace.replaceSubspace(seekNested, map.getRangeSpace()));
  assert(applySpace.getInDimCount() == nPrefixDims + nDomainDims + nPosfixDims);
  assert(applySpace.getOutDimCount() == nPrefixDims + nRangeDims + nPosfixDims);

  auto apply = map.insertDims(isl_dim_in, 0, nPrefixDims);
  apply.insertDims_inplace(isl_dim_out, 0, nPrefixDims);
  apply.addDims_inplace(isl_dim_in, nPosfixDims);
  apply.addDims_inplace(isl_dim_out, nPosfixDims);

  apply.intersect_inplace(apply.getSpace().equalBasicMap(isl_dim_in, 0, nPrefixDims, isl_dim_out, 0));
  apply.intersect_inplace(apply.getSpace().equalBasicMap(isl_dim_in, nPrefixDims + nDomainDims, nPosfixDims, isl_dim_out, nPrefixDims + nRangeDims));
  apply.cast_inplace(applySpace);

  if (type == isl_dim_in)
    return applyDomain(apply);
  else
    return applyRange(apply);
}


void Map::projectOutSubspace_inplace(isl_dim_type type, const Space &subspace) ISLPP_INPLACE_FUNCTION{
  auto myspace = getTupleSpace(type);
  auto dims = getSpace().findSubspace(type, subspace);
  assert(dims.isValid());
  assert(dims.getType() == type);

  auto shrinkSpace = myspace.removeSubspace(subspace);
  this->projectOut_inplace(type, dims.getBeginPos(), dims.getCount());

  if (isl_dim_in == type)
    this->cast_inplace(Space::createMapFromDomainAndRange(shrinkSpace, getRangeSpace()));
  else
    this->cast_inplace(Space::createMapFromDomainAndRange(getDomainSpace(), shrinkSpace));
}


Map Map::projectOutSubspace(isl_dim_type type, const Space &subspace) const {
  auto result = copy();
  result.projectOutSubspace_inplace(type, subspace);
  return result;
}


void Map::moveSubspaceAppendToRange_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION{
  auto myspace = getSpace();
  auto dimrange = myspace.findSubspace(isl_dim_in, subspace);
  assert(dimrange.isValid());

  auto newDomainSpace = getDomainSpace().removeSubspace(subspace);
  auto newRangeSpace = Space::createMapFromDomainAndRange(getRangeSpace(), subspace).wrap();
  auto newSpace = Space::createMapFromDomainAndRange(newDomainSpace, newRangeSpace);

  moveDims_inplace(isl_dim_out, getOutDimCount(), isl_dim_in, dimrange.getBeginPos(), dimrange.getCount());
  cast_inplace(newSpace);
}


void Map::moveSubspacePrependToRange_inplace(const Space &subspace) ISLPP_INPLACE_FUNCTION{
  auto myspace = getSpace();
  auto dimrange = myspace.findSubspace(isl_dim_in, subspace);
  assert(dimrange.isValid());

  auto newDomainSpace = getDomainSpace().removeSubspace(subspace);
  auto newRangeSpace = Space::createMapFromDomainAndRange(subspace, getRangeSpace()).wrap();
  auto newSpace = Space::createMapFromDomainAndRange(newDomainSpace, newRangeSpace);

  moveDims_inplace(isl_dim_out, 0, isl_dim_in, dimrange.getBeginPos(), dimrange.getCount());
  cast_inplace(newSpace);
}




Map isl::join(const Set &domain, const Set &range, unsigned firstDomainDim, unsigned firstRangeDim, unsigned countEquate) {
  auto cartesian = product(domain, range).unwrap();
  cartesian.intersect(cartesian.getSpace().equalBasicMap(isl_dim_in, firstDomainDim, countEquate, isl_dim_out, firstRangeDim));
  return cartesian;
}


Map isl::naturalJoin(const Set &domain, const Set &range) {
  auto cartesian = product(domain, range).unwrap();

  auto domainSpace = domain.getSpace();
  auto rangeSpace = range.getSpace();
  auto productSpace = cartesian.getSpace();

  auto domainSpaces = domainSpace.flattenNestedSpaces();
  unsigned domainPos = 0;
  for (auto &space : domainSpaces) {
    unsigned first, count;
    if (!rangeSpace.findTuple(isl_dim_set, space.getTupleId(isl_dim_set), first, count))
      continue;
    assert(count = space.getSetDimCount());
    cartesian.intersect(productSpace.equalBasicMap(isl_dim_in, domainPos, count, isl_dim_out, first));
    domainPos += space.getSetDimCount();
  }

  auto nDims = domainSpace.getSetDimCount();
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    if (!domainSpace.hasDimId(isl_dim_set, i))
      continue;

    auto id = domain.getSetTupleId();
    auto pos = rangeSpace.findDimById(isl_dim_set, id); // This assumes that thare is at most one match of this id in rangeSpace
    if (pos < 0)
      continue;

    cartesian.equate_inplace(isl_dim_in, i, isl_dim_out, pos);
  }

  return cartesian;
}


void isl::Map::dumpExplicit(int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= true*/, bool sorted) const {
  printExplicit(llvm::dbgs(), maxElts, newlines, formatted, sorted);
  llvm::dbgs() << "\n";
}


void isl::Map::dumpExplicit()const{
  dumpExplicit(8, false);
}


void isl::Map::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= true*/, bool sorted /*= false*/) const{
  auto wrapped = wrap();
  auto nParamDims = getParamDimCount();
  auto nInDims = getInDimCount();
  auto nOutDims = getOutDimCount();
  int count = 0;
  bool omittedsome;

  auto lambda = [&count, maxElts, &os, nInDims, nOutDims, newlines, formatted](Point p) -> bool {
    if (count >= maxElts)
      return true;

    if (count == 0) {
      os << (newlines ? "{\n  " : "{ ");
    } else {
      os << (newlines ? ",\n  " : ", ");
    }

    if (formatted) {
      p.printFormatted(os);
    } else {
      if (nInDims != 1)
        os << '[';
      for (auto i = nInDims - nInDims; i<nInDims; i += 1) {
        if (i>0)
          os << ',';
        os << p.getCoordinate(isl_dim_set, i);
      }
      if (nInDims != 1)
        os << ']';

      os << " -> ";

      if (nOutDims != 1)
        os << '[';
      for (auto i = nOutDims - nOutDims; i<nOutDims; i += 1) {
        if (i>0)
          os << ',';
        os << p.getCoordinate(isl_dim_set, nInDims + i);
      }
      if (nOutDims != 1)
        os << ']';
    }

    count += 1;
    return false;
  };

  if (sorted) {
    std::vector<Point> points;
    points.reserve(maxElts);
    omittedsome = wrapped.foreachPoint([&points, maxElts](Point p) -> bool {
      if (points.size() >= maxElts)
        return true;
      points.push_back(p);
      return false;
    });
    std::sort(points.begin(), points.end(), [nParamDims, nInDims, nOutDims](const Point &p1, const Point &p2) -> bool {
      for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
        auto c1 = p1.getCoordinate(isl_dim_param, i);
        auto c2 = p2.getCoordinate(isl_dim_param, i);
        auto cmp = isl_int_cmp(c1.keep(), c2.keep());
        if (!cmp)
          continue;
        return cmp < 0;
      }
      for (auto i = 0; i < nInDims + nOutDims; i += 1) {
        auto c1 = p1.getCoordinate(isl_dim_set, i);
        auto c2 = p2.getCoordinate(isl_dim_set, i);
        auto cmp = isl_int_cmp(c1.keep(), c2.keep());
        if (!cmp)
          continue;
        return cmp < 0;
      }
      return 0;
    });
    for (const auto &p : points) {
      if (lambda(p))
        break;
    }
  } else {
    omittedsome = wrapped.foreachPoint(lambda);
  }

  if (count == 0) {
    if (omittedsome) {
      os << "{ ... }";
    } else {
      os << "{ }";
    }
  } else {
    if (omittedsome) {
      os << (newlines ? ",\n... }" : ", ... }");
    } else {
      os << (newlines ? ",\n}" : " }");
    }
  }
}


std::string isl::Map::toStringExplicit(int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= true*/, bool sorted) {
  std::string str;
  llvm::raw_string_ostream os(str);
  printExplicit(os, maxElts, newlines, formatted, sorted);
  os.flush();
  return str; // NRVO
}


#ifndef NDEBUG
std::string isl::Map::toString() const {
  return ObjBaseTy::toString();
}
#endif /* NDEBUG */


ISLPP_EXSITU_ATTRS Map isl::Map::intersectDomain(UnionSet uset) ISLPP_EXSITU_FUNCTION{
  return intersectDomain(uset.extractSet(getDomainSpace()));
}


ISLPP_EXSITU_ATTRS Map isl::Map::intersectRange(UnionSet uset) ISLPP_EXSITU_FUNCTION{
  auto set = uset.extractSet(getRangeSpace());
  return Map::enwrap(isl_map_intersect_range(takeCopy(), set.take()));
}

isl::Set isl::Map::map(Set set) const {
  return Set::enwrap(isl_set_apply(set.take(), takeCopy()));
}

isl::Set isl::Map::map(Vec vec) const {
  return map(vec.toBasicSet());
}


static bool isRhsBetter(int nUncovered, const MultiAff &lhs, int lhsComplexity, int lhsRemainingBSets, int lhsCoverBSets, const MultiAff &rhs, int rhsComplexity, int rhsRemainingBSets, int rhsCoverBSets) {
  if (lhs.keep() == rhs.keep())
    return false;

  auto lhsReduction = nUncovered - lhsRemainingBSets;
  auto rhsReduction = nUncovered - rhsRemainingBSets;

  if (lhsReduction > 0 && rhsReduction > 0) {
    // Both reduce at least one
    // Reducing number of bsets becomes less important, but take care to limit complexity
    auto lhsPiecesPerCover = (lhsCoverBSets + lhsReduction - 1) / lhsReduction;
    auto rhsPiecesPerCover = (rhsCoverBSets + rhsReduction - 1) / rhsReduction;

    if (rhsPiecesPerCover < lhsPiecesPerCover)
      return true;
    if (rhsPiecesPerCover > lhsPiecesPerCover)
      return false;
  } else {
    if (rhsRemainingBSets < lhsRemainingBSets)
      return true;
    if (rhsRemainingBSets > lhsRemainingBSets)
      return false;
  }

  if (rhsCoverBSets < lhsCoverBSets)
    return true;
  if (rhsCoverBSets > lhsCoverBSets)
    return false;

  if (rhsComplexity < lhsComplexity)
    return true;
  if (rhsComplexity >lhsComplexity)
    return false;

  // Test might have been skipped above
  if (rhsRemainingBSets < lhsRemainingBSets)
    return true;
  if (rhsRemainingBSets > lhsRemainingBSets)
    return false;

  // Not measurable difference between the two
  return false;
}




// exponential in number of (in)equalities (or dimensions)! May mark condistions as used to make each condition appear in just one
//TODO: Instead of such a long parameter list, make a record that contains the params that do not change and pass a pointer to it
static void inferFromConditions_backtracking(std::vector<MultiAff> &affs, const LocalSpace &ls, Mat &eqs, Mat &ineqs, std::vector<Aff> &stack) {
  auto nParamDims = ls.getParamDimCount();
  auto nInputDims = ls.getInDimCount();
  auto nOutputDims = ls.getOutDimCount();
  auto nDivDims = ls.getDivDimCount();
  auto lookingFor = stack.size();
  auto lookingForDim = 1 + nParamDims + nInputDims + lookingFor;

  if (lookingFor >= nOutputDims) {
    // Stack is complete
    auto sol = ls.getSpace().createZeroMultiAff();
    for (auto k = nOutputDims - nOutputDims; k < nOutputDims; k += 1) {
      auto solaff = stack[k];
      assert(solaff.isValid());
      sol.setAff_inplace(k, std::move(solaff));
    }
    affs.push_back(std::move(sol));
    return;
  }

  auto nEqs = eqs.rows();
  auto nIneqs = ineqs.rows();
  assert(eqs.cols() == ineqs.cols());
  auto nDims = eqs.cols();
  assert(nDims == 1 + nParamDims + nInputDims + nOutputDims + nDivDims);
  bool someUnderdefined = false; bool someDefined = false;

  auto affls = ls.domain();
  //affls.removeDims_inplace(isl_dim_out, 0, lookingFor);
  //affls.removeDims_inplace(isl_dim_out, 1, nOutputDims - lookingFor - 1);

  for (int useIneqs = false; useIneqs < 2; useIneqs += 1) {
    Mat &mat = useIneqs ? ineqs : eqs;
    auto rows = mat.rows();
    for (auto i = rows - rows; i < rows; i += 1) {

      Int lookingCoeff = mat[i][lookingForDim];
      if (lookingCoeff.isZero())
        continue; // Quick skip; does not influence the dimension looked for

      bool skip = false;
      for (auto j = lookingFor + 1; j < nOutputDims; j += 1) {
        auto d = 1 + nParamDims + nInputDims + j;
        Int coeff = mat[i][d];
        if (coeff.isZero())
          continue;
        // Condition is underdefined
        someUnderdefined = true;
        skip = true;
        break;
      }
      if (skip)
        continue;

      // Construct aff
      // eq: c + a1*x1 + a2*x2 + ... + au*xu = 0
      // => xu = (-c - a1*x1 - a2*x2 ...)/au

      Int cst = mat[i][0];
      auto aff = affls.createConstantAff(cst);
      for (auto j = nParamDims - nParamDims; j < nParamDims; j += 1) {
        auto d = 1 + j;
        Int coeff = mat[i][d];
        aff.addCoefficient_inplace(isl_dim_param, j, -coeff);
      }
      for (auto j = nInputDims - nInputDims; j < nInputDims; j += 1) {
        auto d = 1 + nParamDims + j;
        Int coeff = mat[i][d];
        aff.addCoefficient_inplace(isl_dim_in, j, coeff);
      }
      for (auto j = lookingFor - lookingFor; j < lookingFor; j += 1) {
        auto d = 1 + nParamDims + nInputDims + j;
        Int coeff = mat[i][d];
        if (coeff.isZero())
          continue;
        auto multipleof = stack[j];
        assert(multipleof.isValid());
        aff += coeff*multipleof;
      }
      for (auto j = nDivDims - nDivDims; j < nDivDims; j += 1) {
        auto d = 1 + nParamDims + nInputDims + nOutputDims + j;
        Int coeff = mat[i][d];
        if (coeff.isZero())
          continue;
        auto multipleof = floor(affls.getDiv(j));
        assert(multipleof.isValid());
        aff += coeff*multipleof;
      }
      if (lookingCoeff.isNeg()) {
        lookingCoeff.abs_inplace();
      } else {
        aff.neg_inplace();
      }
      if (!lookingCoeff.isOne()) {
        aff.setDenominator_inplace(lookingCoeff);
        aff.floor_inplace();
      }

      someDefined = true;
      stack.push_back(aff);
      inferFromConditions_backtracking(affs, ls, eqs, ineqs, stack);
      stack.pop_back();
    }
  }

  if (someUnderdefined && !someDefined) {
    // Some condition could not be applied because at least two variables it uses do not have a value
    // We arbitrarily define one of them and continue

    auto aff = affls.createConstantAff(0);
    stack.push_back(aff);
    inferFromConditions_backtracking(affs, ls, eqs, ineqs, stack);
    stack.pop_back();
  }
}


static void inferFromConditions2_backtracking(std::vector<MultiAff> &affs, const Space &solspace, const LocalSpace &ls, Mat &eqs, Mat &ineqs, int start, count_t nParamDims, count_t nInputDims, count_t nOutputDims, count_t nDivDims, isl::Aff sols[], int nUndefined) {
  if (nUndefined == 0) {
    // We have a solution!
    auto sol = solspace.createZeroMultiAff();
    for (auto k = nOutputDims - nOutputDims; k < nOutputDims; k += 1) {
      auto solaff = sols[k];
      assert(solaff.isValid());
      sol.setAff_inplace(k, std::move(solaff));
    }
    affs.push_back(sol);
    return; // No more rules to discover
  }

  auto nEqs = eqs.rows();
  auto nIneqs = ineqs.rows();
  assert(eqs.cols() == ineqs.cols());
  auto nDims = eqs.cols();
  assert(nDims == 1 + nParamDims + nInputDims + nOutputDims + nDivDims);
  int someUnderdefinedDim;
  int someUnderdefinedRule = -1;

  int i;
  int useIneqs;
  if (start >= nEqs) {
    i = start - nEqs;
    useIneqs = true;
  } else {
    i = 0;
    useIneqs = false;
  }

  for (; useIneqs < 2; useIneqs += 1) {
    Mat &mat = useIneqs ? ineqs : eqs;
    auto rows = mat.rows();

    for (; i < rows; i += 1, start += 1) {
      int nOpen = 0;
      pos_t undefined;
      for (auto j = nOutputDims - nOutputDims; j < nOutputDims; j += 1) {
        auto d = 1 + nParamDims + nInputDims + j;
        if (sols[j].isValid())
          continue;

        Int coeff = mat[i][d];
        if (coeff.isZero())
          continue;
        nOpen += 1;

        if (nOpen >= 2) {
          if (someUnderdefinedRule < 0) {
            someUnderdefinedRule = start;
            someUnderdefinedDim = undefined;
          }
          break; // underdefined
        }
        undefined = d;
      }

      if (nOpen == 1) {
        // eq: c + a1*x1 + a2*x2 + ... + au*xu = 0
        // => xu = (-c - a1*x1 - a2*x2 ...)/au

        Int cst = mat[i][0];
        auto aff = ls.createConstantAff(cst);
        for (auto j = nParamDims - nParamDims; j < nParamDims; j += 1) {
          auto d = 1 + j;
          Int coeff = mat[i][d];
          aff.addCoefficient_inplace(isl_dim_param, d, -coeff);
        }
        for (auto j = nInputDims - nInputDims; j < nInputDims; j += 1) {
          auto d = 1 + nParamDims + j;
          Int coeff = mat[i][d];
          aff.addCoefficient_inplace(isl_dim_in, d, coeff);
        }
        for (auto j = nOutputDims - nOutputDims; j < nOutputDims; j += 1) {
          auto d = 1 + nParamDims + nInputDims + j;
          if (d == undefined)
            continue; // Handled below
          Int coeff = mat[i][d];
          if (coeff.isZero())
            continue;
          auto multipleof = sols[j];
          assert(multipleof.isValid());
          aff += coeff*multipleof;
        }
        for (auto j = nDivDims - nDivDims; j < nDivDims; j += 1) {
          auto d = 1 + nParamDims + nInputDims + nOutputDims + j;
          Int coeff = mat[i][d];
          if (coeff.isZero())
            continue;
          auto multipleof = ls.getDiv(j);
          assert(multipleof.isValid());
          aff += coeff*multipleof;
        }
        Int denom = mat[i][undefined];
        auto j = undefined - (1 + nParamDims + nInputDims);
        assert(!denom.isZero());
        if (denom.isNeg()) {
          denom.abs_inplace();
        } else {
          aff.neg_inplace();
        }
        aff.setDenominator_inplace(denom);
        assert(sols[j].isNull());
        sols[j] = std::move(aff);

        inferFromConditions2_backtracking(affs, solspace, ls, eqs, ineqs, start + 1, nParamDims, nInputDims, nOutputDims, nDivDims, sols, nUndefined - 1);

        // Undo for next possibility
        sols[j].reset();
      }
    }
  }

  // Went through all conditions; if it is still underdefined, just try with some constant and redo from there
  if (someUnderdefinedRule >= 0) {
    auto j = someUnderdefinedDim - (1 + nParamDims + nInputDims);
    auto aff = ls.createConstantAff(0);
    sols[j] = std::move(aff);
    inferFromConditions2_backtracking(affs, solspace, ls, eqs, ineqs, someUnderdefinedRule, nParamDims, nInputDims, nOutputDims, nDivDims, sols, nUndefined - 1);
    // Undo so parent does need to
    sols[j].reset();
  }
}


/// returns a function that maps to only one element per domain vector; it is undefined which element it is
/// The methods lexminPwMultiAff(), lexmaxPwMultiAff() already do this, but their return value can be quite complex
ISLPP_EXSITU_ATTRS PwMultiAff Map::anyElement() ISLPP_EXSITU_FUNCTION{
  // Wasteful algorithm to find least complicated function that maps all domain vectors to some vector in the map
  // The runtime of everything using this function depends on the complexity, so it is worth spending some time here
  // Unfortunately, I lack ideas how to find such more efficiently
  // This method is very inefficient
  // Actually, I have some idea how to do better: For any BasicMap, iterate through the constraints and fullfil constraints successively by converting the constraint to an Aff and pullbacking it to a global solution, dropping conflicting ones

  auto nDims = getOutDimCount();
  auto space = getSpace();
  auto zeroaff = space.createZeroMultiAff();

  auto lexmin = lexminPwMultiAff();
  //lexmin.coalesce_inplace(); // Does it anything?
  auto lexmax = lexmaxPwMultiAff();
  //lexmax.coalesce_inplace(); // Does it anything?

  std::vector<MultiAff> inferedCandidates;
#if 1
  for (const auto &bmap : detectEqualities().getBasicMaps()) { //TODO: We may increase the chances to cover multiple BasicSets at once if we combine conditions from different BasicSets
    auto eqs = bmap.equalitiesMatrix();
    auto ineqs = bmap.inequalitiesMatrix();
    auto nOutputDims = bmap.getOutDimCount();

    vector<Aff> stack;
    stack.reserve(nOutputDims);
    inferFromConditions_backtracking(inferedCandidates, bmap.getLocalSpace(), eqs, ineqs, stack);
  }
#endif

  SmallVector<MultiAff, 16 > midUpCandidates;
  SmallVector<MultiAff, 16 > midDnCandidates;
#if 0
  midUpCandidates.reserve(lexmin.nPieces() * lexmax.nPieces());
  midDnCandidates.reserve(lexmin.nPieces() * lexmax.nPieces());
  for (auto &minpair : lexmin.getPieces()) {
    auto &minset = minpair.first;
    auto &minma = minpair.second;

    for (auto &maxpair : lexmax.getPieces()) {
      auto &maxset = maxpair.first;
      auto &maxma = maxpair.second;

      auto divdown = zeroaff;
      auto divup = zeroaff;
      for (auto i = nDims - nDims; i < nDims; i += 1) {

        divdown[i] = (maxma[i] + minma[i]) / 2;
        divup[i] = (maxma[i] + minma[i] + 1) / 2;
      }
      midDnCandidates.push_back(divdown);
      midUpCandidates.push_back(divup);
    }
  }
#endif
  SmallVector<MultiAff, 16> candidates;
  candidates.reserve(inferedCandidates.size() + 2 * midDnCandidates.size() + 2 * midUpCandidates.size() + 2 * lexmin.nPieces() + 2 * lexmax.nPieces());
  for (const auto &candid : inferedCandidates) {
    candidates.push_back(candid);
  }
  for (const auto &candid : midUpCandidates) {
    candidates.push_back(candid.removeDivsUsingFloor());
  }
  for (const auto &candid : midDnCandidates) {
    candidates.push_back(candid.removeDivsUsingCeil());
  }
  for (auto &minpair : lexmin.getPieces()) {
    candidates.push_back(minpair.second.removeDivsUsingCeil());
  }
  for (auto &maxpair : lexmax.getPieces()) {
    candidates.push_back(maxpair.second.removeDivsUsingFloor());
  }
  for (const auto &candid : midUpCandidates) {
    candidates.push_back(candid);
  }
  for (const auto &candid : midDnCandidates) {
    candidates.push_back(candid);
  }
  for (auto &minpair : lexmin.getPieces()) {
    candidates.push_back(minpair.second);
  }
  for (auto &maxpair : lexmax.getPieces()) {
    candidates.push_back(maxpair.second);
  }


  auto nCandidates = candidates.size();
  for (auto i = nCandidates - nCandidates; i < nCandidates; i += 1) {
    auto candid1 = candidates[i];
    for (auto j = i + 1; j < nCandidates;) {
      auto candid2 = candidates[j];
      if (candid1 == candid2) {
        candidates.erase(candidates.begin() + j);
        nCandidates -= 1;
        continue;
      }
      j += 1;
    }
  }


  // TODO: Sort candidates by expression complexity


  auto result = space.createEmptyPwMultiAff();
  auto uncovered = getDomain();
  while (true) {
    //uncovered.coalesce_inplace();
    if (uncovered.isEmpty())
      break;
    auto nUncovered = uncovered.getBasicSetCount();
    auto nCandidates = candidates.size();
    assert(nCandidates >= 1);
    MultiAff best;
    unsigned bestPos;
    int bestNCovered;
    int bestNRemaining;
    Set bestCovered;
    Set bestRemaining;
    uint32_t bestComplexity;
    for (auto i = nCandidates - nCandidates; i < nCandidates;) {
      auto &candidate = candidates[i];
      auto covers = isl::intersect(candidate, *this).domain();
      auto covered = isl::intersect(uncovered, covers);
      if (covered.isEmpty()) {
        // This candidate is useless; remove it from candidates
        candidates.erase(candidates.begin() + i);
        nCandidates -= 1;
        continue;
      }

      auto remainingUncovered = isl::subtract(uncovered, covers);
      auto nCovered = covered.getBasicSetCount();
      auto nRemainingUncovered = remainingUncovered.getBasicSetCount();
      auto complexity = candidate.getComplexity();
      if (best.isNull() || isRhsBetter(nUncovered, best, bestComplexity, bestNRemaining, bestNCovered, candidate, complexity, nRemainingUncovered, nCovered)) {
        best = candidate;
        bestPos = i;

        // Remember best stats to avoid recomputing them
        bestComplexity = complexity;
        bestNCovered = nCovered;
        bestNRemaining = nRemainingUncovered;
        bestRemaining = remainingUncovered;
        bestCovered = covered;
        //if (bestNRemaining == 0 && bestNCovered<=1) break; // Ideal solution
      }
      i += 1;
    }

    assert(best.isValid());
    auto piece = PwMultiAff::create(std::move(bestCovered), std::move(best));
    result.unionAdd_inplace(piece);
    uncovered = bestRemaining;

    candidates.erase(candidates.begin() + bestPos);
    nCandidates -= 1;
  }

  assert(result.domain() == this->domain());
  assert(result.toMap() <= *this);

  result.simplify_inplace();
  if (result.nPieces() > lexmin.nPieces()) {
    result = lexmin; // Is this even possible to happen?
  }
  if (result.nPieces() > lexmax.nPieces()) {
    result = lexmax; // Is this even possible to happen?
  }
  
  assert(result.domain() == this->domain());
  assert(result.toMap() <= *this);
  return result;
}
