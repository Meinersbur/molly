#include "islpp/PwAff.h"

#include "islpp/Ctx.h"
#include "islpp/Aff.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Set.h"
#include "islpp/Printer.h"
#include "islpp/Id.h"
#include "islpp/MultiAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/Map.h"

#include <isl/aff.h>
#include <llvm/Support/raw_ostream.h>

using namespace isl;
using namespace std;
using namespace llvm;


PwAff PwAff::createFromAff(Aff &&aff){
  return PwAff::enwrap(isl_pw_aff_from_aff(aff.take()));
}
PwAff PwAff::createEmpty(Space &&space) {
  return PwAff::enwrap(isl_pw_aff_empty(space.take()));
}
PwAff PwAff::create(Set &&set, Aff &&aff) {
  return PwAff::enwrap(isl_pw_aff_alloc(set.take(), aff.take()));
}
PwAff PwAff::createZeroOnDomain(LocalSpace &&space) {
  return PwAff::enwrap(isl_pw_aff_zero_on_domain(space.take()));
}
PwAff PwAff::createVarOnDomain(LocalSpace &&ls, isl_dim_type type, unsigned pos) {
  return PwAff::enwrap(isl_pw_aff_var_on_domain(ls.take(), type, pos));
}
PwAff PwAff::createIndicatorFunction(Set &&set) {
  return PwAff::enwrap(isl_set_indicator_function(set.take()));
}


PwAff PwAff::readFromStr(Ctx *ctx, const char *str) {
  return PwAff::enwrap(isl_pw_aff_read_from_str(ctx->keep(), str));
}


 Map PwAff::toMap() const { 
   return Map::enwrap(isl_map_from_pw_aff(takeCopy())) ;
 }


void PwAff::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}


Space PwAff::getDomainSpace() const {
  return Space::enwrap(isl_pw_aff_get_domain_space(keep()));
}


bool PwAff::isEmpty() const {
  return isl_pw_aff_is_empty(keep());
}


bool PwAff::involvesDim(isl_dim_type type, unsigned first, unsigned n) const {
  return isl_pw_aff_involves_dims(keep(), type, first, n);
}
bool PwAff::isCst() const {
  return isl_pw_aff_is_cst(keep());
}

void PwAff::alignParams(Space &&model) {
  give(isl_pw_aff_align_params(take(), model.take()));
}


void PwAff::neg() {
  give(isl_pw_aff_neg(take()));
}
void PwAff::ceil(){
  give(isl_pw_aff_ceil(take()));
}
void PwAff::floor(){
  give(isl_pw_aff_floor(take()));
}
void PwAff::mod(const Int &mod) {
  give(isl_pw_aff_mod(take(), mod.keep()));
}

void PwAff::intersectParams(Set &&set) {
  give(isl_pw_aff_intersect_params(take(), set.take()));
}
void PwAff::intersetDomain(Set &&set) {
  give(isl_pw_aff_intersect_domain(take(), set.take()));
}

void PwAff::scale(const Int &f) {
  give(isl_pw_aff_scale(take(), f.keep()));
}
void PwAff::scaleDown(const Int &f) {
  give(isl_pw_aff_scale_down(take(), f.keep()));
}

void PwAff::insertDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_pw_aff_insert_dims(take(), type, first, n));
}
void PwAff::addDims(isl_dim_type type, unsigned n){
  give(isl_pw_aff_add_dims(take(), type, n));
}
void PwAff::dropDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_pw_aff_drop_dims(take(), type, first, n));
}

void PwAff::coalesce(){
  give(isl_pw_aff_coalesce(take()));
}
void PwAff::gist(Set &&context) {
  give(isl_pw_aff_gist(take(), context.take()));
}
void PwAff::gistParams(Set &&context) {
  give(isl_pw_aff_gist_params(take(), context.take()));
}

void PwAff::pullback(MultiAff &&ma) {
  give(isl_pw_aff_pullback_multi_aff(take(), ma.take()));
}
void PwAff::pullback(PwMultiAff &&pma) {
  give(isl_pw_aff_pullback_pw_multi_aff(take(), pma.take()));
}

int PwAff::nPiece() const {
  return isl_pw_aff_n_piece(keep());
}

static int piececallback(isl_set *set, isl_aff *aff, void *user) {
  auto fn = *static_cast<std::function<bool(Set,Aff)>*>(user);
  auto retval = fn(Set::enwrap(set), Aff::enwrap(aff));
  return retval ? -1 : 0;
}
bool PwAff::foreachPiece(std::function<bool(Set,Aff)> fn) const {
  auto retval = isl_pw_aff_foreach_piece(keep(), piececallback, &fn);
  return (retval!=0);
}


static int enumPiecesCallback(__isl_take isl_set *set, __isl_take isl_aff *aff, void *user) {
  auto list = static_cast< std::vector<std::pair<Set,Aff>> *>(user);
  list->push_back(std::make_pair(Set::enwrap(set), Aff::enwrap(aff)));
  return 0;
}
std::vector<std::pair<Set,Aff>> PwAff::getPieces() const {
  std::vector<std::pair<Set,Aff>> result(isl_pw_aff_n_piece(keep()));
  auto retval = isl_pw_aff_foreach_piece(keep(), enumPiecesCallback, &result);
  assert(retval==0);
  return result;
}


bool isl:: plainIsEqual(PwAff pwaff1, PwAff pwaff2){
  return isl_pw_aff_plain_is_equal(pwaff1.take(), pwaff2.take());
}

