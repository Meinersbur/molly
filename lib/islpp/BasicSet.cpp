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
  return BasicSet::wrap(isl_basic_set_from_constraint_matrices(space.take(), eq.take(), ineq.take(), c1, c2, c3, c4));
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


Set BasicSet::toSet() const {
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
  return retval != 0;
}

Mat BasicSet::equalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const{
  return Mat::enwrap(isl_basic_set_equalities_matrix(keep(), c1, c2, c3, c4));
}

Mat BasicSet::inequalitiesMatrix(isl_dim_type c1, isl_dim_type c2, isl_dim_type c3, isl_dim_type c4) const{
  return Mat::enwrap(isl_basic_set_inequalities_matrix(keep(), c1, c2, c3, c4));
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

void BasicSet::removeDivsInvolvingDims(isl_dim_type type, unsigned first, unsigned n){
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


void BasicSet::apply_inplace(BasicMap &&bmap) ISLPP_INPLACE_FUNCTION{
  give(isl_basic_set_apply(take(), bmap.take()));
}

BasicSet BasicSet::apply(const BasicMap &bmap) const {
  return BasicSet::enwrap(isl_basic_set_apply(takeCopy(), bmap.takeCopy()));
}


Set BasicSet::apply(const Map &map) const {
  return Set::enwrap(isl_set_apply(isl_set_from_basic_set(takeCopy()), map.takeCopy()));
}


void BasicSet::gist(BasicSet &&context) {
  give(isl_basic_set_gist(take(), context.take()));
}

Vertices BasicSet::computeVertices() const {
  return Vertices::wrap(isl_basic_set_compute_vertices(keep()));
}


ISLPP_EXSITU_ATTRS Aff BasicSet::dimMin(pos_t pos) ISLPP_EXSITU_FUNCTION{
  auto pwmin = toSet().dimMin(pos);
  return pwmin.singletonAff();
}

ISLPP_EXSITU_ATTRS Aff BasicSet::dimMax(pos_t pos) ISLPP_EXSITU_FUNCTION{
  auto pwmax = toSet().dimMax(pos);
  return pwmax.singletonAff();
}

ISLPP_EXSITU_ATTRS BasicSet isl::BasicSet::cast(Space space) ISLPP_EXSITU_FUNCTION
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
  return BasicSet::wrap(isl_basic_set_intersect_params(bset1.take(), bset2.take()));
}
BasicSet isl::intersect(BasicSet &&bset1, BasicSet &&bset2){
  return BasicSet::wrap(isl_basic_set_intersect(bset1.take(), bset2.take()));
}
Set isl::unite(BasicSet &&bset1, BasicSet &&bset2){
  return Set::enwrap(isl_basic_set_union(bset1.take(), bset2.take()));
}

BasicSet isl::flatProduct(BasicSet &&bset1, BasicSet &&bset2) {
  return BasicSet::wrap(isl_basic_set_flat_product(bset1.take(), bset2.take()));
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

  auto nTotalDims = 1/*cst*/ + bset.dim(isl_dim_all);
  assert(nTotalDims == ineqMat.cols());
  assert(nTotalDims == eqMat.cols());
  auto nDivDims = bset.dim(isl_dim_div);
  auto nOtherDims = nTotalDims - nDivDims - 1;

  uint32_t ops = 0;

  // We compare to a constant, therefore it does not need to be added to the expression (assuming there is an instruction to compare to any constant, not just zero)

  for (auto j = neqs - neqs; j < neqs; j += 1) {
    auto i = 1;
    for (; i < nOtherDims + 1; i += 1) {
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
      auto divdim = i - nOtherDims - 1;
      if (!divsUsed[divdim]) {
        divsUsed[divdim] = true;
        auto div = Aff::enwrap(isl_basic_set_get_div(bset.keep(), divdim));
        ops += opcount ? div.getOpComplexity() : div.getComplexity();
      }
      if (opcount && !coeff.isAbsOne())
        ops += 1; // mul
      ops += 1; // add/cmp
    }
  }
  for (auto j = ineqs - ineqs; j < ineqs; j += 1) {
    auto i = 1;
    for (; i < nOtherDims + 1; i += 1) {
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
      auto divdim = i - nOtherDims - 1;
      if (!divsUsed[divdim]) {
        divsUsed[divdim] = true;
        auto div = Aff::enwrap(isl_basic_set_get_div(bset.keep(), divdim));
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


ISLPP_EXSITU_ATTRS Mat isl::BasicSet::equalitiesMatrix() ISLPP_EXSITU_FUNCTION{
  return equalitiesMatrix(isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div);
}


ISLPP_EXSITU_ATTRS Mat isl::BasicSet::inequalitiesMatrix() ISLPP_EXSITU_FUNCTION{
  return inequalitiesMatrix(isl_dim_cst, isl_dim_param, isl_dim_set, isl_dim_div);
}


ISLPP_EXSITU_ATTRS BasicSet isl::BasicSet::preimage(MultiAff maff) ISLPP_EXSITU_FUNCTION{
  return BasicSet::enwrap(isl_basic_set_preimage_multi_aff(takeCopy(), maff.take()));
}


ISLPP_INPLACE_ATTRS void isl::BasicSet::preimage_inplace(MultiAff maff) ISLPP_INPLACE_FUNCTION{
  give(isl_basic_set_preimage_multi_aff(take(), maff.take()));
}


ISLPP_CONSUME_ATTRS BasicSet isl::BasicSet::preimage_consume(MultiAff maff) ISLPP_CONSUME_FUNCTION{
  return BasicSet::enwrap(isl_basic_set_preimage_multi_aff(take(), maff.take()));
}





//static void divUpperLower(const Aff &div, const llvm::SmallBitVector &lowerFound, const llvm::SmallBitVector &upperFound, const std::vector<pair<Int, Int>> &bounds , bool &hasLower, Int &lower, bool hasUpper, Int &upper) {
//}


bool AllBounds::updateEqBounds(const Mat &eqs, bool areIneqs, int i) {
  //auto nInputDims = getAllDimCount();
  auto nDims = eqs.cols() - 1;
  //auto nDivDims = nDims - nInputDims;

  bool first = false;
  Int lowerSum = eqs[i][0];
  Int upperSum = lowerSum;
  int lowerUndefined = 0;
  int upperUndefined = 0;
  bool changed = false;

  for (pos_t j = 1; j < nDims + 1; j += 1) {
    auto d = j - 1;
    Int coeff = eqs[i][j];
    if (coeff.isZero())
      continue;

    // coeff>0 : [lowerSum .. upperSum] + [coeff*lower .. coeff*upper]
    // coeff<0 : [lowerSum .. upperSum] + [coeff*upper .. coeff*lower]

    if (coeff > 0) {
      if (hasLowerBound(d))
        lowerSum += coeff*getLowerBound(d);
      else
        lowerUndefined += 1;

      if (hasUpperBound(d))
        upperSum += coeff*getUpperBound(d);
      else
        upperUndefined += 1;
    } else {
      assert(coeff < 0);
      if (hasLowerBound(d))
        upperSum += coeff*getLowerBound(d);
      else
        upperUndefined += 1;

      if (hasUpperBound(d))
        lowerSum += coeff*getUpperBound(d);
      else
        lowerUndefined += 1;
    }

    if (lowerUndefined >= 2 && upperUndefined >= 2)
      return false;
  }

  // TODO: refactor with loop from updateDivBounds (lowerNom=0 and upperNom=0 if !areIneqs)
  for (pos_t j = 1; j < nDims + 1; j += 1) {
    auto d = j - 1;
    Int coeff = eqs[i][j];
    if (coeff.isZero())
      continue;

    auto oldHasLowerBound = hasLowerBound(d);
    Int oldLowerBound;
    if (oldHasLowerBound)
      oldLowerBound = getLowerBound(d);
    auto oldHasUpperBound = hasUpperBound(d);
    Int oldUpperBound;
    if (oldHasUpperBound)
      oldUpperBound = getUpperBound(d);

    if (coeff > 0) {
      if (!areIneqs) {
        if (lowerUndefined == 1) {
          if (!oldHasLowerBound/*The reason why lowerUndefined==1*/) {
            auto high = fdiv_q(-lowerSum, coeff);
            changed |= updateConjunctiveUpperBound(d, high);
          }
        } else if (lowerUndefined == 0) {
          auto high = fdiv_q(coeff*oldLowerBound - lowerSum, coeff);
          changed |= updateConjunctiveUpperBound(d, high);
        }
      }

      if (upperUndefined == 1) {
        if (!oldHasUpperBound/*The reason why upperUndefined==1*/) {
          auto low = cdiv_q(-upperSum, coeff);
          changed |= updateConjunctiveLowerBound(d, low);
        }
      } else if (upperUndefined == 0) {
        auto low = cdiv_q(coeff*oldUpperBound/*remove the self-term from upperSum; use oldUpperBound since getUpperBound(d) might have been changed*/ - upperSum, coeff);
        changed |= updateConjunctiveLowerBound(d, low);
      }
    } else {
      assert(coeff < 0);

      if (!areIneqs) {
        if (lowerUndefined == 1) {
          if (!oldHasUpperBound/*The reason why lowerUndefined==1*/) {
            auto low = cdiv_q(-lowerSum, coeff);
            changed |= updateConjunctiveLowerBound(d, low);

          }
        } else if (lowerUndefined == 0) {
          auto low = cdiv_q(coeff*oldUpperBound - lowerSum, coeff);
          changed |= updateConjunctiveLowerBound(d, low);
        }
      }

      if (upperUndefined == 1) {
        if (!oldHasLowerBound/*The reason why upperUndefined==1*/) {
          auto high = fdiv_q(-upperSum, coeff);
          changed |= updateConjunctiveUpperBound(d, high);
        }
      } else if (upperUndefined == 0) {
        auto high = fdiv_q(coeff*oldLowerBound/*remove the self-term from the sum; use oldLowerBound since getLowerBound(d) might have been changed*/ - upperSum, coeff);
        changed |= updateConjunctiveUpperBound(d, high);
      } // if (upperUndefined == 1)
    } // if (coeff > 0) 
  } // for (pos_t j = 1; j < nInputDims + 1; j += 1)

  return changed;
} // bool AllBounds::updateEqBounds(const Mat &eqs, bool areIneqs, llvm::ArrayRef<Aff> divs, int i) 


bool AllBounds::updateDivBounds(const Aff &div, pos_t divOffset) {
  auto nDims = getAllDimCount();

  Int lowerSum = div.getConstant();
  Int upperSum = lowerSum;
  int lowerUndefined = 0;
  int upperUndefined = 0;
  bool changed = false;

  for (pos_t j = 1; j < nDims + 1; j += 1) {
    auto d = j - 1;
    isl_dim_type type;
    pos_t pos;
    offsetToTypePos(d, type, pos);
    auto affType = type == isl_dim_set ? isl_dim_in : type;

    Int coeff = div.getCoefficient(affType, pos);
    if (coeff.isZero())
      continue;

    // coeff>0 : [lowerSum .. upperSum] + [coeff*lower .. coeff*upper]
    // coeff<0 : [lowerSum .. upperSum] + [coeff*upper .. coeff*lower]

    if (coeff > 0) {
      if (hasLowerBound(d))
        lowerSum += coeff*getLowerBound(d);
      else
        lowerUndefined += 1;

      if (hasUpperBound(d))
        upperSum += coeff*getUpperBound(d);
      else
        upperUndefined += 1;
    } else {
      assert(coeff < 0);
      if (hasLowerBound(d))
        upperSum += coeff*getLowerBound(d);
      else
        upperUndefined += 1;

      if (hasUpperBound(d))
        lowerSum += coeff*getUpperBound(d);
      else
        lowerUndefined += 1;
    }

    if (lowerUndefined >= 2 && upperUndefined >= 2)
      return false;
  }

  auto denom = div.getDenominator();
  assert(denom > 0);

  // forward derive
  if (lowerUndefined == 0) {
    auto lowerVal = fdiv_q(lowerSum, denom);
    changed |= updateConjunctiveLowerBound(divOffset, lowerVal);
  }
  if (upperUndefined == 0) {
    auto upperVal = fdiv_q(upperSum, denom);
    changed |= updateConjunctiveUpperBound(divOffset, upperVal);
  }


  // backward derive
  Int lowerNom;
  auto hasLowerNom = hasLowerBound(divOffset);
  if (hasLowerNom) {
    lowerNom = denom*getLowerBound(divOffset);
  }

  Int upperNom;
  auto hasUpperNom = hasUpperBound(divOffset);
  if (hasUpperNom) {
    upperNom = denom*getUpperBound(divOffset) + denom - 1;
  }

  // Inequations to apply:
  // lowerNom <= lowerSum             (if hasLowerNom && lowerUndefined<=1)
  //             upperSum <= upperNom (if hasUpperNom && upperUndefined<=1)

  for (pos_t j = 1; j < nDims + 1; j += 1) {
    auto d = j - 1;
    isl_dim_type type;
    pos_t pos;
    offsetToTypePos(d, type, pos);
    auto affType = type == isl_dim_set ? isl_dim_in : type;

    Int coeff = div.getCoefficient(affType, pos);
    if (coeff.isZero())
      continue;


    auto oldHasLowerBound = hasLowerBound(d);
    Int oldLowerBound;
    if (oldHasLowerBound)
      oldLowerBound = getLowerBound(d);
    auto oldHasUpperBound = hasUpperBound(d);
    Int oldUpperBound;
    if (oldHasUpperBound)
      oldUpperBound = getUpperBound(d);

    if (coeff > 0) {
      if (hasUpperNom) {
        if (lowerUndefined == 1) {
          if (!oldHasLowerBound/*The reason why lowerUndefined==1*/) {
            auto high = fdiv_q(upperNom - lowerSum, coeff);
            changed |= updateConjunctiveUpperBound(d, high);
          }
        } else if (lowerUndefined == 0) {
          auto high = fdiv_q(upperNom - (lowerSum - coeff*oldLowerBound), coeff);
          changed |= updateConjunctiveUpperBound(d, high);
        }
      }

      if (hasLowerNom) {
        if (upperUndefined == 1) {
          if (!oldHasUpperBound/*The reason why upperUndefined==1*/) {
            auto low = cdiv_q(lowerNom - upperSum, coeff);
            changed |= updateConjunctiveLowerBound(d, low);
          }
        } else if (upperUndefined == 0) {
          auto low = cdiv_q(lowerNom - (upperSum - coeff*oldUpperBound), coeff);
          changed |= updateConjunctiveLowerBound(d, low);
        }
      }
    } else {
      assert(coeff < 0);

      if (hasUpperNom) {
        if (lowerUndefined == 1) {
          if (!oldHasUpperBound/*The reason why lowerUndefined==1*/) {
            auto low = cdiv_q(upperNom - lowerSum, coeff);
            changed |= updateConjunctiveLowerBound(d, low);
          }
        } else if (lowerUndefined == 0) {
          auto low = cdiv_q(upperNom - (lowerSum - coeff*oldUpperBound), coeff);
          changed |= updateConjunctiveLowerBound(d, low);
        }
      }

      if (hasLowerNom) {
        if (upperUndefined == 1) {
          if (!oldHasLowerBound/*The reason why upperUndefined==1*/) {
            auto high = fdiv_q(lowerNom - upperSum, coeff);
            changed |= updateConjunctiveUpperBound(d, high);
          }
        } else if (upperUndefined == 0) {
          auto high = fdiv_q(lowerNom - (upperSum - coeff*oldLowerBound), coeff);
          changed |= updateConjunctiveUpperBound(d, high);
        } //  if (upperUndefined == 1)
      } // if (hasLowerNom)
    } // if (coeff > 0) 
  } //  for (pos_t j = 1; j < nDims + 1; j += 1)

  return changed;
}


void AllBounds::searchAllBounds(const BasicSet &bset) {
  auto eqs = bset.equalitiesMatrix();
  auto ineqs = bset.inequalitiesMatrix();
  auto nEqs = eqs.rows();
  auto nIneqs = ineqs.rows();
  auto nDims = eqs.cols() - 1; // Without constant
  auto nInputDims = bset.getParamDimCount() + bset.getSetDimCount();
  assert(getAllDimCount() == nDims);
  auto nDivDims = bset.getDivDimCount();

  //llvm::SmallBitVector lowerFound;
  //llvm::SmallBitVector upperFound;
  //std::vector<pair<Int, Int>> result;
  //lowerFound.resize(nInputDims);
  //upperFound.resize(nInputDims);
  //result.resize(nInputDims);
  //bounds.resize(nInputDims);

  llvm::SmallVector<Aff, 4> divs;
  divs.reserve(nDivDims);
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    divs.push_back(bset.getDiv(i));
  }

  while (true) {
    bool changed = false;

    for (auto i = nEqs - nEqs; i < nEqs; i += 1) {
      auto eqChanged = updateEqBounds(eqs, false, i);
      changed = changed || eqChanged;
    }

    for (auto i = nIneqs - nIneqs; i < nIneqs; i += 1) {
      auto ineqChanged = updateEqBounds(ineqs, true, i);
      changed = changed || ineqChanged;
    }

    // Update divs forward
    // Evaluate div and derive the range of it // Why not using isl_basic_set_remove_divs?
    for (auto j = nDivDims - nDivDims; j < nDivDims; j += 1) {
      const auto &div = divs[j];
      auto divChanged = updateDivBounds(div, nInputDims + j);
      changed = changed || divChanged;
    }

    if (!changed)
      break;
  }

#ifndef NDEBUG
  if (isImpossible()) {
    if (!bset.isEmpty()) {
      bset.getAllBounds();
      assert(!"Its not impossible");
    }
  }
#endif /* NDEBUG */
}


void isl::AllBounds::disjunctiveMerge(const AllBounds &that) {
  assert(that.getParamDimsCount() == this->getParamDimsCount());
  assert(this->getSetDimCount() == that.getSetDimCount());

  auto n = getAllDimCount();
  for (auto d = n - n; d < n; d += 1) {
    if (!this->hasLowerBound(d)){
    } else if (!that.hasLowerBound(d))
      resetLowerBound(d);
    else
      this->updateDisjunktiveLowerBound(d, that.getLowerBound(d));

    if (!this->hasUpperBound(d)) {
    } else if (!that.hasUpperBound(d))
      resetUpperBound(d);
    else
      this->updateDisjunctiveUpperBound(d, that.getUpperBound(d));
  }
}


bool isl::AllBounds::isImpossible() const {
  auto nDims = getAllDimCount();
  for (auto i = nDims - nDims; i < nDims; i += 1) {
    if (hasLowerBound(i) && hasUpperBound(i)) {
      if (getLowerBound(i) > getUpperBound(i))
        return true;
    }
  }
  return false;
}


AllBounds BasicSet::getAllBounds() const {
  AllBounds bounds;
  bounds.init(getLocalSpace());
  bounds.searchAllBounds(*this);
  return bounds; // NRVO
}


static HasBounds searchBounds(const BasicSet &bset, isl_dim_type type, pos_t pos, Int &lowerBound, Int &upperBound) {
  auto eqs = bset.equalitiesMatrix();
  auto ineqs = bset.inequalitiesMatrix();
  // TODO: transitive bounds
  bool lowerFound = false;
  bool upperFound = false;
  auto nEqs = eqs.rows();
  auto posOffset = bset.matrixOffset(type, pos);
  auto nDims = eqs.cols() - 1; // Without constant
  for (auto i = nEqs - nEqs; i < nEqs; i += 1) {
    Int posCoeff = eqs[i][posOffset];
    if (posCoeff.isZero())
      continue; // This equality does not influence the dimension in question

    bool otherFound = false;
    for (pos_t j = 1; j < nDims + 1; j += 1) {
      if (j == posOffset)
        continue;

      Int coeff = eqs[i][j];
      if (coeff.isZero())
        continue;

      otherFound = true;
      break;
    }
    if (otherFound)
      continue; // Too complex equation

    Int cst = eqs[i][0];
    auto potentialLowerBound = fdiv_q(cst, posCoeff);
    auto potentialUpperBound = cdiv_q(cst, posCoeff);

    if (!lowerFound || (lowerBound < potentialLowerBound)) {
      lowerBound = potentialLowerBound;
    }
    lowerFound = true;
    if (!upperFound || (upperBound > potentialUpperBound)) {
      upperBound = potentialUpperBound;
    }
    upperFound = true;
  }

  auto nIneqs = ineqs.rows();
  for (auto i = nIneqs - nIneqs; i < nIneqs; i += 1) {
    Int posCoeff = ineqs[i][posOffset];
    if (posCoeff.isZero())
      continue; // This inequality does not influence the dimension in question

    bool otherFound = false;
    for (pos_t j = 1; j < nDims + 1; j += 1) {
      if (j == posOffset)
        continue;

      Int coeff = ineqs[i][j];
      if (coeff.isZero())
        continue;

      otherFound = true;
      break;
    }
    if (otherFound)
      continue; // Too complex inequation

    Int cst = ineqs[i][0];
    if (!posCoeff.isNeg()) {
      // lower bound
      auto potentialLowerBound = fdiv_q(cst, -posCoeff);
      if (!lowerFound || (lowerBound < potentialLowerBound)) {
        lowerBound = potentialLowerBound;
      }
      lowerFound = true;
    } else {
      // upper bound
      auto potentialUpperBound = cdiv_q(cst, -posCoeff);
      if (!upperFound || (upperBound > potentialUpperBound)) {
        upperBound = potentialUpperBound;
      }
      upperFound = true;
    }
  }

  if (lowerFound && upperFound && (lowerBound == upperBound))
    return HasBounds::Fixed;
  if (lowerFound && upperFound && (lowerBound > upperBound))
    return HasBounds::Impossible;
  HasBounds result = HasBounds::Unbounded;
  if (lowerFound)
    result |= HasBounds::LowerBound;
  if (upperFound)
    result |= HasBounds::UpperBound;
  return result;
}


HasBounds BasicSet::getDimBounds(isl_dim_type type, pos_t pos, Int &lowerBound, Int &upperBound) {
  return searchBounds(*this, type, pos, lowerBound, upperBound);
}


bool BasicSet::hasLowerBound(isl_dim_type type, pos_t pos, Int &lowerBound) const {
  Int upperBound;
  auto retval = searchBounds(*this, type, pos, lowerBound, upperBound);
  return flags(retval & HasBounds::LowerBound);
}


bool BasicSet::hasUpperBound(isl_dim_type type, pos_t pos, Int &upperBound) const {
  Int lowerBound;
  auto retval = searchBounds(*this, type, pos, lowerBound, upperBound);
  return flags(retval & HasBounds::UpperBound);
}


void AllBounds::dump() const {
  auto nParamDims = getParamDimsCount();
  for (auto i = nParamDims - nParamDims; i < nParamDims; i += 1) {
    if (hasLowerBound(isl_dim_param, i)) {
      llvm::errs() << getLowerBound(isl_dim_param, i) << " <= ";
    }
    if (space.hasDimName(isl_dim_param, i)) {
      llvm::errs() << space.getDimName(isl_dim_param, i);
    } else {
      llvm::errs() << "p" << i;
    }
    if (hasUpperBound(isl_dim_param, i)) {
      llvm::errs() << " <= " << getUpperBound(isl_dim_param, i);
    }
    llvm::errs() << "\n";
  }


  auto nInDims = getSetDimCount();
  for (auto i = nInDims - nInDims; i < nInDims; i += 1) {
    if (hasLowerBound(isl_dim_set, i)) {
      llvm::errs() << getLowerBound(isl_dim_set, i) << " <= ";
    }
    if (space.hasDimName(isl_dim_set, i)) {
      // TODO: isl also count how many dims have the same name and adds prime characters (') to distinguish them
      llvm::errs() << space.getDimName(isl_dim_set, i);
    } else {
      llvm::errs() << "i" << i;
    }
    if (hasUpperBound(isl_dim_set, i)) {
      llvm::errs() << " <= " << getUpperBound(isl_dim_set, i);
    }
    llvm::errs() << "\n";
  }

  auto nDivDims = getDivDimCount();
  for (auto i = nDivDims - nDivDims; i < nDivDims; i += 1) {
    if (hasLowerBound(isl_dim_div, i)) {
      llvm::errs() << getLowerBound(isl_dim_div, i) << " <= ";
    }
    llvm::errs() << "e" << i; // Div dims never have names
    if (hasUpperBound(isl_dim_div, i)) {
      llvm::errs() << " <= " << getUpperBound(isl_dim_div, i);
    }
    llvm::errs() << "\n";
  }
}
