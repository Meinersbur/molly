#include "islpp/MultiAff.h"

#include "islpp/Printer.h"
#include <isl/map.h>
#include "islpp/BasicMap.h"
#include "islpp/Map.h"

using namespace isl;


#if 0
isl_multi_aff *Multi<Aff>::takeCopy() const {
  assert(this->maff);
  return isl_multi_aff_copy(this->maff);
}


void Multi<Aff>::give(isl_multi_aff *maff) {
  if (this->maff)
    isl_multi_aff_free(this->maff);
  this->maff = maff;
#ifndef NDEBUG
  this->_printed = toString();
#endif
}


Multi<Aff>::~Multi() {
  if (this->maff)
    isl_multi_aff_free(this->maff);
#ifndef NDEBUG
  //TODO: Ifndef NVALGRIND mark as uninitialized
  this->maff = nullptr;
#endif
}
#endif

PwMultiAff Multi<Aff>::toPwMultiAff() const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_from_multi_aff(takeCopy()));
}


void Multi<Aff>::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}

#if 0
std::string Multi<Aff>::toString() const {
  if (!maff)
    return std::string();
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}


void Multi<Aff>::dump() const { 
  isl_multi_aff_dump(keep()); 
}
#endif

void Multi<Aff>::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


void Multi<Aff>::push_back(Aff &&aff) {
  auto n = dim(isl_dim_out);

  auto list = isl_aff_list_alloc(isl_multi_aff_get_ctx(keep()), n+1);
  for (auto i = n-n; i < n; i+=1) {
    list = isl_aff_list_set_aff(list, i, isl_multi_aff_get_aff(keep(), i));
  }
  list = isl_aff_list_set_aff(list, n, aff.take());

  auto space = getSpace();
  space.addDims(isl_dim_out, n);

  give(isl_multi_aff_from_aff_list(space.take(), list));
}


// Missing in isl
static __isl_give isl_map* isl_map_from_multi_pw_aff(__isl_take isl_multi_pw_aff *mpwaff) {
  if (!mpwaff)
    return NULL;

  isl_space *space = isl_space_domain(isl_multi_pw_aff_get_space(mpwaff));
  isl_map *map = isl_map_universe(isl_space_from_domain(space));

  unsigned n = isl_multi_pw_aff_dim(mpwaff, isl_dim_out);
  for (int i = 0; i < n; ++i) {
    isl_pw_aff *pwaff = isl_multi_pw_aff_get_pw_aff(mpwaff, i); 
    isl_map *map_i = isl_map_from_pw_aff(pwaff);
    map = isl_map_flat_range_product(map, map_i);
  }

  isl_multi_pw_aff_free(mpwaff);
  return map;
}


BasicMap Multi<Aff>::toBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_from_multi_aff(takeCopy()));
}


Map Multi<Aff>::toMap() const {
  return Map::enwrap(isl_map_from_multi_aff(takeCopy()));
}


PwMultiAff Multi<Aff>::restrictDomain(Set &&set) const {
  return PwMultiAff::enwrap(isl_pw_multi_aff_alloc(set.take(), takeCopy()));
}