PwAff isl::unionMin(PwAff &&pwaff1, PwAff &&pwaff2) {
  return PwAff::enwrap(isl_pw_aff_union_min(pwaff1.take(), pwaff2.take()));
}
PwAff isl::unionMax(PwAff &&pwaff1, PwAff &&pwaff2){
  return PwAff::enwrap(isl_pw_aff_union_max(pwaff1.take(), pwaff2.take()));
}
PwAff isl::unionAdd(PwAff &&pwaff1, PwAff &&pwaff2){
  return PwAff::enwrap(isl_pw_aff_union_add(pwaff1.take(), pwaff2.take()));
}

Set isl::domain(PwAff &&pwaff) {
  return Set::enwrap(isl_pw_aff_domain(pwaff.take()));
}

PwAff isl::min(PwAff &&pwaff1, PwAff &&pwaff2) {
  return PwAff::enwrap(isl_pw_aff_min(pwaff1.take(), pwaff2.take()));
}
PwAff isl::max(PwAff &&pwaff1, PwAff &&pwaff2){
  return PwAff::enwrap(isl_pw_aff_max(pwaff1.take(), pwaff2.take()));
}
PwAff isl::mul(PwAff &&pwaff1, PwAff &&pwaff2){
  return PwAff::enwrap(isl_pw_aff_mul(pwaff1.take(), pwaff2.take()));
}
PwAff isl::div(PwAff &&pwaff1, PwAff &&pwaff2){
  return PwAff::enwrap(isl_pw_aff_div(pwaff1.take(), pwaff2.take()));
}


PwAff isl::add(PwAff &&lhs, int rhs) {
  auto zero = isl_aff_zero_on_domain(isl_local_space_from_space(isl_pw_aff_get_domain_space(lhs.keep())));
  auto cst = isl_aff_set_constant_si(zero, rhs);
  return PwAff::enwrap(isl_pw_aff_add(lhs.take(), isl_pw_aff_from_aff(cst)));
}


PwAff isl::sub(PwAff &&pwaff1, PwAff &&pwaff2) {
  return PwAff::enwrap(isl_pw_aff_sub(pwaff1.take(), pwaff2.take()));
}
PwAff isl::sub(const PwAff &pwaff1, PwAff &&pwaff2) {
  return PwAff::enwrap(isl_pw_aff_sub(pwaff1.takeCopy(), pwaff2.take()));
}
PwAff isl::sub(PwAff &&pwaff1, const PwAff &pwaff2) {
  return PwAff::enwrap(isl_pw_aff_sub(pwaff1.take(), pwaff2.takeCopy()));
}
PwAff isl::sub(const PwAff &pwaff1,const  PwAff &pwaff2) {
  return PwAff::enwrap(isl_pw_aff_sub(pwaff1.takeCopy(), pwaff2.takeCopy()));
}
PwAff isl::sub(PwAff &&lhs, int rhs) {
  auto zero = isl_aff_zero_on_domain(isl_local_space_from_space(isl_pw_aff_get_domain_space(lhs.keep())));
  auto cst = isl_aff_set_constant_si(zero, rhs);
  return PwAff::enwrap(isl_pw_aff_sub(lhs.take(), isl_pw_aff_from_aff(cst)));
}


PwAff isl::tdivQ(PwAff &&pa1, PwAff &&pa2){
  return PwAff::enwrap(isl_pw_aff_tdiv_q(pa1.take(), pa2.take()));
}
PwAff isl::tdivR(PwAff &&pa1, PwAff &&pa2){
  return PwAff::enwrap(isl_pw_aff_tdiv_r(pa1.take(), pa2.take()));
}

PwAff isl::cond(PwAff &&cond, PwAff &&pwaff_true, PwAff &&pwaff_false) {
  return PwAff::enwrap(isl_pw_aff_cond(cond.take(), pwaff_true.take(), pwaff_false.take()));
}


Set isl::nonnegSet(PwAff &pwaff){
  return Set::enwrap(isl_pw_aff_nonneg_set(pwaff.take()));
}
Set isl::zeroSet(PwAff &pwaff){
  return Set::enwrap(isl_pw_aff_zero_set(pwaff.take()));
}
Set isl::nonXeroSet(PwAff &pwaff){
  return Set::enwrap(isl_pw_aff_non_zero_set(pwaff.take()));
}

Set isl::eqSet(PwAff &&pwaff1, PwAff &&pwaff2) {
  return Set::enwrap(isl_pw_aff_eq_set(pwaff1.take(), pwaff2.take()));
}

Set isl::neSet(PwAff &&pwaff1, PwAff &&pwaff2){
  return Set::enwrap(isl_pw_aff_ne_set(pwaff1.take(), pwaff2.take()));
}
Set isl::leSet(PwAff &&pwaff1, PwAff &&pwaff2){
  return Set::enwrap(isl_pw_aff_le_set(pwaff1.take(), pwaff2.take()));
}
Set isl::ltSet(PwAff &&pwaff1, PwAff &&pwaff2){
  return Set::enwrap(isl_pw_aff_lt_set(pwaff1.take(), pwaff2.take()));
}
Set isl::geSet(PwAff &&pwaff1, PwAff &&pwaff2){
  return Set::enwrap(isl_pw_aff_ge_set(pwaff1.take(), pwaff2.take()));
}
Set isl::gtSet(PwAff &&pwaff1, PwAff &&pwaff2){
  return Set::enwrap(isl_pw_aff_gt_set(pwaff1.take(), pwaff2.take()));
}
