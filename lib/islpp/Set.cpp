#include "islpp/Set.h"

#include "islpp/BasicSet.h"
#include "islpp/cstdiofile.h"
#include "islpp/Space.h"
#include "islpp/Ctx.h"
#include "islpp/Constraint.h"
#include "islpp/Id.h"
#include "islpp/Aff.h"
#include "islpp/MultiAff.h"
#include "islpp/PwAff.h"
#include "islpp/MultiPwAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/Map.h"
#include "islpp/Point.h"
#include "islpp/PwQPolynomialFold.h"

#include <llvm/Support/raw_ostream.h>


#include <isl/set.h>
#include <isl/lp.h>
#include <isl/union_map.h>
#include <isl/map.h>
#include <isl/aff.h>
#include <isl/polynomial.h>


using namespace isl;
using namespace std;


isl_set *Set::takeCopy() const {
  assert(set); 
  return isl_set_copy(this->set);
}


void Set::give(isl_set *set) { 
  if (set)
    isl_set_free(set);
  this->set = set; 
}


Set::~Set() { 
  if (set) 
    isl_set_free(set);
}

Set::Set(BasicSet &&set) {
  this->set = isl_set_from_basic_set(set.take());
}


Set Set::createFromParams(Set &&set) {
  return Set::wrap(isl_set_from_params(set.take()));
}

Set Set::createFromPoint(Point &&point) {
  return Set::wrap(isl_set_from_point(point.take()));
}

Set Set:: createBocFromPoints(Point &&pnt1, Point &&pnt2) {
  return Set::wrap(isl_set_box_from_points(pnt1.take(), pnt2.take()));
}

Set Set::readFrom(const Ctx &ctx, FILE *input) {
  return Set::wrap(isl_set_read_from_file(ctx.keep(), input));
}

Set Set::readFrom(const Ctx &ctx, const char *str) {
  return Set::wrap(isl_set_read_from_str(ctx.keep(), str));
}


void Set::print(llvm::raw_ostream &out) const { 
  molly::CstdioFile tmp;
  isl_set_print(this->keep(), tmp.getFileDescriptor(), 0, ISL_FORMAT_ISL);
  out << tmp;
}


std::string Set::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}

void Set::dump() const { 
  print(llvm::errs());
}



Space Set::getSpace() const {
  return Space::wrap(isl_set_get_space(keep()));
}


bool Set::foreachBasicSet(BasicSetCallback fn, void *user) const {
  return isl_set_foreach_basic_set(keep(), fn, user);
}


bool Set::foreachPoint(PointCallback fn, void *user) const {
  return isl_set_foreach_point(keep(), fn, user);
}

int Set::getBasicSetCount() const {
  return isl_set_n_basic_set(keep());
}

unsigned Set::getDim(isl_dim_type type) const {
  return isl_set_dim(keep(), type);
}


int Set::getInvolvedDims(isl_dim_type type,unsigned first, unsigned n) const {
  return isl_set_involves_dims(keep(), type, first, n);
}


bool Set::dimHasAnyLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_any_lower_bound(keep(), type, pos);
}

bool Set::dimHasAnyUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_any_upper_bound(keep(), type, pos);
}

bool Set::dimHasLowerBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_lower_bound(keep(), type, pos);
}

bool Set::dimHasUpperBound(isl_dim_type type, unsigned pos) const {
  return isl_set_dim_has_upper_bound(keep(), type, pos);
}

void Set::setTupleId(Id &&id) {
  give(isl_set_set_tuple_id(take(), id.take()));
}


void Set::resetTupleId() {
  give(isl_set_reset_tuple_id(take()));
}

bool Set::hasTupleId() const {
  return isl_set_has_tuple_id(keep());
}

Id Set::getTupleId() const {
  return Id::wrap(isl_set_get_tuple_id(keep()));
}

bool Set::hasTupleName() const {
  return isl_set_has_tuple_name(keep());
}

const char *Set::getTupleName() const {
  return isl_set_get_tuple_name(keep());
}

void Set::setDimId(isl_dim_type type, unsigned pos, Id &&id) {
  give(isl_set_set_dim_id(take(), type, pos, id.take()));
}


bool Set::hasDimId(isl_dim_type type, unsigned pos) const {
  return isl_set_has_dim_id(keep(), type, pos);
}


Id Set::getDimId(isl_dim_type type, unsigned pos) const {
  return Id::wrap(isl_set_get_dim_id(keep(), type, pos));
}

int Set::findDimById(isl_dim_type type, const Id &id) const {
  return isl_set_find_dim_by_id(keep(), type, id.keep());
}

int Set::findDimByName(isl_dim_type type, const char *name) const {
  return isl_set_find_dim_by_name(keep(), type, name);
}

bool Set:: dimHasName(isl_dim_type type, unsigned pos) const {
  return isl_set_has_dim_name(keep(), type, pos);
}


