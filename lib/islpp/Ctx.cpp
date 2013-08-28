#include "islpp/Ctx.h"

#include "islpp/BasicSet.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Constraint.h"
#include "islpp/Printer.h"
#include "islpp/Map.h"
#include "islpp/BasicMap.h"
#include "islpp/Aff.h"
#include "islpp/Id.h"
#include "islpp/UnionMap.h"


#include <isl/ctx.h>
#include <isl/options.h>
#include <isl/space.h>
#include <isl/set.h>
#include <isl/printer.h>

using namespace isl;
using namespace std;


void Ctx:: operator delete(void* ptr) {
  isl_ctx *ctx = reinterpret_cast<isl_ctx*>(ptr); 
  if (ctx)
    isl_ctx_free(ctx);
}

Ctx::~Ctx() {
  return;
  isl_ctx *ctx = reinterpret_cast<isl_ctx*>(this); 
  if (ctx)
    isl_ctx_free(ctx);
}

Ctx *Ctx::create() {
  auto result = Ctx::enwrap(isl_ctx_alloc());
#ifndef NDEBUG
  result->setOnError(OnErrorEnum::Abort);
#endif
  return result;
}


isl_error Ctx::getLastError() const {
  return isl_ctx_last_error(keep());
}


void Ctx::resetLastError() {
  isl_ctx_reset_error(keep());
}


void Ctx::setOnError(OnErrorEnum val){
  isl_options_set_on_error(keep(), static_cast<int> (val));
}


OnErrorEnum Ctx::getOnError() const {
  return static_cast<OnErrorEnum>(isl_options_get_on_error(keep()));
}


Printer Ctx::createPrinterToFile(FILE *file) {
  return Printer::wrap(isl_printer_to_file(keep(), file), false);
}


Printer Ctx:: createPrinterToFile(const char *filename) {
  FILE *file = fopen(filename, "w");
  assert(file);
  Printer result = Printer::wrap(isl_printer_to_file(keep(), file), true);
  return result;
}


Printer Ctx::createPrinterToStr(){
  return Printer::wrap(isl_printer_to_str(keep()), false);
}


Space Ctx::createMapSpace(unsigned nparam/*params*/, unsigned n_in/*domain*/, unsigned n_out/*range*/) {
  return Space::createMapSpace(this, nparam, n_in, n_out);
}


Space Ctx::createMapSpace(Space &&domain, Space &&range) {
  return Space::createMapFromDomainAndRange(domain.move(), range.move());
}


Space Ctx::createMapSpace(Space &&domain, unsigned n_out) {
  return Space::enwrap(isl_space_map_from_domain_and_range(domain.take(), isl_space_set_alloc(keep(), 0, n_out )));
}


Space Ctx::createMapSpace(const Space &domain, unsigned n_out) { return createMapSpace(copy(domain), n_out); }


Space Ctx::createMapSpace(unsigned n_in, Space &&range) {
  return Space::enwrap(isl_space_map_from_domain_and_range(isl_space_set_alloc(keep(), 0, n_in), range.take()));
}


Space Ctx::createMapSpace(unsigned n_in, const Space &range) { return createMapSpace(n_in, copy(range)); }


Space Ctx::createParamsSpace(unsigned nparam) {
  return Space::createParamsSpace(this, nparam);
}


Space Ctx::createSetSpace(unsigned nparam, unsigned dim) {
  return Space::createSetSpace(this, nparam, dim);
}


Aff Ctx::createZeroAff(LocalSpace &&space) {
  return Aff::enwrap(isl_aff_zero_on_domain(space.take()));
}


BasicSet Ctx::createRectangularSet(const llvm::SmallVectorImpl<unsigned> &lengths) {
  auto dims = lengths.size();
  Space space = createSetSpace(0, dims);
  BasicSet set = BasicSet::create(space.copy());

  for (auto d = dims-dims; d < dims; d+=1) {
    auto ge = Constraint::createInequality(space.copy());
    ge.setConstant_inplace(0);
    ge.setCoefficient_inplace(isl_dim_set, 0, 1);
    set.addConstraint(move(ge));

    auto lt = Constraint::createInequality(move(space));
    lt.setConstant_inplace(lengths[d]);
    lt.setCoefficient_inplace(isl_dim_set, 0, -1);
    set.addConstraint(move(lt));
  }
  return set;
}


BasicSet Ctx::readBasicSet(const char *str) {
  return BasicSet::wrap(isl_basic_set_read_from_str(keep(), str));
}


