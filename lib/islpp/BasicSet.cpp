#include "islpp/BasicSet.h"

#include "islpp/Ctx.h"
#include "islpp/Constraint.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Mat.h"
#include "islpp/Id.h"
#include "islpp/Int.h"
#include "islpp/BasicMap.h"
#include "islpp/Set.h"
#include "islpp/MultiAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/Point.h"
#include "islpp/Vertices.h"
#include "CstdioFile.h"

#include <llvm/Support/raw_ostream.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/vertices.h>
//#include <stdio.h>

using namespace isl;
using namespace std;

isl_basic_set *BasicSet::takeCopy() const { 
  assert(set); 
  return isl_basic_set_copy(this->set); 
}

void BasicSet::give(isl_basic_set *set) { 
  if (this->set)
    isl_basic_set_free(this->set);
  this->set = set; 
}

BasicSet::~BasicSet() { 
  if (set) 
    isl_basic_set_free(set);
}

BasicSet BasicSet::create(const Space &space) {
  return BasicSet::wrap(isl_basic_set_universe(space.copy().take()));
}

BasicSet BasicSet::create(Space &&space) {
  return BasicSet::wrap(isl_basic_set_universe(space.take()));
}

BasicSet BasicSet::createEmpty(const Space &space){
  return  BasicSet::wrap(isl_basic_set_empty(space.copy().take()));
}
BasicSet BasicSet::createEmpty(Space &&space){
  return BasicSet::wrap(isl_basic_set_empty(space.take()));
}


BasicSet BasicSet::createUniverse(const Space &space){
  return  BasicSet::wrap(isl_basic_set_universe(space.copy().take()));
}
BasicSet BasicSet::createUniverse(Space &&space){
  return BasicSet::wrap(isl_basic_set_universe(space.take()));
}
BasicSet BasicSet::createNatUniverse(Space &&space){
  return BasicSet::wrap(isl_basic_set_nat_universe(space.take()));
}


BasicSet BasicSet::createFromConstraintMatrices(Space &&space, Mat &&eq, Mat &&ineq, isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4){
  return BasicSet::wrap(isl_basic_set_from_constraint_matrices(space.take(), eq.take(), ineq.take(), c1,c2,c3,c4));
}

BasicSet BasicSet::createFromPoint(Point &&pnt) {
  return BasicSet::wrap(isl_basic_set_from_point(pnt.take()));
}

BasicSet BasicSet::createBoxFromPoints(Point &&pnt1, Point &&pnt2) {
  return BasicSet::wrap(isl_basic_set_box_from_points(pnt1.take(), pnt2.take()));
}

BasicSet BasicSet::readFromFile(Ctx *ctx, FILE *input){
  return BasicSet::wrap(isl_basic_set_read_from_file(ctx->keep(), input));
}
BasicSet BasicSet::readFromStr(Ctx *ctx, const char *str) {
  return BasicSet::wrap(isl_basic_set_read_from_str(ctx->keep(), str));
}

void BasicSet::print(llvm::raw_ostream &out) const { 
  molly::CstdioFile tmp;
  isl_basic_set_print(this->keep(), tmp.getFileDescriptor(), 0, "prefix", "suffic", ISL_FORMAT_ISL);
  out << tmp.readAsStringAndClose();
}

std::string BasicSet::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}

void BasicSet::dump() const { 
  print(llvm::errs());
}


void BasicSet::addConstraint(Constraint &&constraint) {
  give( isl_basic_set_add_constraint(this->take(), constraint.take()));
}

void BasicSet::dropContraint(Constraint &&constraint) {
  give(isl_basic_set_drop_constraint(take(), constraint.take()));
}


int BasicSet::getCountConstraints() const {
  return isl_basic_set_n_constraint(keep());
}
bool BasicSet::foreachConstraint(ConstraintCallback fn, void *user) const {
  return isl_basic_set_foreach_constraint(keep(), fn, user);
}


