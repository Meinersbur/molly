#include "islpp_impl_common.h"
#include "islpp/PwAffList.h"

#include "islpp/Printer.h"

using namespace isl;


void PwAffList::print(llvm::raw_ostream &out) const {
  Printer printer = Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


static int foreachPwAffCallback(__isl_take isl_pw_aff *paff, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(PwAff)> *>(user);
  auto retval = func( PwAff::enwrap(paff) );
  return retval ? -1 : 0;
}
bool PwAffList::foreachPwAff(std::function<bool(PwAff)> func) const {
  auto retval = isl_pw_aff_list_foreach(keep(), foreachPwAffCallback, &func);
  return retval!=0;
}


static int enumPwAffCallback(__isl_take isl_pw_aff *paff, void *user) {
  auto list = static_cast< std::vector<PwAff> *>(user);
  list->push_back(PwAff::enwrap(paff));
  return 0;
}
std::vector<PwAff> PwAffList::getPwAffs() const {
  std::vector<PwAff> result;
  result.reserve(isl_pw_aff_list_n_pw_aff(keep()) );
  auto retval = isl_pw_aff_list_foreach(keep(), enumPwAffCallback, &result);
  assert(retval==0);
  return result;
}

static int sortPwAffCallback(__isl_keep isl_pw_aff *el1, __isl_keep isl_pw_aff *el2, void *user) {
  auto &func = *static_cast<const std::function<int(const PwAff&, const PwAff&)>*>(user);
  auto retval = func( PwAff::enwrapCopy(el1), PwAff::enwrapCopy(el2) );
  return retval ? -1 : 0;
}
void PwAffList::sort_inplace(const std::function<int(const PwAff&, const PwAff&)> &func) ISLPP_INPLACE_QUALIFIER {
  give(isl_pw_aff_list_sort(take(), sortPwAffCallback, const_cast<std::function<int(const PwAff&, const PwAff&)>*>(&func)));
}

static int foreachSccPwAffCallback(__isl_take isl_pw_aff_list *scc, void *user) {
  assert(user);
  auto &func = *static_cast<const std::function<bool(PwAffList)>*>(user);
  auto retval = func( PwAffList::enwrap(scc) );
  return retval ? -1 : 0;
}
static int foreachSccPwAffFollows(__isl_keep isl_pw_aff *a, __isl_keep isl_pw_aff *b, void *user) {
  assert(user);
  auto &func = *static_cast<std::function<bool(const PwAff &, const PwAff &)>*>(user);
  auto retval = func( PwAff::enwrapCopy(a), PwAff::enwrapCopy(b) );
  return retval ? 1 : 0;
}
bool PwAffList::foreachScc(const std::function<bool(const PwAff &, const PwAff &)> &followsFunc, const std::function<bool(PwAffList)> &func) const {
  auto retval = isl_pw_aff_list_foreach_scc(keep(), 
    foreachSccPwAffFollows, const_cast<std::function<bool(const PwAff &, const PwAff &)>*>(&followsFunc),
    foreachSccPwAffCallback, const_cast<std::function<bool(PwAffList)>*>(&func)
    );
  return retval!=0;
}
