#include "islpp/Space.h"

#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Set.h"
#include "islpp/BasicSet.h"
#include "islpp/Map.h"
#include "islpp/BasicMap.h"
#include "cstdiofile.h"
#include "islpp/MultiPwAff.h"
#include "islpp/Point.h"
#include "islpp/Id.h"
#include "islpp/Constraint.h"
#include "islpp/UnionMap.h"
#include "islpp/AstBuild.h"
#include "islpp/DimRange.h"
#include "islpp/AffExpr.h"
#include "islpp/ParamSpace.h"
#include "islpp/SetSpace.h"
#include "islpp/MapSpace.h"

#include <isl/space.h>
#include <isl/set.h>
#include <isl/map.h>

using namespace isl;
using namespace std;


void Space::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


MapSpace Space::createMapSpace(const Ctx *ctx, unsigned nparam, unsigned n_in, unsigned n_out) {
  return MapSpace::enwrap(isl_space_alloc(ctx->keep(), nparam, n_in, n_out));
}


ParamSpace Space::createParamsSpace(const Ctx *ctx, unsigned nparam) {
  return Space::enwrap(isl_space_params_alloc(ctx->keep(), nparam));
}


SetSpace Space::createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim) {
  return Space::enwrap(isl_space_set_alloc(ctx->keep(), nparam, dim));
}


MapSpace Space::createMapFromDomainAndRange(Space domain, Space range) {
  assert(domain.getCtx() == range.getCtx());
  compatibilize(domain, range);
  return Space::enwrap(isl_space_map_from_domain_and_range(domain.take(), range.take()));
}


BasicMap Space::emptyBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_empty(takeCopy()));
}


BasicMap Space::universeBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(takeCopy()));
}


BasicMap Space::identityBasicMap() const {
  assert(isMapSpace());
  return BasicMap::enwrap(isl_basic_map_identity(takeCopy()));
}


BasicMap Space::equalBasicMap(unsigned n_equal) const {
  return BasicMap::enwrap(isl_basic_map_equal(takeCopy(), n_equal));
}


BasicMap Space::equalBasicMap(isl_dim_type type1, unsigned pos1, unsigned count, isl_dim_type type2, unsigned pos2) const {
  auto result = universeBasicMap();

  if (type1 == isl_dim_in && pos1 == 0 && type2 == isl_dim_out && pos2 == 0) {
    return equalBasicMap(count);
  }
  if (type1 == isl_dim_out && pos1 == 0 && type2 == isl_dim_in && pos2 == 0) {
    return equalBasicMap(count);
  }

  // TODO: There might be something more efficient
  for (auto i = count - count; i < count; i += 1) {
    result.equate_inplace(type1, pos1 + i, type2, pos2 + i);
  }

  return result;
}


BasicMap Space::equalSubspaceBasicMap(const Space &domainSubpace, const Space &rangeSubspace) const {
  assert(isMapSpace());

  auto domainDims = findSubspace(isl_dim_in, domainSubpace);
  assert(domainDims.isValid());

  auto rangeDims = findSubspace(isl_dim_out, rangeSubspace);
  assert(rangeDims.isValid());

  assert(domainDims.getCount() == rangeDims.getCount());
  return equalBasicMap(isl_dim_in, domainDims.getBeginPos(), domainDims.getCount(), isl_dim_out, rangeDims.getBeginPos());
}


BasicMap Space::equalSubspaceBasicMap(const Space &subspace) const {
  return equalSubspaceBasicMap(subspace, subspace);
}


BasicMap Space::lessAtBasicMap(unsigned pos) const {
  return BasicMap::enwrap(isl_basic_map_less_at(takeCopy(), pos));
}


BasicMap Space::moreAtBasicMap(unsigned pos) const {
  return BasicMap::enwrap(isl_basic_map_more_at(takeCopy(), pos));
}


Map Space::emptyMap() const {
  return Map::enwrap(isl_map_empty(takeCopy()));
}


Map Space::universeMap() const {
  return Map::enwrap(isl_map_universe(takeCopy()));
}


Map Space::identityMap() const {
  assert(isMapSpace());
  return Map::enwrap(isl_map_identity(takeCopy()));
}


Map Space::lexLtMap() const {
  return Map::enwrap(isl_map_lex_lt(takeCopy()));
}


Map Space::lexLeMap() const {
  return Map::enwrap(isl_map_lex_le(takeCopy()));
}


Map Space::lexGtMap() const {
  return Map::enwrap(isl_map_lex_gt(takeCopy()));
}


Map Space::lexGeMap() const {
  return Map::enwrap(isl_map_lex_ge(takeCopy()));
}


Map Space::lexLtFirstMap(unsigned pos) const {
  return Map::enwrap(isl_map_lex_lt_first(takeCopy(), pos));
}


Map Space::lexLeFirstMap(unsigned pos) const {
  return Map::enwrap(isl_map_lex_le_first(takeCopy(), pos));
}


Map Space::lexGtFirstMap(unsigned pos) const {
  return Map::enwrap(isl_map_lex_gt_first(takeCopy(), pos));
}


Map Space::lexGeFirstMap(unsigned pos) const {
  return Map::enwrap(isl_map_lex_ge_first(takeCopy(), pos));
}


UnionMap Space::emptyUnionMap() const {
  assert(isParamsSpace());
  return UnionMap::enwrap(isl_union_map_empty(takeCopy()));
}


UnionSet Space::emptyUnionSet() const {
  assert(isParamsSpace());
  return UnionSet::enwrap(isl_union_set_empty(takeCopy()));
}


Aff Space::createZeroAff() const {
  assert(isSetSpace());
  return Aff::enwrap(isl_aff_zero_on_domain(isl_local_space_from_space(takeCopy())));
}


Aff Space::createConstantAff(const Int &c) const {
  auto zero = createZeroAff();
  zero.setConstant_inplace(c);
  return zero;
}


Aff Space::createVarAff(isl_dim_type type, pos_t pos) const {
  assert(isSetSpace());
  assert(type == isl_dim_param || type == isl_dim_set);
  return Aff::enwrap(isl_aff_var_on_domain(isl_local_space_from_space(takeCopy()), type, pos));
}


