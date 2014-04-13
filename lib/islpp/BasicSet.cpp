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
#include "islpp/Map.h"
#include "cstdiofile.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallBitVector.h>

#include <isl/set.h>
#include <isl/map.h>
#include <isl/vertices.h>
#include <functional>

using namespace isl;
using namespace std;




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

BasicSet BasicSet::createFromBasicMap(BasicMap &&bmap) { 
  return wrap(isl_basic_set_from_basic_map(bmap.take())); 
}

BasicSet BasicSet::readFromFile(Ctx *ctx, FILE *input){
  return BasicSet::wrap(isl_basic_set_read_from_file(ctx->keep(), input));
}


BasicSet BasicSet::readFromStr(Ctx *ctx, const char *str) {
  return BasicSet::wrap(isl_basic_set_read_from_str(ctx->keep(), str));
}


 Set BasicSet:: toSet() const {
   return Set::enwrap(isl_set_from_basic_set(takeCopy()));
 }


void BasicSet::print(llvm::raw_ostream &out) const { 
  molly::CstdioFile tmp;
  isl_basic_set_print(this->keep(), tmp.getFileDescriptor(), 0, "prefix", "suffix", ISL_FORMAT_ISL);
  out << tmp.readAsStringAndClose();
}


void BasicSet::dump() const { 
  isl_basic_set_dump(keep());
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
  auto dobreak = fn(Constraint::enwrap(constraint));
  return dobreak ? -1 : 0;
}
bool BasicSet::foreachConstraint(std::function<bool(Constraint)> fn) const {
  auto retval = isl_basic_set_foreach_constraint(keep(), constraintcallback, &fn);
  return retval!=0;
}

Mat BasicSet::equalitiesMatrix(isl_dim_type c1, isl_dim_type c2,  isl_dim_type c3,  isl_dim_type c4) const{
  return Mat::enwrap(isl_basic_set_equalities_matrix(keep(), c1,c2,c3,c4));
}

Mat BasicSet::inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2,  isl_dim_type c3,  isl_dim_type c4) const{
  return Mat::enwrap(isl_basic_set_inequalities_matrix(keep(), c1,c2,c3,c4));
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

ISLPP_INPLACE_ATTRS void BasicSet::detectEqualities_inplace()ISLPP_INPLACE_FUNCTION{
  give(isl_basic_set_detect_equalities(take()));
}
ISLPP_INPLACE_ATTRS void BasicSet::removeRedundancies_inplace()ISLPP_INPLACE_FUNCTION{
  give(isl_basic_set_remove_redundancies(take()));
}
ISLPP_INPLACE_ATTRS void BasicSet::affineHull_inplace() ISLPP_INPLACE_FUNCTION{
  give(isl_basic_set_affine_hull(take())); 
}

#if 0
Space BasicSet::getSpace() const {
  return Space::wrap(isl_basic_set_get_space(keep()));
}


LocalSpace BasicSet::getLocalSpace() const {
  return LocalSpace::wrap(isl_basic_set_get_local_space(keep()));
}
#endif

void BasicSet::removeDivs() {
  give(isl_basic_set_remove_divs(take()));
}

void BasicSet:: removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n){
  give(isl_basic_set_remove_divs_involving_dims(take(), type, first, n));
}

void  BasicSet::removeUnknownDivs() {
  give(isl_basic_set_remove_unknown_divs(take()));
}


