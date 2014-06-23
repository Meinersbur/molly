#include "islpp_impl_common.h"
#include "islpp/BasicMap.h"

#include "islpp/Map.h"
#include <isl/map.h>
#include <llvm/Support/raw_ostream.h>

using namespace isl;



void BasicMap::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


void BasicMap::dump() const {
  isl_basic_map_dump(keep());
}


BasicMap BasicMap::createEmptyLikeMap(Map &&model) {
  return BasicMap::enwrap(isl_basic_map_empty_like_map(model.take()));
}


BasicMap BasicMap::createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4, isl_dim_type c5) {
  return BasicMap::enwrap(isl_basic_map_from_constraint_matrices(space.take(), eq.take(), ineq.take(), c1, c2, c3, c4, c5));
}


Map BasicMap::intersect(Map &&that) const {
  return Map::enwrap(isl_map_intersect(isl_map_from_basic_map(takeCopy()), that.take()));
}


Map BasicMap::intersect(const Map &that) const {
  return Map::enwrap(isl_map_intersect(isl_map_from_basic_map(takeCopy()), that.takeCopy()));
}


Map BasicMap::intersectDomain(const Set &set) const {
  return Map::enwrap(isl_map_intersect_domain(isl_map_from_basic_map(takeCopy()), set.takeCopy()));
}


Map BasicMap::intersectRange(const Set &set) const {
  return Map::enwrap(isl_map_intersect_range(isl_map_from_basic_map(takeCopy()), set.takeCopy()));
}


Map isl::partialLexmax(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
  isl_set *rawempty = 0;
  auto result = Map::enwrap(isl_basic_map_partial_lexmax(bmap.take(), dom.take(), &rawempty));
  empty = Set::enwrap(rawempty);
  return result;
}


Map isl::partialLexmin(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
  isl_set *rawempty = 0;
  auto result = Map::enwrap(isl_basic_map_partial_lexmin(bmap.take(), dom.take(), &rawempty));
  empty = Set::enwrap(rawempty);
  return result;
}


Map isl::lexmin(BasicMap &&bmap) {
  return Map::enwrap(isl_basic_map_lexmin(bmap.take()));
}


Map isl::lexmax(BasicMap &&bmap) {
  return Map::enwrap(isl_basic_map_lexmax(bmap.take()));
}

PwMultiAff isl::partialLexminPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
  isl_set *rawempty = 0;
  auto result = enwrap(isl_basic_map_partial_lexmin_pw_multi_aff(bmap.take(), dom.take(), &rawempty));
  empty = Set::enwrap(rawempty);
  return result;
}
PwMultiAff isl::partialLexmaxPwMultiAff(BasicMap &&bmap, BasicSet &&dom, Set &empty) {
  isl_set *rawempty = 0;
  auto result = enwrap(isl_basic_map_partial_lexmax_pw_multi_aff(bmap.take(), dom.take(), &rawempty));
  empty = Set::enwrap(rawempty);
  return result;
}


PwMultiAff isl::lexminPwMultiAff(BasicMap &&bmap) { return enwrap(isl_basic_map_lexmin_pw_multi_aff(bmap.take())); }
PwMultiAff isl::lexmaxPwMultiAff(BasicMap &&bmap) { return enwrap(isl_basic_map_lexmax_pw_multi_aff(bmap.take())); }


Map isl::unite(BasicMap &&bmap1, BasicMap &&bmap2) {
  return Map::enwrap(isl_basic_map_union(bmap1.take(), bmap2.take()));
}


Map isl::computeDivs(BasicMap &&bmap) {
  return enwrap(isl_basic_map_compute_divs(bmap.take()));
}


static int foreachConstraintCallback(__isl_take isl_constraint *c, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Constraint)>*>(user);
  auto retval = func(Constraint::enwrap(c));
  return retval ? -1 : 0;
}
bool BasicMap::foreachConstraint(const std::function<bool(Constraint)> &func) const {
  auto retval = isl_basic_map_foreach_constraint(keep(), foreachConstraintCallback, const_cast<std::function<bool(Constraint)>*>(&func));
  return retval != 0;
}


