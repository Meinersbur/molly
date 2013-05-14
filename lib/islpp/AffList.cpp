#include "islpp/AffList.h"

using namespace isl;

static int foreachAffCallback(__isl_take isl_aff *el, void *user) {
  assert(user);
  auto &func = *static_cast<const std::function<bool(Aff&&)>*>(user);
  auto retval = func( Aff::wrap(el) );
  return retval ? -1 : 0;
}
bool List<Aff>::foreach(const std::function<bool(Aff &&)> &func) {
  auto retval = isl_aff_list_foreach(keep(), foreachAffCallback, const_cast<std::function<bool(Aff&&)>*>(&func));
  return retval!=0;
}

static int foreachSccAffCallback(__isl_take isl_aff_list *scc, void *user) {
  assert(user);
  auto &func = *static_cast<const std::function<bool(List<Aff>&&)>*>(user);
  auto retval = func( List<Aff>::wrap(scc) );
  return retval ? -1 : 0;
}
static int foreachSccAffFollows(__isl_keep isl_aff *a, __isl_keep isl_aff *b, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(const Aff &, const Aff &)>*>(user);
  auto retval = func( Aff::wrapCopy(a), Aff::wrapCopy(b) );
  return retval ? 1 : 0;
}
bool List<Aff>::foreachScc(const std::function<bool(const Aff &, const Aff &)> &followsFunc, const std::function<bool(List<Aff> &&)> &func) {
  auto retval = isl_aff_list_foreach_scc(keep(), 
    foreachSccAffFollows, const_cast<std::function<bool(const Aff &, const Aff &)>*>(&followsFunc),
    foreachSccAffCallback, const_cast<std::function<bool(List<Aff> &&)>*>(&func)
    );
  return retval!=0;
}

static int sortAffCallback(__isl_keep isl_aff *el1, __isl_keep isl_aff *el2, void *user) {
  auto &func = *static_cast<const std::function<int(const Aff&, const Aff&)>*>(user);
  auto retval = func( Aff::wrapCopy(el1), Aff::wrapCopy(el2) );
  return retval ? -1 : 0;
}
void List<Aff>::sort(const std::function<int(const Aff&, const Aff&)> &func) {
  give(isl_aff_list_sort(take(), &sortAffCallback, const_cast<std::function<int(const Aff&, const Aff&)>*>(&func)));
}