const char *Set::getDimName(enum isl_dim_type type, unsigned pos) const {
  return isl_set_get_dim_name(keep(), type, pos);
}

bool Set::plainIsEmpty() const {
  return isl_set_plain_is_empty(keep());
}
bool Set::isEmpty() const {
  return isl_set_is_empty(keep());
}

bool Set::plainIsUniverse() const {
  return isl_set_plain_is_universe(keep());
}


bool Set::plainIsFixed(isl_dim_type type, unsigned pos, Int &val) const {
  return isl_set_plain_is_fixed(keep(), type, pos, val.change());
}


void Set::eliminate( isl_dim_type type, unsigned first, unsigned n) {
  give(isl_set_eliminate(take(), type, first, n));
}

void Set::fix(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_fix(take(), type, pos, value.keep()));
}

void Set::fix(isl_dim_type type, unsigned pos, int value) {
  give(isl_set_fix_si(take(), type, pos, value));
}

void Set::lowerBound(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_lower_bound(take(), type, pos, value.keep()));
}

void Set::lowerBound(isl_dim_type type, unsigned pos, signed long value) {
  give(isl_set_lower_bound_si(take(), type, pos, value));
}

void Set::upperBound(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_set_upper_bound(take(), type, pos, value.keep()));
}

void Set::upperBound(isl_dim_type type, unsigned pos, signed long value) {
  give(isl_set_upper_bound_si(take(), type, pos, value));
}


void Set::equate(isl_dim_type type1, int pos1, isl_dim_type type2, int pos2) {
  give(isl_set_equate(take(), type1, pos1, type2, pos2));
}

void Set::coalesce() {
  give(isl_set_coalesce(take()));
}

void Set::detectEqualities() {
  give(isl_set_detect_equalities(take()));
}





void Set::removeRedundancies() {
  give(isl_set_remove_redundancies(take()));
}


BasicSet isl::convexHull(Set&&set) {
  return BasicSet::wrap(isl_set_convex_hull(set.take()));
}
BasicSet isl::unshiftedSimpleHull(Set&&set){
  return  BasicSet::wrap(isl_set_unshifted_simple_hull(set.take()));
}
BasicSet isl::simpleHull(Set&&set){
  return BasicSet::wrap(isl_set_simple_hull(set.take()));
}

BasicSet isl::affineHull(Set &&set) {
  return   BasicSet::wrap(isl_set_affine_hull(set.take()));
}


BasicSet isl::polyhedralHull(Set &&set) {
  return BasicSet::wrap(isl_set_polyhedral_hull(set.take()));
}


void Set::dropContraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_set_drop_constraints_involving_dims(take(), type, first, n));
}


BasicSet isl::sample(Set &&set) {
  return  BasicSet::wrap(isl_set_sample(set.take()));
}

PwAff isl::dimMin(Set &&set, int pos) {
  return PwAff::wrap(isl_set_dim_min(set.take(), pos));
}
PwAff isl::dimMax(Set &&set, int pos) {
  return PwAff::wrap(isl_set_dim_max(set.take(), pos));
}


BasicSet isl::coefficients(Set &&set) {
  return BasicSet::wrap(isl_set_coefficients(set.take()));
}

BasicSet isl::solutions(Set &&set){
  return BasicSet::wrap(isl_set_solutions(set.take()));
}


//  Map isl::unwrap(Set &&set) {
//     return Map::wrap(isl_set_unwrap(set.take()));
//  }

void Set::flatten() {
  give(isl_set_flatten(take()));
}


Map isl::flattenMap(Set &&set) {
  return Map::wrap(isl_set_flatten_map( set.take()));
}


void Set::lift() {
  give(isl_set_lift(take()));
}

Set isl::alignParams(Set &&set, Space &&model) {
  return Set::wrap(isl_set_align_params(set.take(), model.take()));
}


Set isl::addDims(Set &&set,isl_dim_type type, unsigned n) {
  return Set::wrap(isl_set_add_dims(set.take(),type,n));
}

Set isl::insertDims(Set &&set, isl_dim_type type, unsigned pos, unsigned n){
  return Set::wrap(isl_set_insert_dims(set.take(), type, pos, n));
}
Set isl::moveDims(Set &&set, isl_dim_type dst_type, unsigned dst_pos, isl_dim_type src_type, unsigned src_pos,  unsigned n){
  return Set::wrap(isl_set_move_dims(set.take(), dst_type, dst_pos, src_type, src_pos, n));
}