Aff Space::createAffOnVar(pos_t pos) const {
  assert(isSetSpace());
  return Aff::enwrap(isl_aff_var_on_domain(isl_local_space_from_space(takeCopy()), isl_dim_set, pos));
}


Aff Space::createAffOnParam(const Id &dimId) const {
  assert(isSetSpace());
  auto pos = findDimById(isl_dim_param, dimId);
  assert(pos >= 0);
  return Aff::enwrap(isl_aff_var_on_domain(isl_local_space_from_space(takeCopy()), isl_dim_param, pos));
}


AffExpr Space::createVarAffExpr(isl_dim_type type, unsigned pos) const {
  return AffExpr::createVar(*this, type, pos);
}


PwAff Space::createEmptyPwAff() const {
  assert(isMapSpace());
  assert(getOutDimCount() == 1);
  return PwAff::enwrap(isl_pw_aff_empty(takeCopy()));
}


PwAff Space::createZeroPwAff() const {
  assert(isMapSpace());
  assert(getOutDimCount() == 1);
  return PwAff::enwrap(isl_pw_aff_zero_on_domain(isl_local_space_from_space(takeCopy())));
}


MultiAff Space::createZeroMultiAff() const {
  //assert(isMapSpace());
  return MultiAff::enwrap(isl_multi_aff_zero(takeCopy()));
}


MultiAff Space::createIdentityMultiAff() const {
  assert(isMapSpace());
  return MultiAff::createIdentity(*this);
}


MultiPwAff Space::createZeroMultiPwAff() const {
  //assert(this->isMapSpace());
  return MultiPwAff::enwrap(isl_multi_pw_aff_zero(takeCopy()));
}


ISLPP_EXSITU_ATTRS MultiPwAff Space::createEmptyMultiPwAff() ISLPP_EXSITU_FUNCTION {
  return MultiPwAff::enwrap(isl_multi_pw_aff_from_pw_multi_aff(isl_pw_multi_aff_empty(takeCopy())));
}


PwMultiAff Space::createEmptyPwMultiAff() const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_empty(takeCopy()));
}


Point Space::zeroPoint() const {
  return Point::enwrap(isl_point_zero(keep()));
}


Constraint Space::createZeroConstraint() const {
  return Constraint::enwrap(isl_equality_alloc(isl_local_space_from_space(takeCopy())));
}

Constraint Space::createConstantConstraint(int v) const {
  auto result = isl_equality_alloc(isl_local_space_from_space(takeCopy()));
  result = isl_constraint_set_constant_si(result, v);
  return Constraint::enwrap(result);
}
Constraint Space::createVarConstraint(isl_dim_type type, int pos) const {
  auto result = isl_equality_alloc(isl_local_space_from_space(takeCopy()));
  result = isl_constraint_set_coefficient_si(result, type, pos, 1);
  return Constraint::enwrap(result);
}


Constraint Space::createEqualityConstraint() const {
  return Constraint::enwrap(isl_equality_alloc(isl_local_space_from_space(takeCopy())));
}


Constraint Space::createInequalityConstraint() const {
  return Constraint::enwrap(isl_inequality_alloc(isl_local_space_from_space(takeCopy())));
}


Constraint Space::createLtConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  return createGeConstraint(std::move(lhs), std::move(rhs));
}
Constraint Space::createLeConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  return createGtConstraint(std::move(lhs), std::move(rhs));
}
Constraint Space::createEqConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take());
  return Constraint::enwrap(isl_equality_from_aff(term));
}

Constraint Space::createEqConstraint(Aff &&lhs, int rhs) const {
  return createEqConstraint(std::move(lhs), lhs.getLocalSpace().createConstantAff(rhs));
}


Constraint Space::createEqConstraint(Aff &&lhs, isl_dim_type type, unsigned pos) const {
  auto term = isl_equality_from_aff(lhs.take());
  isl_int coeff;
  isl_int_init(coeff);
  isl_constraint_get_coefficient(term, type, pos, &coeff);
  isl_int_sub_ui(coeff, coeff, 1);
  isl_constraint_set_coefficient(term, type, pos, coeff);
  isl_int_clear(coeff);
  return Constraint::enwrap(term);
}


Constraint Space::createGeConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs >= 0
  auto c = isl_inequality_from_aff(term); // TODO: Confirm
  return Constraint::enwrap(c);
}
Constraint Space::createGtConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs > 0
  term = isl_aff_add_constant_si(term, -1); // lhs - rhs - 1 >= 0
  auto c = isl_inequality_from_aff(term);
  return Constraint::enwrap(c);
}


Expr Space::createVarExpr(isl_dim_type type, int pos) const {
  return Expr::createVar(this->asLocalSpace(), type, pos);
}

Set Space::emptySet() const {
  assert(dim(isl_dim_in) == 0);
  return Set::enwrap(isl_set_empty(takeCopy()));
}


Set Space::universeSet() const {
  assert(dim(isl_dim_in) == 0);
  return Set::enwrap(isl_set_universe(takeCopy()));
}


BasicSet Space::emptyBasicSet() const {
  assert(isSetSpace());
  return BasicSet::enwrap(isl_basic_set_empty(takeCopy()));
}


BasicSet Space::universeBasicSet() const {
  assert(isSetSpace());
  return BasicSet::enwrap(isl_basic_set_universe(takeCopy()));
}


bool Space::isParamsSpace() const {
  return isl_space_is_params(keep());
}


bool Space::isSetSpace() const {
  return isl_space_is_set(keep());
}


bool Space::isMapSpace() const {
  return isl_space_is_map(keep());
}


bool Space::isWrapping() const{
  return isl_space_is_wrapping(keep());
}

#if 0
void Space::fromDomain(){
  give(isl_space_from_domain(take()));
}


void Space::fromRange() {
  give(isl_space_from_range(take()));
}
#endif

void Space::setFromParams(){
  give(isl_space_set_from_params(take()));
}
void Space::reverse(){
  give(isl_space_reverse(take()));
}


