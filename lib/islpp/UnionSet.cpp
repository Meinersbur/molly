#include "islpp_impl_common.h"
#include "islpp/UnionSet.h"

#include "islpp/Printer.h"
#include "islpp/Point.h"
#include "islpp/UnionMap.h"


void UnionSet::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


UnionSet UnionSet::apply(const UnionMap &umap) const { 
  return UnionSet::enwrap(isl_union_set_apply(takeCopy(), umap.takeCopy()));
}



static int foreachSetCallback(__isl_take isl_set *set, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Set)>*>(user);
  auto retval = func(Set::enwrap(set));
  return retval ? -1 : 0;
}
bool UnionSet::foreachSet(const std::function<bool(isl::Set)> &func) const {
  auto retval = isl_union_set_foreach_set(keep(), foreachSetCallback, (void*)&func);
  return retval!=0;
}


static int enumSetCallback(__isl_take isl_set *set, void *user) {
  std::vector<Set> * list = static_cast< std::vector<Set> *>(user);
  list->push_back(Set::enwrap(set));
  return 0;
}
std::vector<Set> UnionSet::getSets() const {
  std::vector<Set> result(isl_union_set_n_set(keep()));
  auto retval = isl_union_set_foreach_set(keep(), enumSetCallback, &result);
  assert(retval==0);
  return result;
}


static int foreachPointCallback(__isl_take isl_point *point, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(Point)>*>(user);
  auto retval = func(Point::enwrap(point));
  return retval ? -1 : 0;
}
bool UnionSet::foreachPoint(const std::function<bool(isl::Point)> &func) const {
  auto retval = isl_union_set_foreach_point(keep(), foreachPointCallback, (void*)&func);
  return retval!=0;
}


UnionMap UnionSet::lt(const UnionSet &uset2) const { 
  return UnionMap::enwrap(isl_union_set_lex_lt_union_set(takeCopy(), uset2.takeCopy())); 
}


UnionMap UnionSet::le(const UnionSet &uset2) const {
  return UnionMap::enwrap(isl_union_set_lex_le_union_set(takeCopy(), uset2.takeCopy()));
}


UnionMap UnionSet::gt(const UnionSet &uset2) const { 
  return UnionMap::enwrap(isl_union_set_lex_gt_union_set(takeCopy(), uset2.takeCopy()));
}


UnionMap UnionSet::ge(const UnionSet &uset2) const {
  return UnionMap::enwrap(isl_union_set_lex_ge_union_set(takeCopy(), uset2.takeCopy()));
}