Set isl::intersectParams(Set &&set, Set &&params){
  return Set::wrap(isl_set_intersect_params(set.take(), params.take()));
}
Set isl::intersect(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_intersect(set1.take(), set2.take()));
}
Set isl::union_(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_union(set1.take(), set2.take()));
}
Set isl::subtract(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_subtract (set1.take(), set2.take()));
}
Set isl::apply(Set &&set, Map &&map){
  return Set::wrap(isl_set_apply (set.take(), map.take()));
}
Set isl::preimage(Set &&set, MultiAff &&ma){
  return Set::wrap(isl_set_preimage_multi_aff (set.take(), ma.take()));
}
Set isl::preimage(Set &&set, PwMultiAff &&ma){
  return Set::wrap(isl_set_preimage_pw_multi_aff(set.take(), ma.take()));
}

Set isl::product(Set &&set1, Set &&set2){
  return Set::wrap(isl_set_product(set1.take(), set2.take()));
}
Set isl::flatProduct(Set &&set1,Set &&set2){
  return Set::wrap(isl_set_flat_product(set1.take(), set2.take()));
}
Set isl::gist(Set &&set, Set &&context){
  return Set::wrap(isl_set_gist(set.take(), context.take()));
}
Set isl::gistParams(Set &&set, Set &&context){
  return Set::wrap(isl_set_gist_params(set.take(), context.take()));
}
Set isl::partialLexmin(Set &&set, Set &&dom, Set &empty){
  return Set::wrap(isl_set_partial_lexmin(set.take(), dom.take(), empty.change()));
}
Set isl::partialLexmax(Set &&set, Set &&dom, Set &empty){
  return Set::wrap(isl_set_partial_lexmax(set.take(), dom.take(), empty.change()));
}
Set isl::lexmin(Set &&set){
  return Set::wrap(isl_set_lexmin(set.take()));
}
Set isl::lexmax(Set &&set){
  return Set::wrap(isl_set_lexmax(set.take()));
}
PwAff isl::indicatorFunction(Set &&set){
  return PwAff::wrap(isl_set_indicator_function(set.take()));
}

Point isl::samplePoint(Set &&set) {
  return Point::wrap(isl_set_sample_point(set.take()));
}

PwQPolynomialFold isl::apply(Set &&set, PwQPolynomialFold &&pwf, bool &tight) {
  int intTight;
  auto result = isl_set_apply_pw_qpolynomial_fold(set.take(), pwf.take(), &intTight);
  tight = intTight;
  return PwQPolynomialFold::wrap(result);
}


Set Set::complement() const {
  return Set::wrap(isl_set_complement(takeCopy()));
}




Set isl::complement(Set &&set) {
  return Set::wrap(isl_set_complement(set.take()));
}

Set Set::projectOut(isl_dim_type type, unsigned first, unsigned n) const {
  return Set::wrap(isl_set_project_out(takeCopy(), type, first, n));
}
Set isl::projectOut(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::wrap(isl_set_project_out(set.take(), type, first, n));
}


Set Set::params() const {
  return Set::wrap(isl_set_params(takeCopy()));
}


Set isl::params(Set &&set){
  return Set::wrap(isl_set_params(set.take()));
}


Set isl::addContraint(Set &&set, Constraint &&constraint) {
  return Set::wrap(isl_set_add_constraint(set.take(), constraint.take()));
}

Set isl::computeDivs(Set &&set) {
  return Set::wrap(isl_set_compute_divs(set.take()));
}

Set isl::alignDivs(Set &&set) {
  return Set::wrap(isl_set_align_divs(set.take()));
}

Set isl::removeDivs(Set &&set) {
  return Set::wrap(isl_set_remove_divs(set.take()));
}

Set isl::removeDivsInvolvingDims(Set &&set, isl_dim_type type, unsigned first, unsigned n) {
  return Set::wrap(isl_set_remove_divs_involving_dims(set.take(), type, first, n));
}

Set isl::removeUnknownDivs(Set &&set) {
  return Set::wrap(isl_set_remove_unknown_divs(set.take()));
}

Set isl::makeDisjoint(Set &&set) {
  return Set::wrap(isl_set_make_disjoint(set.take()));
}


bool isl::plainsIsEqual(const Set &left, const Set &right){
  return isl_set_plain_is_equal(left.keep(), right.keep());
}


bool isl::isEqual(const Set &left, const Set &right) {
  return isl_set_is_equal(left.keep(), right.keep());
}

bool isl::plainIsDisjoint(const Set &lhs, const Set &rhs) {
  return isl_set_plain_is_disjoint(lhs.keep(), rhs.keep());
}
bool isl::isDisjoint(const Set &lhs, const Set &rhs) {
  return isl_set_is_disjoint(lhs.keep(), rhs.keep());
}


bool isl::isSubset(const Set &lhs, const Set &rhs) {
  return isl_set_is_subset(lhs.keep(), rhs.keep());
}
bool isl::isStrictSubset(const Set &lhs, const Set &rhs) {
  return isl_set_is_strict_subset(lhs.keep(), rhs.keep());
}

int isl::plainCmp(const Set &lhs, const Set &rhs) {
  return isl_set_plain_cmp(lhs.keep(), rhs.keep());
}
