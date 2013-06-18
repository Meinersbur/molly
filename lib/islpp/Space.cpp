#include "islpp/Space.h"

#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Set.h"
#include "islpp/BasicSet.h"
#include "islpp/Map.h"
#include "islpp/BasicMap.h"
#include "cstdiofile.h"
#include "islpp/MultiPwAff.h"

#include <isl/space.h>
#include <isl/set.h>
#include <isl/map.h>

using namespace isl;
using namespace std;


isl_space *Space::takeCopy() const {
  return isl_space_copy(space);
}


void Space::give(isl_space *space) {
  assert(space);
  if (this->space)
    isl_space_free(this->space);
  this->space = space;
#ifndef NDEBUG
  this->_printed = toString();
#endif
}


Space::~Space() {
  if (space) isl_space_free(space);
}


void Space::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}


std::string Space::toString() const {
  if (!keep())
    return string();  
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return stream.str();
}


void Space::dump() const {
  isl_space_dump(keep());
}


Space Space::createMapSpace(const Ctx *ctx, unsigned nparam, unsigned n_in, unsigned n_out) {
  return Space::wrap(isl_space_alloc(ctx->keep(), nparam, n_in, n_out));
}


Space Space::createParamsSpace(const Ctx *ctx, unsigned nparam) {
  return Space::wrap(isl_space_params_alloc(ctx->keep(), nparam));
}


Space Space:: createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim) {
  return Space::wrap(isl_space_set_alloc(ctx->keep(), nparam, dim));
}


Space Space::createMapFromDomainAndRange(Space &&domain, Space &&range) {
  return Space::wrap(isl_space_map_from_domain_and_range(domain.take(), range.take()));
}


Map Space::emptyMap() const {
  return enwrap(isl_map_empty(takeCopy()));
}


Map Space::universeMap() const {
  return enwrap(isl_map_universe(takeCopy()));
}


Aff  Space::createZeroAff() const {
  return enwrap(isl_aff_zero_on_domain(isl_local_space_from_space(takeCopy())));
}


Aff Space::createConstantAff(const Int &c) const {
  auto zero = createZeroAff();
  zero.setConstant(c);
  return zero;
}


Aff Space::createVarAff(isl_dim_type type, unsigned pos) const {
  return enwrap(isl_aff_var_on_domain(isl_local_space_from_space(takeCopy()), type, pos)); 
}


MultiAff Space::createZeroMultiAff() const {
  return MultiAff::wrap(isl_multi_aff_zero(takeCopy()));
}


MultiPwAff Space::createZeroMultiPwAff() const {
  return MultiPwAff::wrap(isl_multi_pw_aff_zero(takeCopy()));
}


Set Space::emptySet() const {
  assert(dim(isl_dim_in) == 0);
  return enwrap(isl_set_empty(takeCopy()));
}


Set Space::universeSet() const {
  assert(dim(isl_dim_in ) == 0);
  return enwrap(isl_set_universe(takeCopy()));
}


unsigned Space::dim(isl_dim_type type) const {
  return isl_space_dim(keep(), type);
}


unsigned Space::getParamDims() const {
  return dim(isl_dim_param);
}


unsigned Space::getSetDims() const {
  assert(isSetSpace());
  return dim(isl_dim_set);
}


unsigned Space::getInDims() const{
  assert(isMapSpace());
  return dim(isl_dim_in);
}


unsigned Space::getOutDims() const{
  assert(isMapSpace());
  return dim(isl_dim_out);
}


unsigned Space::getTotalDims() const {
  return dim(isl_dim_all);
}


bool Space::isParamsSpace() const{
  return isl_space_is_params(keep());
}
bool Space::isSetSpace() const{
  return isl_space_is_set(keep());
}
bool Space::isMapSpace() const{
  return isl_space_is_map(keep());
}



bool Space::hasDimId(isl_dim_type type, unsigned pos) const{
  return isl_space_has_dim_id(keep(), type, pos);
}
void Space::setDimId(isl_dim_type type, unsigned pos, Id &&id){
  give(isl_space_set_dim_id(take(), type, pos, id.take()));
}
Id Space::getDimId(isl_dim_type type, unsigned pos)const{
  return Id::wrap(isl_space_get_dim_id(keep(), type, pos));
}
void Space::setDimName(isl_dim_type type, unsigned pos, const char *name){
  give(isl_space_set_dim_name(take(), type, pos, name));
}
bool Space::hasDimName(isl_dim_type type, unsigned pos)const{
  return isl_space_has_dim_name(keep(), type, pos);
}
const char *Space::getDimName(isl_dim_type type, unsigned pos)const{
  return isl_space_get_dim_name(keep(), type, pos);
}

int Space::findDimById(isl_dim_type type, const Id &id) const {
  return isl_space_find_dim_by_id(keep(), type, id.keep());
}
int Space::findDimByName(isl_dim_type type, const char *name) const {
  return isl_space_find_dim_by_name(keep(), type, name);
}

void Space::setTupleId(isl_dim_type type,  Id &&id){
  give(isl_space_set_tuple_id(take(), type, id.take()));
}
void Space::resetTupleId(isl_dim_type type) {
  give(isl_space_reset_tuple_id (take(), type));
}
bool Space::hasTupleId(isl_dim_type type) const {
  return isl_space_has_tuple_id(keep(), type);
}
Id Space::getTupleId(isl_dim_type type) const {
  return Id::wrap(isl_space_get_tuple_id(keep(), type) );
}
void Space::setTupleName(isl_dim_type type, const char *s) {
  give(isl_space_set_tuple_name(take(), type, s));
}
bool Space::hasTupleName(isl_dim_type type) const{
  return isl_space_has_tuple_name(keep(), type);
}
const char *Space::getTupleName(isl_dim_type type) const {
  return isl_space_get_tuple_name(keep(), type);
}


bool Space::isWrapping() const{
  return isl_space_is_wrapping(keep());
}
void Space::wrap() {
  give(isl_space_wrap(take()));
}
void Space::unwrap() {
  give(isl_space_unwrap(take()));
}


void Space::domain(){
  give(isl_space_domain(take()));
}
void Space:: fromDomain(){
  give(isl_space_from_domain(take()));
}
void Space::range(){
  give(isl_space_range(take()));
}
void Space:: fromRange() {
  give(isl_space_from_range(take()));
}
void Space::params(){ 
  give(isl_space_params(take()));

}
void Space::setFromParams(){
  give(isl_space_set_from_params(take()));
}
void Space:: reverse(){
  give(isl_space_reverse(take()));
}

void Space::insertDims(isl_dim_type type, unsigned pos, unsigned n){
  give(isl_space_insert_dims(take(), type, pos, n));
}
void Space:: addDims(isl_dim_type type, unsigned n) {
  give(isl_space_add_dims(take(), type, n));
}
void Space::dropDims(isl_dim_type type, unsigned first, unsigned num) {
  give(isl_space_drop_dims(take(), type, first, num));
}
void Space::moveDims(isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos, unsigned n) {
  give(isl_space_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n));
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
  return enwrap(isl_set_universe(takeCopy()));
}

BasicSet Space::createUniverseBasicSet() const {
  return enwrap(isl_basic_set_universe(takeCopy()));
}

Map Space::createUniverseMap() const {
  return enwrap(isl_map_universe(takeCopy()));
}

BasicMap Space::createUniverseBasicMap() const {
  return enwrap(isl_basic_map_universe(takeCopy()));
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
  return Space::wrap(isl_space_join(left.take(), right.take()));
}
Space isl::alignParams(Space &&space1, Space &&space2){
  return Space::wrap(isl_space_align_params(space1.take(), space2.take()));
}