void Space::mapFromSet(){
  give(isl_space_map_from_set(take()));
}
void Space::zip(){
  give(isl_space_zip(take()));
}
void Space::curry(){
  give(isl_space_curry(take()));
}
void Space::uncurry(){
  give(isl_space_uncurry(take()));
}



Set Space::createUniverseSet() const {
  return Set::enwrap(isl_set_universe(takeCopy()));
}

BasicSet Space::createUniverseBasicSet() const {
  return BasicSet::wrap(isl_basic_set_universe(takeCopy()));
}

Map Space::createUniverseMap() const {
  return Map::enwrap(isl_map_universe(takeCopy()));
}

BasicMap Space::createUniverseBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(takeCopy()));
}


UnionMap Space::createEmptyUnionMap() const {
  return UnionMap::enwrap(isl_union_map_empty(takeCopy()));
}


LocalSpace Space::asLocalSpace() const {
  return LocalSpace::enwrap(isl_local_space_from_space(takeCopy()));
}


AstBuild Space::createAstBuild() const {
  return AstBuild::enwrap(isl_ast_build_from_context(isl_set_universe(takeCopy())));
}

#if 0
Space Space::getNestedOrDefault(isl_dim_type type) const {
  if (isNested(type))
    return getNested(type);
  return extractTuple(type);
}


static bool findNestedTuple_recursive(const Space &space, const Id &tupleToFind, unsigned &pos, unsigned &n) {
  auto startPos = pos;

  auto dom = space.getNestedDomain();
  if (dom.isValid()) {
    auto retval = findNestedTuple_recursive(dom, tupleToFind, pos, n);
    if (retval) {
      assert(pos == startPos);
      return true;
    }
  } else {
    auto domId = space.getInTupleId();
    if (domId == tupleToFind) {
      n = space.getInDimCount();
      return true;
    }
    pos += space.getInDimCount();
  }
  assert(pos == startPos+space.getInDimCount());

  auto range = space.getNestedRange();
  if (range.isValid()) {
    auto retval = findNestedTuple_recursive(range, tupleToFind, pos, n);
    if (retval) {
      assert(pos == startPos);
      return true;
    }
  } else {
    auto rangeId = space.getInTupleId();
    if (rangeId == tupleToFind) {
      n = space.getOutDimCount();
      return true;
    }
    pos += space.getOutDimCount();
  }
  assert(pos == startPos+space.getOutDimCount());

  return false;
}


static bool findNestedTuple_recursive(const Space &space, unsigned &tuplePos, unsigned &pos, unsigned &n, Id &id) {
  auto dom = space.getNestedDomain();
  if (dom.isValid()) {
    auto retval = findNestedTuple_recursive(dom, tuplePos, pos, n, id);
    if (retval)
      return true;
  } else if (tuplePos == 0) {
    n = space.getInDimCount();
    id = space.getInTupleId();
    return true;
  }  else {
    pos += space.getInDimCount();
    tuplePos -= 1;
  }

  auto range = space.getNestedRange();
  if (range.isValid()) {
    auto retval = findNestedTuple_recursive(range, tuplePos, pos, n, id);
    if (retval)
      return true;
  } else if (tuplePos == 0) {
    n = space.getOutDimCount();
    id = space.getOutTupleId();
    return true;
  } else {
    pos += space.getOutDimCount();
  }

  return false;
}


DimRange Space::findNestedTuple(unsigned tuplePos) const {
  if (isMapSpace()) {
    unsigned pos,count;
    Id id;
    if (findNestedTuple_recursive(*this, tuplePos, pos, count, id)) {
      auto nDomainDims = getInDimCount();
      if (pos >= nDomainDims) 
        return DimRange::enwrap(isl_dim_in, pos, count, *this);
      else
        return DimRange::enwrap(isl_dim_out, pos-nDomainDims, count, *this); 
    }
  } else if (isSetSpace()) {
    auto nested = getNested();
    unsigned pos,count;
    Id id;
    if (findNestedTuple_recursive(nested, tuplePos, pos, count, id)) {
      return DimRange::enwrap(isl_dim_set, pos, count, *this); 
    }
  }

  return DimRange();
}


DimRange Space:: findNestedTuple(const Id &tupleId) const {
  if (isMapSpace()) {
    unsigned pos, count;
    if (findNestedTuple_recursive(*this, tupleId, pos, count)) {
      auto nDomainDims = getInDimCount();
      if (pos >= nDomainDims) 
        return DimRange::enwrap(isl_dim_in, pos, count, *this);
      else
        return DimRange::enwrap(isl_dim_out, pos-nDomainDims, count, *this); 
    }
  } else if (isSetSpace()) {
    auto nested = getNested();
    unsigned pos,count;
    Id id;
    if (findNestedTuple_recursive(nested, tupleId, pos, count)) {
      return DimRange::enwrap(isl_dim_set, pos, count, *this); 
    }
  }

  return DimRange();
}
#endif

static unsigned countSubspaceMatches_recursive(const Space &space, const Space &spaceToFind) {
  assert(spaceToFind.isValid());
  if (matchesSpace(space, spaceToFind)) {
    return 1;
  }

  if (space.isWrapping()) {
    return countSubspaceMatches_recursive(space.unwrap(), spaceToFind);
  }

  if (space.isMapSpace()) {
    return countSubspaceMatches_recursive(space.domain(), spaceToFind) + countSubspaceMatches_recursive(space.range(), spaceToFind);
  }

  return 0;
}


static bool findSubspace_recursive(const Space &space, const Space &spaceToFind, /*inout*/unsigned &currentDimPos) {
  assert(spaceToFind.isValid());
  if (matchesSpace(space, spaceToFind)) {
    return true;
  }

  if (space.isWrapping()) {
    return findSubspace_recursive(space.unwrap(), spaceToFind, currentDimPos);
  }

  if (space.isMapSpace()) {
    return findSubspace_recursive(space.domain(), spaceToFind, currentDimPos) || findSubspace_recursive(space.range(), spaceToFind, currentDimPos);
  }

  currentDimPos += space.getSetDimCount();
  return false;
}


