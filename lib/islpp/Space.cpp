#include "islpp/Space.h"

#include "islpp/Ctx.h"
#include "islpp/Id.h"
#include "islpp/Set.h"
#include "islpp/BasicSet.h"
#include "islpp/Map.h"
#include "islpp/BasicMap.h"
#include "cstdiofile.h"
#include "islpp/MultiPwAff.h"
#include "islpp/Point.h"
#include "islpp/Id.h"
#include "islpp/Constraint.h"
#include "islpp/UnionMap.h"
#include "islpp/AstBuild.h"

#include <isl/space.h>
#include <isl/set.h>
#include <isl/map.h>

using namespace isl;
using namespace std;

#if 0
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
#endif

void Space::print(llvm::raw_ostream &out) const {
  auto printer = isl::Printer::createToStr(getCtx());
  printer.print(*this);
  printer.print(out);
}

#if 0
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
#endif

Space Space::createMapSpace(const Ctx *ctx, unsigned nparam, unsigned n_in, unsigned n_out) {
  return Space::enwrap(isl_space_alloc(ctx->keep(), nparam, n_in, n_out));
}


Space Space::createParamsSpace(const Ctx *ctx, unsigned nparam) {
  return Space::enwrap(isl_space_params_alloc(ctx->keep(), nparam));
}


Space Space:: createSetSpace(const Ctx *ctx, unsigned nparam, unsigned dim) {
  return Space::enwrap(isl_space_set_alloc(ctx->keep(), nparam, dim));
}


Space Space::createMapFromDomainAndRange(Space &&domain, Space &&range) {
  return Space::enwrap(isl_space_map_from_domain_and_range(domain.take(), range.take()));
}


BasicMap Space::emptyBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_empty(takeCopy()));
}


BasicMap Space::universeBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(takeCopy()));
}


    BasicMap Space::equalBasicMap(unsigned n_equal) const {
      return BasicMap::enwrap(isl_basic_map_equal(takeCopy(), n_equal));
    }

    
    BasicMap Space::lessAtBasicMap(unsigned pos) const {
        return BasicMap::enwrap(isl_basic_map_less_at(takeCopy(), pos));
    }

    
    BasicMap Space::moreAtBasicMap(unsigned pos) const {
      return BasicMap::enwrap(isl_basic_map_more_at(takeCopy(), pos));
    }


Map Space::emptyMap() const {
  return Map::enwrap(isl_map_empty(takeCopy()));
}


Map Space::universeMap() const {
  return Map::enwrap(isl_map_universe(takeCopy()));
}


 Map Space::lexLtMap() const {
   return Map::enwrap(isl_map_lex_lt(takeCopy()));
 }


     Map Space::lexGtMap() const {
         return Map::enwrap(isl_map_lex_gt(takeCopy()));
     }


 Map Space::lexLtFirstMap(unsigned pos) const {
   return Map::enwrap(isl_map_lex_lt_first(takeCopy(), pos));
 }


    Map Space::lexGtFirstMap(unsigned pos) const {
         return Map::enwrap(isl_map_lex_gt_first(takeCopy(), pos));
    }


Aff Space::createZeroAff() const {
  return Aff::enwrap(isl_aff_zero_on_domain(isl_local_space_from_space(takeCopy())));
}


Aff Space::createConstantAff(const Int &c) const {
  auto zero = createZeroAff();
  zero.setConstant(c);
  return zero;
}


Aff Space::createVarAff(isl_dim_type type, unsigned pos) const {
  assert(isSetSpace());
  return Aff::enwrap(isl_aff_var_on_domain(isl_local_space_from_space(takeCopy()), type, pos)); 
}

 PwAff Space::createEmptyPwAff() const {
   return PwAff::enwrap(isl_pw_aff_empty(takeCopy()));
 }


 PwAff Space::createZeroPwAff() const {
return PwAff::enwrap(isl_pw_aff_zero_on_domain(isl_local_space_from_space(takeCopy())));
 }


MultiAff Space::createZeroMultiAff() const {
  return MultiAff::enwrap(isl_multi_aff_zero(takeCopy()));
}


MultiPwAff Space::createZeroMultiPwAff() const {
  return MultiPwAff::enwrap(isl_multi_pw_aff_zero(takeCopy()));
}


 PwMultiAff Space::createEmptyPwMultiAff() const {
   return PwMultiAff::enwrap(isl_pw_multi_aff_empty(takeCopy()));
 }


Point Space::zeroPoint() const {
  return Point::enwrap(isl_point_zero(keep()));
}

Constraint Space::createZeroConstraint() const {
  return Constraint::enwrap(isl_equality_alloc(isl_local_space_from_space(takeCopy())));
}

Constraint Space::createConstantConstraint(int v) const {
  auto result = isl_equality_alloc(isl_local_space_from_space(takeCopy()));
  result = isl_constraint_set_constant_si(result, v);
  return Constraint::enwrap(result);
}
Constraint Space::createVarConstraint(isl_dim_type type, int pos) const {
  auto result = isl_equality_alloc(isl_local_space_from_space(takeCopy()));
  result = isl_constraint_set_coefficient_si(result, type, pos, 1);
  return Constraint::enwrap(result);
}