bool BasicSet::involvesDims(isl_dim_type type, unsigned first, unsigned n) const{
  return isl_basic_set_involves_dims(keep(), type, first, n);
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


void BasicSet::apply_inplace(BasicMap &&bmap) ISLPP_INPLACE_FUNCTION {
  give(isl_basic_set_apply(take(), bmap.take()));
}

    BasicSet BasicSet::apply(const BasicMap &bmap) const {
      return BasicSet::enwrap(isl_basic_set_apply(takeCopy(), bmap.takeCopy()));
    }


    Set BasicSet::apply(const Map &map) const {
      return Set::enwrap(isl_set_apply(isl_set_from_basic_set(takeCopy()), map.takeCopy()));
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


ISLPP_EXSITU_ATTRS Aff BasicSet::dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION {
  auto pwmin = toSet().dimMin(pos);
  return pwmin.singletonAff();
}

ISLPP_EXSITU_ATTRS Aff BasicSet::dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION {
  auto pwmax = toSet().dimMax(pos);
  return pwmax.singletonAff();
}

ISLPP_EXSITU_ATTRS BasicSet isl::BasicSet::cast( Space space ) ISLPP_EXSITU_FUNCTION
{
  auto mapSpace = getSpace().mapsTo(std::move(space));
  auto map = mapSpace.equalBasicMap();
  return apply(map);
}



BasicSet isl::params(BasicSet &&params) {
  return BasicSet::wrap(isl_basic_set_params(params.take()));
}


bool isl::isSubset(const BasicSet &bset1, const BasicSet &bset2) {
  return isl_basic_set_is_subset(bset1.keep(), bset2.keep());
}

BasicMap isl::unwrap(BasicSet &&bset) {
  return BasicMap::enwrap(isl_basic_set_unwrap(bset.take()));
}


BasicSet isl::intersectParams(BasicSet &&bset1, BasicSet &&bset2){
  return BasicSet::wrap( isl_basic_set_intersect_params(bset1.take(), bset2.take()));
}
BasicSet isl::intersect(BasicSet &&bset1, BasicSet &&bset2){
  return BasicSet::wrap( isl_basic_set_intersect(bset1.take(), bset2.take()));
}
Set isl::unite(BasicSet &&bset1, BasicSet &&bset2){
  return Set::enwrap(isl_basic_set_union(bset1.take(), bset2.take()));
}

BasicSet isl::flatProduct(BasicSet &&bset1, BasicSet &&bset2) {
  return BasicSet::wrap( isl_basic_set_flat_product(bset1.take(), bset2.take()));
}


Set isl::partialLexmin(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return Set::enwrap(isl_basic_set_partial_lexmin(bset.take(), dom.take(), empty.change()));
}
Set isl::partialLexmax(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return Set::enwrap(isl_basic_set_partial_lexmax(bset.take(), dom.take(), empty.change()));
}

PwMultiAff isl::partialLexminPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return PwMultiAff::enwrap(isl_basic_set_partial_lexmin_pw_multi_aff(bset.take(), dom.take(), empty.change()));
}
PwMultiAff isl::partialLexmaxPwMultiAff(BasicSet &&bset, BasicSet &&dom, /*give*/ Set &empty){
  return PwMultiAff::enwrap(isl_basic_set_partial_lexmax_pw_multi_aff(bset.take(), dom.take(), empty.change()));
}

Set isl::lexmin(BasicSet &&bset){
  return Set::enwrap(isl_basic_set_lexmin(bset.take()));
}
Set isl::lexmax(BasicSet &&bset) {
  return Set::enwrap(isl_basic_set_lexmax(bset.take()));
}


Point isl::samplePoint(BasicSet &&bset){
  return Point::enwrap(isl_basic_set_sample_point(bset.take()));
}


static uint32_t complexity(const BasicSet &bset, bool opcount, llvm::SmallBitVector &divsUsed) {
  if (bset.isUniverse())
    return 0;
  if (bset.isEmpty())
    return 0;

  auto eqMat = Mat::enwrap(isl_basic_set_equalities_matrix(bset.keep(), isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div));
  auto ineqMat = Mat::enwrap(isl_basic_set_inequalities_matrix(bset.keep(), isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div));
  assert(eqMat.cols() == ineqMat.cols());
  auto neqs = eqMat.rows();
  auto ineqs = ineqMat.rows();

  auto nTotalDims = bset.dim(isl_dim_all);
  assert(nTotalDims == ineqMat.cols());
  assert(nTotalDims == eqMat.cols());
  auto nDivDims = bset.dim(isl_dim_div);
  auto nOtherDims = nTotalDims - nDivDims - 1;

  uint32_t ops = 0;

  // We compare to a constant, therefore it does not need to be added to the expression (assuming there is an instruction to compare to any constant, not just zero)
 
  for (auto j = neqs - neqs; j < neqs; j += 1) {
    auto i = 1;
    for (; i < nOtherDims; i += 1) {
      Int coeff = eqMat[j][i];
      if (coeff.isZero())
        continue;
      if (opcount && !coeff.isAbsOne())
        ops += 1; // mul
      ops += 1; // add/cmp
    }
    for (; i < nTotalDims; i += 1) {
      Int coeff = eqMat[j][i];
      if (coeff.isZero())
        continue;
      if (!divsUsed[i]) {
        divsUsed[i] = true;
        auto div = Aff::enwrap(isl_basic_set_get_div(bset.keep(), i));
        ops += opcount ? div.getOpComplexity() : div.getComplexity();
      }
      if (opcount && !coeff.isAbsOne())
        ops += 1; // mul
      ops += 1; // add/cmp
    }
  }
  for (auto j = ineqs - ineqs; j < ineqs; j += 1) {
    auto i = 1;
    for (; i < nOtherDims; i += 1) {
      Int coeff = ineqMat[j][i];
      if (coeff.isZero())
        continue;
      if (opcount && !coeff.isAbsOne())
        ops += 1; // mul
      ops += 1; // add/cmp
    }
    for (; i < nTotalDims; i += 1) {
      Int coeff = ineqMat[j][i];
      if (coeff.isZero())
        continue;
      if (!divsUsed[i]) {
        divsUsed[i] = true;
        auto div = Aff::enwrap(isl_basic_set_get_div(bset.keep(), i));
        ops += opcount ? div.getOpComplexity() : div.getComplexity();
      }
      if (opcount && !coeff.isAbsOne())
        ops += 1; // mul
      ops += 1; // add/cmp
    }
  }

  if (opcount) {
    ops += neqs + ineqs - 1; // For the logical ands between conditions
  }

  return ops;
}


ISLPP_PROJECTION_ATTRS uint32_t isl::BasicSet::getComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDivDims = dim(isl_dim_div);
  llvm::SmallBitVector divsUsed;
  divsUsed.resize(nDivDims);
  return complexity(*this, false, divsUsed);
}


ISLPP_PROJECTION_ATTRS uint32_t isl::BasicSet::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  auto nDivDims = dim(isl_dim_div);
  llvm::SmallBitVector divsUsed;
  divsUsed.resize(nDivDims);
  return complexity(*this, true, divsUsed);
}