DimRange Space::findSubspace(isl_dim_type type, const Space &subspace) const {
  assert((isSetSpace() && type == isl_dim_set) || (isMapSpace() && (type == isl_dim_in || type == isl_dim_out)));

  // trivial case
  auto nested = extractTuple(type);
  auto seek = subspace.normalizeUnwrapped();
  if (::matchesSpace(nested, seek))
    return DimRange::enwrap(type, 0, this->dim(type), *this);

  unsigned currentDimPos = 0;
  if (findSubspace_recursive(nested, seek, currentDimPos)) {
    auto result = DimRange::enwrap(type, currentDimPos, subspace.dim(isl_dim_in) + subspace.dim(isl_dim_out), *this);
    assert(countSubspaceMatches_recursive(nested, subspace) == 1 && "subspace must be unique");
    return result;
  }

  return DimRange();
}


static isl_dim_type reverseDimType(isl_dim_type type) {
  assert(type == isl_dim_in || type == isl_dim_out);
  if (type == isl_dim_out)
    return isl_dim_in;
  return isl_dim_out;
}


static bool removeTuple(/*out*/Space &result, /*in*/const Space &space, /*in*/unsigned soughtTuplePos, /*out*/unsigned &numTuples, /*out*/unsigned &first, /*out*/unsigned &count, /*out*/Id &id) {
  auto runningDimPos = 0;
  auto runningTuplePos = 0;

  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    auto tupleDimCount = space.dim(type);

    if (space.isNested(type)) {
      // Inner node
      auto nested = space.extractTuple(type);

      Space nestedResult;
      unsigned nestedTupleCount, nestedFirst;
      if (removeTuple(nestedResult, nested, soughtTuplePos - runningTuplePos, nestedTupleCount, nestedFirst, count, id)) {
        // Tuple found, therefore replace tuple in space

        if (nestedResult.isNull()) {
          // Remove tuple from space, i.e. return the other tuple
          result = space.isMapSpace() ? space.extractTuple(reverseDimType(type)) : Space();
        } else {
          // Replace the previously nested space by the new one
          result = space.setNested(type, nestedResult);
        }
        first = runningDimPos + nestedFirst;
        return true;
      } else {
        // Tuple not found here, continue searching
        assert(first == tupleDimCount);
        assert(nestedTupleCount >= 1);
        runningTuplePos += nestedTupleCount;
      }

    } else {
      // Leaf node

      if (runningTuplePos == soughtTuplePos) {
        // Tuple found
        first = runningDimPos;
        count = tupleDimCount;
        if (space.hasTupleId(type))
          id = space.getTupleId(type);
        result = Space(); // The caller is supposed to remove this tuple
        return true;
      }

      // Not the tupled sought, continue searching
      runningTuplePos += 1;
    }

    runningDimPos += tupleDimCount;
  }

  first = runningDimPos;
  numTuples = runningTuplePos;
  return false;
}


static bool insertTuple(/*out*/Space &result, /*in*/const Space &space, /*in*/const Space &insert, /*in*/unsigned insertTuplePos, /*out*/unsigned &first, /*out*/unsigned &tupleCount) {
  unsigned runningTuplePos = 0;
  unsigned runningDimPos = 0;

  if (space.isMapSpace()) {
    auto nDomainDims = space.getInDimCount();
    auto domainDimPos = 0;
    auto domainTuplePos = 0;
    auto nRangeDims = space.getOutDimCount();
    auto rangeDimPos = domainDimPos + nDomainDims;
    unsigned rangeTuplePos;

    if (insertTuplePos == domainTuplePos) {
      // Insert here, before the domain
      // That is, we insert as a new domain an move the current space as new range
      result = space.wrap();
      result.insertDims_inplace(isl_dim_in, 0, space.getInDimCount() + space.getOutDimCount());
      result = combineSpaces(result.domain(), space);
      //result.castRange_inplace(space);
      //TODO: Copy DimIds
      first = 0;
      return true;
    }

    if (space.isNestedDomain()) {
      auto nestedDomain = space.domain();

      Space nestedResult;
      unsigned nestedFirst, nestedTupleCount;
      if (insertTuple(nestedResult, nestedDomain, insert, insertTuplePos + domainTuplePos, nestedFirst, nestedTupleCount)) {
        result = combineSpaces(nestedResult, space.range());
        // result = space.castDomain(nestedResult);
        first = domainDimPos + nestedFirst;
        return true;
      }

      rangeTuplePos = domainTuplePos + nestedTupleCount;
    } else {
      rangeTuplePos = domainTuplePos + 1;
    }


    if (insertTuplePos == rangeTuplePos) {
      // insert here, between domain and range
      // We create a new nested range of the space to insert and the old range
      auto insertDimCount = insert.getInDimCount() + insert.getOutDimCount();
      auto nestedRange = space.range();
      nestedRange.wrap_inplace();
      nestedRange.addDims_inplace(isl_dim_in, insertDimCount);
      nestedRange = combineSpaces(insert, nestedRange.range());
      //nestedRange.setInNested_inplace(insert);
      result = space.insertDims(isl_dim_out, 0, insertDimCount);
      result = combineSpaces(result.domain(), nestedRange);
      //result.setNested_inplace(isl_dim_out, nestedRange);
      first = rangeDimPos;
      return true;
    }

    if (space.isNestedRange()) {
      auto nestedRange = space.range();
      Space nestedResult;
      unsigned nestedFirst, nestedTupleCount;
      if (insertTuple(nestedResult, nestedRange, insert, insertTuplePos - rangeTuplePos, nestedFirst, nestedTupleCount)) {
        //result = space.setOutNested(nestedRange);
        result = combineSpaces(space.domain(), nestedRange);
        first = rangeDimPos + nestedFirst;
        return true;
      }

      runningTuplePos = rangeTuplePos + nestedTupleCount;
    } else {
      runningTuplePos = rangeTuplePos + 1;
    }
  } else {
    assert(space.isSetSpace());

    if (insertTuplePos == runningTuplePos) {
      // Insert before
      first = runningTuplePos;
      result = space.insertDims(isl_dim_in, 0, insert.getInDimCount() + insert.getOutDimCount());
      result = combineSpaces(insert, result.range());
      //result.setNested_inplace(isl_dim_in, insert);
      return true;
    }

    auto nDims = space.getSetDimCount();
    if (space.isNestedSet()) {
      auto nested = space.getNested();

      Space nestedResult;
      unsigned nestedFirst, nestedTupleCount;
      if (insertTuple(nestedResult, nested, insert, insertTuplePos - runningTuplePos, nestedFirst, nestedTupleCount)) {
        first = runningTuplePos + nestedFirst;
        result = nestedResult;
        return true;
      }

      runningTuplePos += nestedTupleCount;
    }

    auto tupleId = space.getSetTupleId();

  }

  first = runningDimPos;
  tupleCount = runningTuplePos;
  return false;
}