Map Ctx::createMap(unsigned nparam, unsigned in, unsigned out, int n, unsigned flags) {
  return Map::create(this, nparam, in, out, n, flags);
}


Map Ctx::createUniverseMap(Space &&space) {
  return Map::createUniverse(move(space));
}


Map Ctx::createAlltoallMap(Set &&domain, Set &&range) {
  auto resultspace = Space::createMapFromDomainAndRange(domain.getSpace(), range.getSpace());
  auto result = resultspace.createUniverseMap();
  result = intersectDomain(result.move(), domain.move());
  result = intersectRange(result.move(), range.move());
  return result;
}

Map Ctx::createAlltoallMap(const Set &domain, Set &&range) { return createAlltoallMap(domain.copy(), range.move()); }
Map Ctx::createAlltoallMap(Set &&domain, const Set &range) { return createAlltoallMap(domain.move(), range.copy()); }
Map Ctx::createAlltoallMap(const Set &domain, const Set &range) { return createAlltoallMap(domain.copy(), range.copy()); }


Map Ctx::createEmptyMap(Space &&space) {
  assert(isl_space_get_ctx(space.keep()) == keep());
  return Map::enwrap(isl_map_empty(space.take()));
}


Map Ctx::createEmptyMap(Space &&domainSpace, Space &&rangeSpace) {
  assert(domainSpace.getCtx()->keep() == keep());
  assert(rangeSpace.getCtx()->keep() == keep());
  auto mapspace = isl_space_map_from_domain_and_range(domainSpace.take(), rangeSpace.take());
  return Map::enwrap(isl_map_empty(mapspace));
}


Map Ctx::createEmptyMap(const Space &domainSpace, Space &&rangeSpace) {
  assert(domainSpace.getCtx()->keep() == keep());
  assert(rangeSpace.getCtx()->keep() == keep());
  auto mapspace = isl_space_map_from_domain_and_range(domainSpace.takeCopy(), rangeSpace.take());
  return Map::enwrap(isl_map_empty(mapspace));
}


Map Ctx::createEmptyMap(const BasicSet &domain, Space &&rangeSpace) {
  auto mapspace = isl_space_map_from_domain_and_range(domain.getSpace().take(), rangeSpace.take());
  return Map::enwrap(isl_map_empty(mapspace));
}


Map Ctx::createEmptyMap(const BasicSet &domain, const Set &range) {
  auto mapspace = isl_space_map_from_domain_and_range(domain.getSpace().take(), range.getSpace().take());
  return Map::enwrap(isl_map_empty(mapspace));
}


Map Ctx::createEmptyMap(Set &&domain, Set &&range) {
  auto mapspace = isl_space_map_from_domain_and_range(isl_set_get_space(domain.keep()), isl_set_get_space(range.keep()));
  return Map::enwrap(isl_map_empty(mapspace));
}


BasicMap Ctx::createBasicMap(unsigned nparam, unsigned in, unsigned out, unsigned extra, unsigned n_eq, unsigned n_ineq) {
  return BasicMap::enwrap(isl_basic_map_alloc(keep(), nparam, in, out, extra, n_eq, n_ineq));
}


BasicMap Ctx::createUniverseBasicMap(Space &&space) {
  assert(space.getCtx() == this);
  return BasicMap::enwrap(isl_basic_map_universe(space.take()));
}


UnionMap Ctx::createEmptyUnionMap() {
  return UnionMap::enwrap(isl_union_map_empty(isl_space_alloc(keep(), 0,0,0)));
}


UnionSet Ctx::createEmptyUnionSet() {
  return UnionSet::enwrap(isl_union_set_empty(isl_space_alloc(keep(),0,0,0)));
}


Id Ctx::createId(const char *name, const void *user) {
  //TODO: Check for invalid characters in name 
  return Id::enwrap(isl_id_alloc(keep(), name, const_cast<void*>(user)));
}


Id Ctx::createId(const std::string &name, const void *user ){
  return Id::enwrap(isl_id_alloc(keep(), name.c_str(), const_cast<void*>(user)));
}


Id Ctx::createId(llvm::StringRef name, const void *user) {
  return Id::enwrap(isl_id_alloc(keep(), name.str().c_str(), const_cast<void*>(user)));
}


Id Ctx::createId(const llvm::Twine& name, const void *user){
  return Id::enwrap(isl_id_alloc(keep(), name.str().c_str(), const_cast<void*>(user)));
}