static int constraintcallback(__isl_take isl_constraint *constraint, void *user) {
  auto fn = *static_cast<std::function<bool(Constraint)>*>(user);
  auto dobreak = fn(Constraint::wrap(constraint));
  return dobreak ? -1 : 0;
}
bool BasicSet::foreachConstraint(std::function<bool(Constraint)> fn) const {
  auto retval = isl_basic_set_foreach_constraint(keep(), constraintcallback, &fn);
  return retval!=0;
}

Mat BasicSet::equalitiesMatrix(isl_dim_type c1, isl_dim_type c2,  isl_dim_type c3,  isl_dim_type c4) const{
  return Mat::wrap(isl_basic_set_equalities_matrix(keep(), c1,c2,c3,c4));
}

Mat BasicSet::inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2,  isl_dim_type c3,  isl_dim_type c4) const{
  return Mat::wrap(isl_basic_set_inequalities_matrix(keep(), c1,c2,c3,c4));
}

void BasicSet::projectOut(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_basic_set_project_out(take(), type, first, n));
}


void BasicSet::params() {
  give(isl_basic_set_params(take()));
}


void BasicSet::eliminate(isl_dim_type type, unsigned first, unsigned n){
  give(isl_basic_set_eliminate(take(), type, first, n));
}
void BasicSet::fix(isl_dim_type type, unsigned pos, const Int &value) {
  give(isl_basic_set_fix(take(), type, pos, value.keep()));
}
void BasicSet::fix(isl_dim_type type, unsigned pos, int value) {
  give(isl_basic_set_fix_si(take(), type, pos, value));
}
void BasicSet::detectEqualities() {
  give(isl_basic_set_detect_equalities(take()));
}
void BasicSet::removeRedundancies() {
  give(isl_basic_set_remove_redundancies(take()));
}
void BasicSet::affineHull() {
  give(isl_basic_set_affine_hull(take())); 
}


Space BasicSet::getSpace() const {
  return Space::wrap(isl_basic_set_get_space(keep()));
}


LocalSpace BasicSet::getLocalSpace() const {
  return LocalSpace::wrap(isl_basic_set_get_local_space(keep()));
}

void BasicSet::removeDivs() {
  give(isl_basic_set_remove_divs(take()));
}

void BasicSet:: removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n){
  give(isl_basic_set_remove_divs_involving_dims(take(), type, first, n));
}

void  BasicSet::removeUnknownDivs() {
  give(isl_basic_set_remove_unknown_divs(take()));
}

unsigned BasicSet::dim(isl_dim_type type) const {
  return isl_basic_set_dim(keep(), type);
}

bool BasicSet::involvesDims(isl_dim_type type, unsigned first, unsigned n) const{
  return isl_basic_set_involves_dims(keep(), type, first, n);
}

const char *BasicSet::getTupleName() const{
  return isl_basic_set_get_tuple_name(keep());
}
void BasicSet::setTupleName(const char *s){
  give(isl_basic_set_set_tuple_name(take(), s));
}


Id BasicSet::getDimId(isl_dim_type type, unsigned pos) const{
  return Id::wrap(isl_basic_set_get_dim_id(keep(), type, pos));
}
const char *BasicSet::getDimName(isl_dim_type type, unsigned pos) const{
  return isl_basic_set_get_dim_name(keep(), type, pos);
}


bool BasicSet::plainIsEmpty() const {
  return isl_basic_set_plain_is_empty(keep());
}

bool BasicSet::isEmpty() const {
  return isl_basic_set_is_empty(keep());
}
bool BasicSet::isUniverse() const{
  return isl_basic_set_is_universe(keep());
}
bool BasicSet::isWrapping() const{
  return isl_basic_set_is_wrapping(keep());
}


void BasicSet::dropConstraintsInvolvingDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_basic_set_drop_constraints_involving_dims(take(), type, first, n));
}
void BasicSet::dropConstraintsNotInvolvingDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_basic_set_drop_constraints_not_involving_dims(take(), type, first, n));
}