Space Space::moveTuple(isl_dim_type dst_type, unsigned dst_tuplePos, isl_dim_type src_type, unsigned src_tuplePos) const {
  assert(dst_type == isl_dim_in || dst_type == isl_dim_out);
  assert(src_type == isl_dim_in || src_type == isl_dim_out);

  if (dst_type == src_type && dst_tuplePos == src_tuplePos)
    return copy();

  Space tmp;
  unsigned numTuples;
  unsigned first;
  unsigned count;
  Id id;
  bool removeSuccessful = removeTuple(tmp, *this, src_tuplePos, numTuples, first, count, id);
  assert(removeSuccessful);

  Space result;
  unsigned tupleCount;
  bool insertSuccessful = insertTuple(result, tmp, this->params().addDims(isl_dim_set, count), dst_tuplePos, first, tupleCount);
  assert(insertSuccessful);

  return result;
}



static void queryNesting_recursive(const Space &space, /*out*/unsigned &nestedCount, /*out*/unsigned &nestedDepth) {
  if (space.isNull() || space.isParamsSpace()) {
    nestedCount = 0;
    nestedDepth = 0;
    return;
  }

  unsigned tupleCount = 0;
  unsigned maxDepth = 1;
  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    if (space.isNested(type)) {

      auto part = space.extractTuple(type);
      unsigned partNestedCount, partNestedDepth;
      queryNesting_recursive(part, partNestedCount, partNestedDepth);
      maxDepth = std::max(maxDepth, partNestedDepth + 1);
      tupleCount += partNestedCount;
    } else {
      tupleCount += 1;
    }
  }

  nestedCount = tupleCount;
  nestedDepth = maxDepth;
}


unsigned Space::nestedTupleCount() const {
  unsigned nestedCount, nestedDepth;
  queryNesting_recursive(*this, nestedCount, nestedDepth);
  return nestedCount;
}


unsigned Space::nestedMaxDepth() const {
  unsigned nestedCount, nestedDepth;
  queryNesting_recursive(*this, nestedCount, nestedDepth);
  return nestedDepth;
}


static bool findTupleByPosition_recursive(const Space &space, unsigned searchTuplePos, /*inout*/unsigned &currentTuplePos, /*inout*/unsigned &currentDimPos, /*out*/unsigned &foundTupleDimCount, /*out*/Id &tupleId) {
  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    if (currentTuplePos > searchTuplePos)
      return false;

    auto dimCount = space.dim(type);
    auto startDimPos = currentDimPos;

    if (searchTuplePos == searchTuplePos) {
      // Found the tuple
      foundTupleDimCount = dimCount;
      tupleId = space.getTupleIdOrNull(type);
      return true; // Abort the recursion
    }

    if (space.isNested(type)) {
      auto nestedSpace = space.extractTuple(type);
      if (findTupleByPosition_recursive(nestedSpace, searchTuplePos, currentTuplePos, currentDimPos, foundTupleDimCount, tupleId)) {
        return true;
      }
    } else {
      currentTuplePos += 1;
      currentDimPos += dimCount;
    }

    assert(currentDimPos == startDimPos + dimCount);
  }

  return false;
}


bool Space::findTuple(isl_dim_type type, unsigned tuplePos, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount, /*out*/Id &tupleId) const {
  if (!isNested(type)) {
    firstDim = 0;
    dimCount = dim(type);
    tupleId = getTupleIdOrNull(type);
    return tuplePos == 0;
  }

  dimCount = -1;
  tupleId = Id();
  unsigned currentTuplePos = 0;
  firstDim = 0;
  return findTupleByPosition_recursive(extractTuple(type), tuplePos, currentTuplePos, firstDim, dimCount, tupleId);
}


static bool findTupleByDimPosition_recursive(const Space &space, unsigned searchDimPos, /*inout*/unsigned &currentTuplePos, /*inout*/unsigned &currentDimPos, /*out*/unsigned &foundTupleDimCount, /*out*/Id &tupleId) {
  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    if (currentDimPos > searchDimPos)
      return false;

    auto dimCount = space.dim(type);
    auto startDimPos = currentDimPos;

    if (space.isNested(type)) {
      auto nestedSpace = space.extractTuple(type);
      if (findTupleByDimPosition_recursive(nestedSpace, searchDimPos, currentTuplePos, currentDimPos, foundTupleDimCount, tupleId)) {
        return true;
      }
    } else {
      if (currentDimPos <= searchDimPos && searchDimPos < currentDimPos + dimCount) {
        // Found the tuple
        foundTupleDimCount = dimCount;
        tupleId = space.getTupleIdOrNull(type);
        return true; // Abort the recursion
      }


      currentTuplePos += 1;
      currentDimPos += dimCount;
    }

    assert(currentDimPos == startDimPos + dimCount);
  }

  return false;
}


bool Space::findTupleAt(isl_dim_type type, unsigned dimPos, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount, /*out*/Id &tupleId, /*out*/unsigned &tuplePos) const {
  if (!isNested(type)) {
    firstDim = 0;
    dimCount = dim(type);
    tupleId = getTupleIdOrNull(type);
    tuplePos = 0;
    return tuplePos == 0;
  }

  dimCount = -1;
  tupleId.clear();
  tuplePos = 0;
  firstDim = 0;
  return findTupleByPosition_recursive(extractTuple(type), tuplePos, tuplePos, firstDim, dimCount, tupleId);
}


