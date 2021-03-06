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
  return Map::enwrap(isl_map_from_pw_aff(takeCopy()));
}


MultiPwAff PwAff::toMultiPwAff() ISLPP_EXSITU_FUNCTION{
  return MultiPwAff::enwrap(isl_multi_pw_aff_from_pw_aff(takeCopy()));
}


PwMultiAff PwAff::toPwMultiAff() ISLPP_EXSITU_FUNCTION{
  return PwMultiAff::enwrap(isl_pw_multi_aff_from_pw_aff(takeCopy()));
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


ISLPP_INPLACE_ATTRS void PwAff::coalesce_inplace() ISLPP_INPLACE_FUNCTION{
  give(isl_pw_aff_coalesce(take()));
}


void PwAff::gistParams(Set &&context) {
  give(isl_pw_aff_gist_params(take(), context.take()));
}


PwAff PwAff::pullback(const MultiAff &maff) const {
  return PwAff::enwrap(isl_pw_aff_pullback_multi_aff(takeCopy(), maff.takeCopy()));
}


void PwAff::pullback_inplace(const MultiAff &maff) ISLPP_INPLACE_FUNCTION{
  give(isl_pw_aff_pullback_multi_aff(take(), maff.takeCopy()));
}


PwAff PwAff::pullback(const PwMultiAff &pmaff) const {
  return PwAff::enwrap(isl_pw_aff_pullback_pw_multi_aff(takeCopy(), pmaff.takeCopy()));
}


void PwAff::pullback_inplace(const PwMultiAff &pma) ISLPP_INPLACE_FUNCTION{
  give(isl_pw_aff_pullback_pw_multi_aff(take(), pma.takeCopy()));
}


#if 0
PwAff PwAff::pullback(const MultiPwAff &mpa) ISLPP_EXSITU_QUALIFIER {

}


void PwAff::pullback_inplace(const MultiPwAff &mpa) ISLPP_INPLACE_QUALIFIER {
}
#endif


int PwAff::nPiece() const {
  return isl_pw_aff_n_piece(keep());
}


static int piececallback(isl_set *set, isl_aff *aff, void *user) {
  auto fn = *static_cast<std::function<bool(Set, Aff)>*>(user);
  auto retval = fn(Set::enwrap(set), Aff::enwrap(aff));
  return retval ? -1 : 0;
}
bool PwAff::foreachPiece(std::function<bool(Set, Aff)> fn) const {
  auto retval = isl_pw_aff_foreach_piece(keep(), piececallback, &fn);
  return (retval != 0);
}


static int enumPiecesCallback(__isl_take isl_set *set, __isl_take isl_aff *aff, void *user) {
  auto list = static_cast<std::vector<std::pair<Set, Aff>> *>(user);
  list->push_back(std::make_pair(Set::enwrap(set), Aff::enwrap(aff)));
  return 0;
}
std::vector<std::pair<Set, Aff>> PwAff::getPieces() const {
  std::vector<std::pair<Set, Aff>> result;
  result.reserve(isl_pw_aff_n_piece(keep()));
  auto retval = isl_pw_aff_foreach_piece(keep(), enumPiecesCallback, &result);
  assert(retval == 0);
  return result;
}


ISLPP_EXSITU_ATTRS Aff PwAff::singletonAff() ISLPP_EXSITU_FUNCTION{
  Aff result;
  foreachPiece([&result](Set set, Aff aff) -> bool {
    if (result.isValid()) {
      result.reset();
      return true; // break with error; no singleton pw
    }
    result = aff;
    return false; // continue to see if there are multiple
  });
  assert(result.isValid());
  return result;
}


void isl::PwAff::dumpExplicit(int maxElts /*= 8*/)const{
  printExplicit(llvm::dbgs(), maxElts);
  llvm::dbgs() << "\n";
}


void isl::PwAff::dumpExplicit() const {
  dumpExplicit(8);
}


void isl::PwAff::printExplicit(llvm::raw_ostream &os, int maxElts /*= 8*/) const {
  toMap().printExplicit(os, maxElts);
}


std::string isl::PwAff::toStringExplicit(int maxElts /*= 8*/) {
  std::string str;
  llvm::raw_string_ostream os(str);
  printExplicit(os, maxElts);
  os.flush();
  return str; // NRVO
}


#ifndef NDEBUG
std::string isl::PwAff::toString() const {
  return ObjBaseTy::toString();
}
#endif

PwAff isl::unionMin(PwAff pwaff1, PwAff pwaff2) {
  return PwAff::enwrap(isl_pw_aff_union_min(pwaff1.take(), pwaff2.take()));
}
PwAff isl::unionMax(PwAff pwaff1, PwAff pwaff2) {
  return PwAff::enwrap(isl_pw_aff_union_max(pwaff1.take(), pwaff2.take()));
}
PwAff isl::unionAdd(PwAff pwaff1, PwAff pwaff2) {
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
PwAff isl::sub(const PwAff &pwaff1, const  PwAff &pwaff2) {
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


ISLPP_EXSITU_ATTRS PwAff isl::PwAff::cast(Space space) ISLPP_EXSITU_FUNCTION{
  assert(dim(isl_dim_in) == space.dim(isl_dim_in));
  assert(dim(isl_dim_out) == space.dim(isl_dim_out));

  auto domainSpace = space.getDomainSpace();
  auto transfromDomainSpace = getDomainSpace().mapsTo(domainSpace);
  auto transformDomain = transfromDomainSpace.createUniverseBasicMap();

  auto result = space.createEmptyPwAff();
  foreachPiece([&result, &domainSpace, &transformDomain](Set set, Aff aff) {
    //aff.pullback_inplace(transformDomain);
    aff.castDomain_inplace(domainSpace);
    set.apply_inplace(transformDomain);
    result.unionAdd_inplace(PwAff::create(set, aff)); // TODO: disjoint union add for speed
    return true;
  });
  return result;
}


ISLPP_INPLACE_ATTRS void PwAff::castDomain_inplace(Space domainSpace) ISLPP_INPLACE_FUNCTION{
  assert(dim(isl_dim_in) == domainSpace.getSetDimCount());

  auto transfromDomainSpace = Space::createMapFromDomainAndRange(domainSpace, getDomainSpace());
  auto transformDomain = transfromDomainSpace.createIdentityMultiAff();

  pullback_inplace(transformDomain);
}


ISLPP_EXSITU_ATTRS Map isl::PwAff::reverse() ISLPP_EXSITU_FUNCTION{
  return toMap().reverse();
}


ISLPP_EXSITU_ATTRS PwAff isl::PwAff::mod(Val divisor) ISLPP_EXSITU_FUNCTION{
  return PwAff::enwrap(isl_pw_aff_mod_val(takeCopy(), divisor.take()));
}


void isl::Pw<Aff>::dump() const {
  isl_pw_aff_dump(keep());
}



/// Merge as many affine pieces as possible
/// Greedy algorithm
template<typename BasicSet>
static void combineAffs(std::vector<std::pair<Set, Aff>> &merged, std::vector<std::pair<BasicSet, Aff>> &affs, bool tryCoeffs, bool tryDivs) {
  auto n = affs.size();
  merged.reserve(n);
  while (n) {
    assert(n == affs.size());
    Set set = affs[n - 1].first;
    auto aff = affs[n - 1].second;
   
    affs.pop_back();
    n -= 1;

    auto ip = n;
    while (ip) {
      auto i = ip - 1;
      auto bset = affs[i].first;
      auto baff = affs[i].second;

      Aff maff;
      Set mset;
      if (isl::tryCombineAff(set, aff, bset, baff, tryCoeffs, tryDivs, mset, maff)) {
        aff = maff;
        set = mset;

        affs.erase(affs.begin() + i);
        n -= 1;
      }
      ip -= 1;
    }

    merged.emplace_back(set, aff);
  }
}


template<typename BasicSet>
static PwAff fromList(const std::vector<pair<BasicSet, Aff>> &list) {
  const auto &any = list[0];
  auto result = any.second.getSpace().createEmptyPwAff();
  for (const auto &pair : list) {
    const auto &set = pair.first;
    const auto &aff = pair.second;
    result.unionAdd_inplace(PwAff::create(set, aff));
  }
  return result;
}


/// Similar to isl_pw_aff_coalesce, but more thorough
ISLPP_EXSITU_ATTRS PwAff isl::Pw<Aff>::simplify() ISLPP_EXSITU_FUNCTION{
  auto nParamDims = getParamDimCount();
  auto nDomainDims = getInDimCount();

  // Normalize affs
  // TODO: Because this is done per BasicSet -- not per piece -- the same aff can gist differently, leading to that those cannot be merged again
  // one of those affs can be simpler than the other since its domain is more restrictive
  // Still have to consider whether this is a bad thing since at least one of the expressions is simpler than before; the total number of BasicSets from all pieces does not worsen which is what matters
  // There is an exception: If gist()/gistUndefined() is applied some time later, this possible inhibits the pieces extended outside context
  std::vector<pair<Set, Aff>> affs;
  for (auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    // Normalize the divs to the way they are added by isl_aff_floor
    assert(aff.normalizeDivs() == aff);
    aff.normalizeDivs_inplace();

    aff.gist_inplace(set);
    affs.emplace_back(set, std::move(aff));

#if 0
    // TODO: This may add more pieces, leaving the sets overlap would actually be cheaper
    set.makeDisjoint_inplace();

    for (const auto &bset : set.getBasicSets()) {
      //bset.detectEqualities_inplace(); // make isl_basic_set_is_fixed work
      auto normalizedAff = aff.gist(bset);

      assert(PwAff::create(bset, aff) == PwAff::create(bset, normalizedAff));
      affs.emplace_back(bset, std::move(normalizedAff));
    }
#endif
  }
#if 0
  if (fromList(affs) != *this) {
    auto ref = fromList(affs);
    auto diff1 = ref - *this;
    auto diffDom1 = ref.domain() - domain();
    diff1.dump();
    auto diff2 = *this - ref;
    auto diffDom2 = domain() - ref.domain();
    diff2.dump();
    int a = 0;
    //simplify();
  }
#endif

  std::vector<pair<Set, Aff>> merged;
  combineAffs(merged, affs, false, false);
  std::vector<pair<Set, Aff>> coeffmerged;
  combineAffs(coeffmerged, merged, true, false);
  std::vector<pair<Set, Aff>> divmerged;
  combineAffs(divmerged, coeffmerged, true, true);

  auto result = getSpace().createEmptyPwAff();
  for (auto &pair : divmerged) {
    auto &set = pair.first;
    set.coalesce_inplace();
    auto &aff = pair.second;
    result.unionAdd_inplace(PwAff::create(std::move(set), std::move(aff)));
  }

  auto beforeComplexity = getComplexity();
  auto afterComplexity = result.getComplexity();
#ifndef NDEBUG
  auto beforeOpComplexity = getOpComplexity();
  auto afterOpComplexity = result.getOpComplexity();

  if (beforeComplexity < afterComplexity) {
    int a = 0;
    if ((beforeComplexity >> 32) < (afterComplexity >> 32)) {
      int c = 0;
    }
    //auto tmp = simplify();
  }
  if (beforeOpComplexity < afterOpComplexity) {
    int b = 0;
  }
#endif

  if (beforeComplexity < afterComplexity) {
    // It's not the idea to make it even more complicated
    result = copy();
  }

  assert(*this == result);
  return result; // NRVO
}


ISLPP_PROJECTION_ATTRS size_t isl::Pw<Aff>::nPieces() ISLPP_PROJECTION_FUNCTION{
  return isl_pw_aff_n_piece(keep());
}


ISLPP_PROJECTION_ATTRS uint64_t PwAff::getComplexity() ISLPP_PROJECTION_FUNCTION{
  uint32_t nBsets = 0;
  uint32_t affComplexity = 0;
  uint32_t bsetComplexity = 0;

  for (const auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    // The impact of the expression itself is actually very minor and only one really needs to be evaluated. We assume the most complex one
    affComplexity = std::max(affComplexity, aff.getComplexity());

    for (auto const &bset : set.getBasicSets()) {// for an input vector need to determine in which bset it is; which aff it resolves to is of minor importance
      nBsets += 1;
      bsetComplexity += bset.getComplexity();
    }
  }

  if (!nBsets)
    return 0;
  uint64_t result = (nBsets - 1); // Remove one because even the simplest (except the trivial) Pw have one piece
  result <<= 32;
  result |= bsetComplexity + affComplexity;
  return result;
}


ISLPP_PROJECTION_ATTRS uint64_t PwAff::getOpComplexity() ISLPP_PROJECTION_FUNCTION{
  uint64_t complexity = 0;
  int nPieces = 0;

  for (const auto &pair : getPieces()) {
    auto &set = pair.first;
    auto &aff = pair.second;

    complexity += set.getOpComplexity();
    complexity += aff.getOpComplexity();
    nPieces += 1;
  }

  if (nPieces == 0)
    return 0;

  complexity += nPieces - 1; // select for matching piece; just last one doesn't need a select

  return complexity;
}


ISLPP_INPLACE_ATTRS void PwAff::gistUndefined_inplace() ISLPP_INPLACE_FUNCTION{
  if (nPiece() == 1) {
    // Extend the only piece to universe
    auto piece = anyPiece();
    obj_give(piece.second.toPwAff_consume());
    return;
  }

  auto definedDomain = domain();
  gist_inplace(std::move(definedDomain));
}


ISLPP_EXSITU_ATTRS std::pair<Set, Aff> PwAff::anyPiece() ISLPP_EXSITU_FUNCTION{
  std::pair<Set, Aff> result;
  foreachPiece([&result](Set set, Aff aff) -> bool {
    result.first = std::move(set);
    result.second = std::move(aff);
    return true; // break, just need one piece
  });
  return result; // NRVO
}