static int enumConstraintCallback(__isl_take isl_constraint *c, void *user) {
  auto list = static_cast<std::vector<Constraint> *>(user);
  list->push_back(Constraint::enwrap(c));
  return 0;
}
std::vector<Constraint> BasicMap::getConstraints() const {
  std::vector<Constraint> result;
  //result.reserve(isl_basic_map_n_constraint (keep()));
  auto retval = isl_basic_map_foreach_constraint(keep(), enumConstraintCallback, &result);
  assert(retval == 0);
  return result;
}


Map BasicMap::domainProduct(const Map &that) const {
  return Map::enwrap(isl_map_domain_product(isl_map_from_basic_map(this->takeCopy()), that.takeCopy()));
}


Map BasicMap::rangeProduct(const Map &that) const {
  return Map::enwrap(isl_map_range_product(isl_map_from_basic_map(this->takeCopy()), that.takeCopy()));
}


Map BasicMap::applyDomain(const Map &that) const {
  return Map::enwrap(isl_map_apply_domain(isl_map_from_basic_map(takeCopy()), that.takeCopy()));
}


Map BasicMap::applyRange(const Map &that) const {
  return Map::enwrap(isl_map_apply_domain(isl_map_from_basic_map(takeCopy()), that.takeCopy()));
}


ISLPP_EXSITU_ATTRS Aff BasicMap::dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION{
  auto pwmin = toMap().dimMin(pos);
  return pwmin.singletonAff();
}


ISLPP_EXSITU_ATTRS Aff BasicMap::dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION{
  auto pwmax = toMap().dimMax(pos);
  return pwmax.singletonAff();
}


void isl::BasicMap::cast_inplace(Space space) ISLPP_INPLACE_FUNCTION{
  auto domainMap = Space::createMapFromDomainAndRange(getDomainSpace(), space.domain()).equalBasicMap();
  auto rangeMap = Space::createMapFromDomainAndRange(getRangeSpace(), space.range()).equalBasicMap();
  applyDomain_inplace(domainMap.move());
  applyRange_inplace(rangeMap.move());
}


ISLPP_PROJECTION_ATTRS int isl::BasicMap::getComplexity() ISLPP_PROJECTION_FUNCTION{
  auto eqMat = Mat::enwrap(isl_basic_map_equalities_matrix(keep(), isl_dim_cst, isl_dim_param, isl_dim_in, isl_dim_out, isl_dim_div));
  auto ineqMat = Mat::enwrap(isl_basic_map_inequalities_matrix(keep(), isl_dim_cst, isl_dim_param, isl_dim_in, isl_dim_out, isl_dim_div));
  assert(eqMat.cols() == ineqMat.cols());
  auto neqs = eqMat.rows();
  auto ineqs = ineqMat.rows();

  int complexity = eqMat.cols() + 2 * ineqMat.cols();
  auto nDivDims = dim(isl_dim_div);
  auto nTotalDims = eqMat.cols();
  for (auto i = nTotalDims - nDivDims; i < nTotalDims; i += 1) {
    for (auto j = neqs - neqs; j < neqs; j += 1) {
      if (!eqMat[j][i].isZero())
        complexity += 1;
    }
    for (auto j = ineqs - ineqs; j < ineqs; j += 1) {
      if (!ineqMat[j][i].isZero())
        complexity += 2;
    }
  }

  return complexity;
}


void isl::BasicMap::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted /*= false*/) const{
  toMap().printExplicit(os, maxElts, newlines, formatted, sorted);
}


void isl::BasicMap::dumpExplicit(int maxElts, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted /*= false*/) const{
  toMap().dumpExplicit(maxElts, newlines, formatted, sorted);
}


void isl::BasicMap::dumpExplicit() const{
  toMap().dumpExplicit();
}


std::string isl::BasicMap::toStringExplicit(int maxElts /*= 8*/, bool newlines /*= false*/, bool formatted /*= false*/, bool sorted /*= false*/){
  return toMap().toStringExplicit(maxElts, newlines, formatted, sorted);
}