static bool findTupleById_recursive(const Space &space, const Id &searchTupleId, /*inout*/unsigned &currentTuplePos, /*inout*/unsigned &currentDimPos, /*out*/unsigned &foundTupleDimCount) {
  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    auto dimCount = space.dim(type);
    auto startDimPos = currentDimPos;

    if (searchTupleId == space.getTupleIdOrNull(type)) {
      // Found the tuple
      foundTupleDimCount = dimCount;
      return true; // Abort the recursion
    }

    if (space.isNested(type)) {
      auto nestedSpace = space.extractTuple(type);
      if (findTupleById_recursive(nestedSpace, searchTupleId, currentTuplePos, currentDimPos, foundTupleDimCount)) {
        return true;
      }
    } else {
      currentTuplePos += 1;
      currentDimPos += dimCount;
    }

    assert(currentDimPos == startDimPos + dimCount);
  }

  return false;
}


bool Space::findTuple(isl_dim_type type, const Id &tupleToFind, /*out*/unsigned &firstDim, /*out*/unsigned &dimCount) const {
  if (!isNested(type)) {
    firstDim = 0;
    dimCount = dim(type);
    return tupleToFind == getTupleIdOrNull(type);
  }

  dimCount = -1;
  unsigned currentTuplePos = 0;
  firstDim = 0;
  return findTupleById_recursive(extractTuple(type), tupleToFind, currentTuplePos, firstDim, dimCount);
}


unsigned Space::findTuplePos(isl_dim_type type, const Id &tupleToFind) const {
  if (!isNested(type)) {
    assert(getTupleId(type) == tupleToFind);
    return 0;
  }

  unsigned dimCount = -1;
  unsigned currentTuplePos = 0;
  unsigned firstDim = 0;
  auto retval = findTupleById_recursive(extractTuple(type), tupleToFind, currentTuplePos, firstDim, dimCount);
  assert(retval);
  return currentTuplePos;
}


Space Space::extractNestedTupleSpace(isl_dim_type type, unsigned tuplePos) const {
  unsigned firstDim, dimCount;
  Id id;
  if (!findTuple(type, tuplePos, firstDim, dimCount, id))
    return Space();

  auto result = extractDimRange(type, firstDim, dimCount);
  result.setTupleId_inplace(type, id);
  return result;
}


Space Space::extractNestedTupleSpace(isl_dim_type type, const Id &tupleToFind) const {
  return extractNestedTupleSpace(type, findTuplePos(type, tupleToFind));
}


Space Space::extractDimRange(isl_dim_type type, unsigned first, unsigned count) const {
  auto result = extractTuple(type);
  result.removeDims_inplace(type, first + count, result.dim(type) - first - count);
  result.removeDims_inplace(type, 0, first);
  return result;
}


static void flattenNestedSpaces_recursive(/*inout*/std::vector<Space> &result, /*in*/const Space &space) {
  if (space.isParamsSpace())
    return;

  for (auto type = space.isMapSpace() ? isl_dim_in : isl_dim_set; type <= isl_dim_out; ++type) {
    if (space.isNested(type)) {
      auto nested = space.extractTuple(type);
      flattenNestedSpaces_recursive(result, nested);
    } else {
      result.push_back(space.extractTuple(type));
    }
  }
}


std::vector<Space> Space::flattenNestedSpaces() const {
  std::vector<Space> result;
  flattenNestedSpaces_recursive(result, *this);
  return result;
}


Space isl::combineSpaces(const Space &lhs, const Space &rhs) {
  if (lhs.isNull() && rhs.isNull())
    return Space();

  if (rhs.isNull())
    return lhs;

  if (lhs.isNull())
    return rhs;

  return Space::createMapFromDomainAndRange(lhs.isMapSpace() ? lhs.wrap() : lhs, rhs.isMapSpace() ? rhs.wrap() : rhs);
}


static bool replaceSubspace_recursive(/*inout*/Space &space, const Space &toBeReplaced, const Space &replacement) {
  assert(space.isValid());
  assert(toBeReplaced.isValid());

  if (matchesSpace(space, toBeReplaced)) {
    space = replacement;
    return true;
  }

  if (space.isMapSpace()) {
    auto domain = space.domain();
    auto range = space.range();
    if (replaceSubspace_recursive(domain, toBeReplaced, replacement))
      return space = combineSpaces(domain, range), true;

    if (replaceSubspace_recursive(range, toBeReplaced, replacement))
      return space = combineSpaces(domain, range), true;

    return false;
  }

  assert(space.isSetSpace());
  if (space.isWrapping()) {
    space = space.unwrap();
    return replaceSubspace_recursive(space, toBeReplaced, replacement);
  }

  return false;
}


Space Space::removeSubspace(const Space &subspace) const {
  auto result = copy();
  bool wasSetSpace = isSetSpace();
  auto retval = replaceSubspace_recursive(result, subspace, Space());
  assert(retval && "Subspace must be somewhere in the space");

  if (wasSetSpace && result.isMapSpace())
    result.wrap_inplace();
  return result;
}


Space Space::replaceSubspace(const Space &subspaceToReplace, const Space &replaceBy) const {
  auto result = copy();
  bool wasSetSpace = isSetSpace();
  auto retval = replaceSubspace_recursive(result, subspaceToReplace, replaceBy);
  assert(retval && "Subspace must be somewhere in the space");

  if (wasSetSpace && result.isMapSpace())
    result.wrap_inplace();
  return result;
}


static bool findNthSubspace_recursive(const Space &space, Space &resultSpace, unsigned &posToGo, unsigned &first) {
  if (space.isMapSpace()) {
    if (findNthSubspace_recursive(space.getDomainSpace(), resultSpace, posToGo, first))
      return true;
    if (findNthSubspace_recursive(space.getRangeSpace(), resultSpace, posToGo, first))
      return true;
  } else if (space.isWrapping()) {
    if (findNthSubspace_recursive(space.unwrap(), resultSpace, posToGo, first))
      return true;
  } else {
    // Leaf space
    if (posToGo == 0) {
      // Found
      resultSpace = space;
      return true;
    }

    posToGo -= 1;
    first += space.getSetDimCount();
  }

  return false;
}