void BasicSet::sample() {
  give(isl_basic_set_sample(take()));
}
void BasicSet::coefficients(){
  give(isl_basic_set_coefficients(take()));
}
void BasicSet::solutions(){
  give(isl_basic_set_solutions(take()));
}

void BasicSet::flatten(){
  give(isl_basic_set_flatten(take()));
}
void BasicSet::lift(){
  give(isl_basic_set_lift(take()));
}
void BasicSet::alignParams(Space &&model){
  give(isl_basic_set_align_params(take(), model.take()));
}


void BasicSet::addDims(isl_dim_type type, unsigned n){
  give(isl_basic_set_add_dims(take(), type, n));
}
void BasicSet::insertDims(isl_dim_type type, unsigned pos,  unsigned n){
  give(isl_basic_set_insert_dims(take(), type, pos, n));
}
void BasicSet::moveDims(isl_dim_type dst_type, unsigned dst_pos,  isl_dim_type src_type, unsigned src_pos,  unsigned n){
  give(isl_basic_set_move_dims(take(), dst_type, dst_pos, src_type, src_pos, n));
}


void BasicSet::apply(BasicMap &&bmap){
  give(isl_basic_set_apply(take(), bmap.take()));
}

void BasicSet::preimage(MultiAff &&ma) {
  give(isl_basic_set_preimage_multi_aff(take(), ma.take()));
}

void BasicSet::gist(BasicSet &&context) {
  give(isl_basic_set_gist(take(), context.take()));
}

Vertices BasicSet::computeVertices() const {
  return Vertices::wrap(isl_basic_set_compute_vertices(keep()));
}

BasicSet isl::params(BasicSet &&params) {
  return BasicSet::wrap(isl_basic_set_params(params.take()));
}


bool isl::isSubset(const BasicSet &bset1, const BasicSet &bset2) {
  return isl_basic_set_is_subset(bset1.keep(), bset2.keep());
}

BasicMap isl::unwrap(BasicSet &&bset) {
  return BasicMap::wrap(isl_basic_set_unwrap(bset.take()));
}


BasicSet isl::intersectParams(BasicSet &&bset1, BasicSet &&bset2){
  return BasicSet::wrap( isl_basic_set_intersect_params(bset1.take(), bset2.take()));
}
BasicSet isl::intersect(BasicSet &&bset1, BasicSet &&bset2){
  return BasicSet::wrap( isl_basic_set_intersect(bset1.take(), bset2.take()));
}
Set isl::union_(BasicSet &&bset1, BasicSet &&bset2){
  return Set::wrap( isl_basic_set_union(bset1.take(), bset2.take()));
}

BasicSet isl::flatProduct(BasicSet &&bset1, BasicSet &&bset2) {
  return BasicSet::wrap( isl_basic_set_flat_product(bset1.take(), bset2.take()));
}


Set isl::partialLexmin(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return Set::wrap(isl_basic_set_partial_lexmin(bset.take(), dom.take(), empty.change()));
}
Set isl::partialLexmax(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return Set::wrap(isl_basic_set_partial_lexmax(bset.take(), dom.take(), empty.change()));
}

PwMultiAff isl::partialLexminPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return PwMultiAff::wrap(isl_basic_set_partial_lexmin_pw_multi_aff(bset.take(), dom.take(), empty.change()));
}
PwMultiAff isl::partialLexmaxPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return PwMultiAff::wrap(isl_basic_set_partial_lexmax_pw_multi_aff(bset.take(), dom.take(), empty.change()));
}

Set isl::lexmin(BasicSet &&bset){
  return Set::wrap(isl_basic_set_lexmin(bset.take()));
}
Set isl::lexmax(BasicSet &&bset) {
  return Set::wrap(isl_basic_set_lexmax(bset.take()));
}

Point isl::samplePoint(BasicSet &&bset){
  return Point::wrap(isl_basic_set_sample_point(bset.take()));
}
