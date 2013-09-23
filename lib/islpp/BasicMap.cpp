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
  return BasicMap::enwrap(isl_basic_map_from_constraint_matrices(space.take(), eq.take(), ineq.take(), c1,c2,c3,c4,c5));
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
  auto retval = func( Constraint::enwrap(c) );
  return retval ? -1 : 0;
}
bool BasicMap::foreachConstraint(const std::function<bool(Constraint)> &func) const {
  auto retval = isl_basic_map_foreach_constraint(keep(), foreachConstraintCallback, const_cast<std::function<bool(Constraint)>*>(&func));
  return retval!=0;
}


static int enumConstraintCallback(__isl_take isl_constraint *c, void *user) {
  auto list = static_cast< std::vector<Constraint> *>(user);
  list->push_back(Constraint::enwrap(c));
  return 0;
}
std::vector<Constraint> BasicMap::getConstraints() const {
  std::vector<Constraint> result;
  //result.reserve(isl_basic_map_n_constraint (keep()));
  auto retval = isl_basic_map_foreach_constraint(keep(), enumConstraintCallback, &result);
  assert(retval==0);
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


ISLPP_EXSITU_PREFIX Aff BasicMap::dimMin(pos_t pos) ISLPP_EXSITU_QUALIFIER {
  auto pwmin = toMap().dimMin(pos);
  return pwmin.singletonAff();
}


ISLPP_EXSITU_PREFIX Aff BasicMap::dimMax(pos_t pos) ISLPP_EXSITU_QUALIFIER {
  auto pwmax = toMap().dimMax(pos);
  return pwmax.singletonAff();
}