Space Space::findNthSubspace(isl_dim_type type, unsigned pos, DimRange &dimrange) const {
  Space result;
  unsigned first = 0;
  auto myspace = extractTuple(type);
  auto found = findNthSubspace_recursive(myspace, result, pos, first);
  assert(found); assert(pos == 0);

  dimrange = DimRange::enwrap(type, first, result.dim(isl_dim_in) + result.dim(isl_dim_out), myspace);
  return result;
}


ISLPP_EXSITU_ATTRS BasicSet isl::Space::equalBasicSet(isl_dim_type type1, unsigned pos1, unsigned count, isl_dim_type type2, unsigned pos2) ISLPP_EXSITU_FUNCTION
{
  auto result = universeBasicSet();
  for (auto i = count - count; i < count; i += 1) {
    result.equate_inplace(type1, pos1 + i, type2, pos2 + i);
  }
  return result;
}


ISLPP_EXSITU_ATTRS BasicSet isl::Space::equalBasicSet(Space subspace1, Space subspace2) ISLPP_EXSITU_FUNCTION
{
  assert(isSetSpace());
  auto range1 = this->findSubspace(isl_dim_set, subspace1);
  assert(range1.isValid());
  auto range2 = this->findSubspace(isl_dim_set, subspace2);
  assert(range2.isValid());
  assert(range1.getCount() == range2.getCount());
  return equalBasicSet(isl_dim_set, range1.getFirst(), range1.getCount(), isl_dim_set, range2.getFirst());
}


ISLPP_EXSITU_ATTRS BasicMap isl::Space::equalBasicMap() ISLPP_EXSITU_FUNCTION{
  assert(isMapSpace());
  assert(dim(isl_dim_in) == dim(isl_dim_out));
  return equalBasicMap(std::min(dim(isl_dim_in), dim(isl_dim_out)));
}


ISLPP_EXSITU_ATTRS MapSpace isl::Space::createMapSpace(count_t nDomainDims, Space rangeSpace) ISLPP_EXSITU_FUNCTION{
  assert(isParamsSpace());
  assert(rangeSpace.isSetSpace());

  auto range = isl_space_align_params(rangeSpace.take(), takeCopy());
  auto domain = isl_space_set_alloc(isl_space_get_ctx(keep()), 0, nDomainDims);
  domain = isl_space_align_params(domain, isl_space_copy(range));
  auto result = isl_space_map_from_domain_and_range(domain, range); domain = nullptr; range = nullptr;
  return Space::enwrap(result);
}

isl::MapSpace isl::Space::createMapSpace(count_t nDomainDims, count_t nRangeDims) const {
  assert(isParamsSpace()); return MapSpace::enwrap(isl_space_align_params(isl_space_alloc(getCtx()->keep(), 0, nDomainDims, nRangeDims), takeCopy()));
}


void isl::Space::dump() const {
  isl_space_dump(keep());
}

isl::UnionSet isl::Space::createEmptyUnionSet() const {
  return UnionSet::enwrap(isl_union_set_empty(takeCopy()));
}


ISLPP_EXSITU_ATTRS BasicMap isl::Space::identityToSelfBasicMap() ISLPP_EXSITU_FUNCTION{
  assert(isSetSpace());
  return BasicMap::enwrap(isl_basic_map_identity(isl_space_map_from_domain_and_range(takeCopy(), takeCopy())));
}

ISLPP_EXSITU_ATTRS MapSpace isl::Space::mapsTo(count_t nOut) ISLPP_EXSITU_FUNCTION {
  return MapSpace::enwrap(isl_space_map_from_domain_and_range(takeCopy(), isl_space_align_params(isl_space_set_alloc(isl_space_get_ctx(keep()), 0, nOut), getSpace().take())));
}

ISLPP_EXSITU_ATTRS MapSpace isl::Space::mapsToItself() ISLPP_EXSITU_FUNCTION {
  assert(isSet()); return MapSpace::createMapFromDomainAndRange(*this, *this);
}

isl::SetSpace isl::Space::domain() const {
  return SetSpace::enwrap(isl_space_domain(takeCopy()));
}

isl::SetSpace isl::Space::range() const {
  return SetSpace::enwrap(isl_space_range(takeCopy()));
}

isl::ParamSpace isl::Space::params() const {
  return ParamSpace::enwrap(isl_space_params(takeCopy()));
}

isl::ParamSpace isl::Space::getParamsSpace() const {
  return Space::enwrap(isl_space_params(takeCopy()));
}

isl::SetSpace isl::Space::getDomainSpace() const {
  return SetSpace::enwrap(isl_space_domain(takeCopy()));
}

isl::SetSpace isl::Space::getRangeSpace() const {
  return SetSpace::enwrap(isl_space_range(takeCopy()));
}

isl::SetSpace isl::Space::createSetSpace(count_t nDims) const {
  assert(isParamsSpace()); return SetSpace::enwrap(isl_space_align_params(isl_space_set_alloc(getCtx()->keep(), 0, nDims), takeCopy()));
}

bool isl::Space::matchesMapSpace(Space domainSpace, Space rangeSpace) const {
  return isMap() && isl::matchesSpace(domain(), domainSpace) && isl::matchesSpace(range(), rangeSpace);
}

isl::Space isl::Space::extractTuple(isl_dim_type type) const {
  switch (type) {
  case isl_dim_param:
    return params(); //TODO: Does is return a set space?
  case isl_dim_in:
    return domain();
  case isl_dim_out:
    return range();
  case isl_dim_all:
    return copy();
  default:
    llvm_unreachable("Invalid dim type");
    return Space();
  }
}

ISLPP_DEPRECATED bool isl::Space::isNestedDomain() const {
  assert(isMapSpace()); return domain().isWrapping();
}

ISLPP_DEPRECATED bool isl::Space::isNestedRange() const {
  assert(isMapSpace()); return range().isWrapping();
}