Constraint Space::createEqualityConstraint() const {
  return Constraint::enwrap(isl_equality_alloc(isl_local_space_from_space(takeCopy())));
}


Constraint Space::createInequalityConstraint() const {
  return Constraint::enwrap(isl_inequality_alloc(isl_local_space_from_space(takeCopy())));
}


Constraint Space::createLtConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  return createGeConstraint(std::move(lhs), std::move(rhs));
}
Constraint Space::createLeConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  return createGtConstraint(std::move(lhs), std::move(rhs));
}
Constraint Space::createEqConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take());
  return Constraint::enwrap(isl_equality_from_aff(term));
}

Constraint Space::createEqConstraint(Aff &&lhs, int rhs) const {
  return createEqConstraint(std::move(lhs), lhs.getLocalSpace().createConstantAff(rhs));
}


  Constraint Space::createEqConstraint(Aff &&lhs, isl_dim_type type, unsigned pos) const {
    auto term = isl_equality_from_aff(lhs.take());
    isl_int coeff;
    isl_int_init(coeff);
    isl_constraint_get_coefficient(term, type, pos, &coeff);
    isl_int_sub_ui(coeff, coeff, 1);
    isl_constraint_set_coefficient(term, type, pos, coeff);
    isl_int_clear(coeff);
    return Constraint::enwrap(term);
  }


Constraint Space::createGeConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs >= 0
  auto c = isl_inequality_from_aff(term); // TODO: Confirm
  return Constraint::enwrap(c);
}
Constraint Space::createGtConstraint(Aff &&lhs, Aff &&rhs) const {
  assert(isEqual(lhs.getLocalSpace(), rhs.getLocalSpace()));
  auto term = isl_aff_sub(lhs.take(), rhs.take()); // lhs - rhs > 0
  term = isl_aff_add_constant_si(term, -1); // lhs - rhs - 1 >= 0
  auto c = isl_inequality_from_aff(term);
  return Constraint::enwrap(c);
}


Expr Space::createVarExpr(isl_dim_type type, int pos) const {
  return Expr::createVar(this->asLocalSpace(), type, pos);
}

Set Space::emptySet() const {
  assert(dim(isl_dim_in) == 0);
  return Set::enwrap(isl_set_empty(takeCopy()));
}


Set Space::universeSet() const {
  assert(dim(isl_dim_in) == 0);
  return Set::enwrap(isl_set_universe(takeCopy()));
}


BasicSet Space::emptyBasicSet() const {
  assert(isSetSpace());
  return BasicSet::enwrap(isl_basic_set_empty(takeCopy()));
}


BasicSet Space::universeBasicSet() const {
  assert(isSetSpace());
  return BasicSet::enwrap(isl_basic_set_universe(takeCopy()));
}


bool Space::isParamsSpace() const {
  assert(1 == isl_space_is_params(keep()) + isl_space_is_set(keep()) + isl_space_is_map(keep()));
  return isl_space_is_params(keep());
}
bool Space::isSetSpace() const {
  assert(1 == isl_space_is_params(keep()) + isl_space_is_set(keep()) + isl_space_is_map(keep()));
  return isl_space_is_set(keep());
}
bool Space::isMapSpace() const {
  assert(1 == isl_space_is_params(keep()) + isl_space_is_set(keep()) + isl_space_is_map(keep()));
  return isl_space_is_map(keep());
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
  return Set::enwrap(isl_set_universe(takeCopy()));
}

BasicSet Space::createUniverseBasicSet() const {
  return BasicSet::wrap(isl_basic_set_universe(takeCopy()));
}

Map Space::createUniverseMap() const {
  return Map::enwrap(isl_map_universe(takeCopy()));
}

BasicMap Space::createUniverseBasicMap() const {
  return BasicMap::enwrap(isl_basic_map_universe(takeCopy()));
}


UnionMap Space::createEmptyUnionMap() const {
  return UnionMap::enwrap(isl_union_map_empty(takeCopy()));
}


LocalSpace Space::asLocalSpace() const {
  return LocalSpace::wrap(isl_local_space_from_space(takeCopy()));
}


 AstBuild Space::createAstBuild() const {
   return AstBuild::enwrap(isl_ast_build_from_context(isl_set_universe(takeCopy())));
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
  return Space::enwrap(isl_space_join(left.take(), right.take()));
}
Space isl::alignParams(Space &&space1, Space &&space2){
  return Space::enwrap(isl_space_align_params(space1.take(), space2.take()));
}


Space isl::setTupleId(Space &&space, isl_dim_type type, Id &&id) {
  return Space::enwrap(isl_space_set_tuple_id(space.take(), type, id.take()));
}
Space isl::setTupleId(Space &&space, isl_dim_type type, const Id &id) {
  return Space::enwrap(isl_space_set_tuple_id(space.take(), type, id.takeCopy()));
}
Space isl::setTupleId(const Space &space, isl_dim_type type, Id &&id) {
  return Space::enwrap(isl_space_set_tuple_id(space.takeCopy(), type, id.take()));
}
Space isl::setTupleId(const Space &space, isl_dim_type type, const Id &id) {
  return Space::enwrap(isl_space_set_tuple_id(space.takeCopy(), type, id.takeCopy()));
}