ISLPP_DEPRECATED Space isl::Space::setNested(isl_dim_type type, const Space &nest) const {
  if (isSetSpace() && type == isl_dim_set)
    return nest;
  else if (isMapSpace() && type == isl_dim_in)
    return Space::createMapFromDomainAndRange(nest, range());
  else if (isMapSpace() && type == isl_dim_out)
    return Space::createMapFromDomainAndRange(domain(), nest);
  else
    llvm_unreachable("unsupported type");
}

ISLPP_EXSITU_ATTRS Space isl::Space::untyped() ISLPP_EXSITU_FUNCTION {
  if (isParamsSpace())
  return *this;

  if (isSetSpace()) {
    return params().createSetSpace(getSetDimCount());
  }

  assert(isMapSpace());
  return params().createMapSpace(getInDimCount(), getOutDimCount());
}


ISLPP_EXSITU_ATTRS MapSpace isl::Space::selfMap() ISLPP_EXSITU_FUNCTION {
  auto norm = normalizeWrapped();
  return createMapFromDomainAndRange(norm, norm);
}


BasicMap isl::alltoall(Space domain, Space range) {
  return BasicMap::createFromDomainAndRange(domain.universeBasicSet(), range.universeBasicSet());
}


bool isl::isEqual(const Space &space1, const Space &space2){
  return isl_space_is_equal(space1.keep(), space2.keep());
}


bool isl::isDomain(const Space &space1, const Space &space2){
  return isl_space_is_domain(space1.keep(), space2.keep());
}


bool isl::isRange(const Space &space1, const Space &space2){
  return isl_space_is_range(space1.keep(), space2.keep());
}


Space isl::join(Space &&left, Space &&right){
  return Space::enwrap(isl_space_join(left.take(), right.take()));
}


Space isl::alignParams(Space &&space1, Space &&space2){
  return Space::enwrap(isl_space_align_params(space1.take(), space2.take()));
}


Space isl::setTupleId(Space &&space, isl_dim_type type, Id &&id) {
  return Space::enwrap(isl_space_set_tuple_id(space.take(), type, id.take()));
}


Space isl::setTupleId(Space &&space, isl_dim_type type, const Id &id) {
  return Space::enwrap(isl_space_set_tuple_id(space.take(), type, id.takeCopy()));
}


Space isl::setTupleId(const Space &space, isl_dim_type type, Id &&id) {
  return Space::enwrap(isl_space_set_tuple_id(space.takeCopy(), type, id.take()));
}


Space isl::setTupleId(const Space &space, isl_dim_type type, const Id &id) {
  return Space::enwrap(isl_space_set_tuple_id(space.takeCopy(), type, id.takeCopy()));
}


void isl::compatibilize(/*inout*/Space &space1, /*inout*/Space &space2) {
  assert(space1.getCtx() == space2.getCtx());
  space1.alignParams_inplace(space2); // ensure that space1 contains the params of path spaces
  space2.alignParams_inplace(space1); // change the order of spaces of space2 to match space1
  assert(checkBool(isl_space_match(space1.keep(), isl_dim_param, space2.keep(), isl_dim_param)));
}


bool matchesTuples_recursive(const Space &lhs, const Space &rhs) {
  if (lhs.keep() == rhs.keep())
    return true;

  bool isParams = lhs.isParams();
  if (isParams != rhs.isParams())
    return false;
  if (isParams)
    return true;

  if (lhs.dim(isl_dim_out) != lhs.dim(isl_dim_out))
    return false;

  bool hasOutTupleId = lhs.hasTupleId(isl_dim_out);
  if (hasOutTupleId != rhs.hasTupleId(isl_dim_out))
    return false;

  if (hasOutTupleId) {
    if (lhs.getTupleId(isl_dim_out) != rhs.getTupleId(isl_dim_out))
      return false;
  }

  auto isMap = lhs.isMap();
  if (isMap != rhs.isMap())
    return false;

  if (isMap) {
    return matchesTuples_recursive(lhs.domain(), rhs.domain()) && matchesTuples_recursive(lhs.range(), rhs.range());
  }

  auto isWrapping = lhs.isWrapping();
  if (isWrapping != rhs.isWrapping())
    return false;

  if (isWrapping) {
    return matchesTuples_recursive(lhs.unwrap(), rhs.unwrap());
  }

  return true;
}


bool isl::matchesTuples(const Space &lhs, const Space &rhs) {
  return matchesTuples_recursive(lhs, rhs);
}


bool isl::matchesSpace(const Space &lhs, const Space &rhs) {
  assert(lhs.isValid()); assert(rhs.isValid());
  if (lhs.keep() == rhs.keep())
    return true;

  if (!matchesTuples_recursive(lhs, rhs))
    return false;

  // Matches if have different ids???
  return true;

  auto nOutDims = lhs.dim(isl_dim_out);
  assert(nOutDims == rhs.dim(isl_dim_out));
  for (auto i = nOutDims - nOutDims; i < nOutDims; i += 1) {
    bool hasDimId = lhs.hasDimId(isl_dim_out, i);
    if (hasDimId != rhs.hasDimId(isl_dim_out, i))
      return false;
    if (!hasDimId)
      continue;

    auto lhsDimId = lhs.getDimId(isl_dim_out, i);
    auto rhsDimId = rhs.getDimId(isl_dim_out, i);
    if (lhsDimId != rhsDimId)
      return false;
  }

  auto nInDims = lhs.dim(isl_dim_in);
  assert(nInDims == rhs.dim(isl_dim_in));
  for (auto i = nInDims - nInDims; i < nInDims; i += 1) {
    bool hasDimId = lhs.hasDimId(isl_dim_in, i);
    if (hasDimId != rhs.hasDimId(isl_dim_in, i))
      return false;
    if (!hasDimId)
      continue;

    auto lhsDimId = lhs.getDimId(isl_dim_in, i);
    auto rhsDimId = rhs.getDimId(isl_dim_in, i);
    if (lhsDimId != rhsDimId)
      return false;
  }

  return true;
}


isl::Space isl::operator>>(Space domainSpace, Space rangeSpace) {
  return isl::Space::createMapFromDomainAndRange(domainSpace.normalizeWrapped(), rangeSpace.normalizeWrapped());
}



