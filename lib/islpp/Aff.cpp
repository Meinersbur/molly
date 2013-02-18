#include "islpp/Aff.h"

#include "islpp/Ctx.h"
#include "islpp/Printer.h"
#include "islpp/Space.h"
#include "islpp/LocalSpace.h"
#include "islpp/Int.h"
#include "islpp/Id.h"
#include "islpp/Set.h"
#include "islpp/MultiAff.h"

#include <isl/aff.h>
#include <llvm/Support/raw_ostream.h>

using namespace isl;
using namespace std;


isl_aff *Aff::takeCopy() const {
  return isl_aff_copy(keep());
}


void Aff::give(isl_aff *aff) {
  if (this->aff)
    isl_aff_free(this->aff);
  this->aff = aff;
}


Aff::~Aff(void) {
  if (this->aff)
    isl_aff_free(this->aff);
}


Aff Aff::createZeroOnDomain(LocalSpace &&space) {
  return Aff::wrap(isl_aff_zero_on_domain(space.take()));
}
Aff Aff::createVarOnDomain(LocalSpace &&space, isl_dim_type type, unsigned pos) {
  return Aff::wrap(isl_aff_var_on_domain(space.take(), type, pos));
}

Aff Aff::readFromString(Ctx *ctx, const char *str) {
  return Aff::wrap(isl_aff_read_from_str(ctx->keep(), str));
}


void Aff::print(llvm::raw_ostream &out) const{
  auto printer = Printer::createToStr(getCtx());
  printer.print(*this);
  out << printer.getString();
}
std::string Aff::toString() const{
  std::string buf;
  llvm::raw_string_ostream out(buf);
  print(out);
  return out.str();
}
void Aff::dump() const{
  print(llvm::errs());
}
void Aff::printProperties(llvm::raw_ostream &out, int depth, int indent) const {
  if (depth > 0) {
    print(out);
  } else {
    out << "...";
  }
}


Ctx *Aff::getCtx() const {
  return Ctx::wrap(isl_aff_get_ctx(keep()));
}

int Aff::dim(isl_dim_type type) const{
  return isl_aff_dim(keep(), type);
}
bool Aff::involvesDims(isl_dim_type type, unsigned first, unsigned n) const{
  return isl_aff_involves_dims(keep(), type, first, n);
}
Space Aff::getDomainSpace() const {
  return Space::wrap(isl_aff_get_domain_space(keep()));
}
Space Aff::getSpace() const {
  return Space::wrap(isl_aff_get_space(keep()));
}
LocalSpace Aff::getDomainLocalSpace() const{
  return LocalSpace::wrap(isl_aff_get_domain_local_space(keep()));
}
LocalSpace Aff::getLocalSpace() const {
  return LocalSpace::wrap(isl_aff_get_local_space(keep()));
}

const char *Aff::getDimName( isl_dim_type type, unsigned pos) const {
  return isl_aff_get_dim_name(keep(), type, pos);
}
Int Aff::getConstant() const {
  Int result;
  isl_aff_get_constant(keep(), result.change());
  return result;
}
Int Aff::getCoefficient(isl_dim_type type, unsigned pos) const {
  Int result;
  isl_aff_get_coefficient(keep(), type, pos, result.change());
  return result;
}
Int Aff::getDenominator() const {
  Int result;
  isl_aff_get_denominator(keep(), result.change());
  return result;
}

void Aff::setConstant(const Int &v) {
  give(isl_aff_set_constant(take(), v.keep())); 
}
void Aff::setCoefficient(isl_dim_type type, unsigned pos, int v) {
  give(isl_aff_set_coefficient_si(take(), type, pos, v));
}
void Aff::setCoefficient(isl_dim_type type, unsigned pos, const Int &v) {
  give(isl_aff_set_coefficient(take(), type, pos, v.keep()));
}
void Aff::setDenominator(const Int &v) {
  give(isl_aff_set_denominator(take(), v.keep()));
}

void Aff::addConstant(const Int &v) {
  give(isl_aff_add_constant(take(), v.keep()));
}
void Aff::addConstant(int v) {
  give(isl_aff_add_constant_si(take(), v));
}
void Aff::addConstantNum(const Int &v) {
  give(isl_aff_add_constant_num(take(), v.keep()));
}
void Aff::addConstantNum(int v) {
  give(isl_aff_add_constant_num_si(take(), v));
}
void Aff::addCoefficient(isl_dim_type type, unsigned pos, int v) {
  give(isl_aff_add_coefficient_si(take(), type, pos, v));
}
void Aff::addCoefficient(isl_dim_type type, unsigned pos, const Int &v) {
  give(isl_aff_add_coefficient(take(), type, pos, v.keep()));
}

bool Aff::isCst() const {
  return isl_aff_is_cst(keep());
}

void Aff::setDimName(isl_dim_type type, unsigned pos, const char *s) {
  give(isl_aff_set_dim_name(take(), type, pos, s));
}
void Aff::setDimId(isl_dim_type type, unsigned pos, Id &&id) {
  give(isl_aff_set_dim_id(take(), type, pos, id.take()));
}

bool Aff::isPlainZero() const {
  return isl_aff_plain_is_zero(keep());
}
Aff Aff::getDiv(int pos) const {
  return Aff::wrap(isl_aff_get_div(keep(), pos));
}

void Aff::neg() {
  give(isl_aff_neg(take()));
}
void Aff::ceil() {
  give(isl_aff_ceil(take()));
}
void Aff::floor() {
  give(isl_aff_floor(take()));
}
void Aff::mod(const Int &v) {
  give(isl_aff_mod(take(), v.keep()));
}

void Aff::scale(const Int &f) {
  give(isl_aff_scale(take(), f.keep()));
}
void Aff::scaleDown(const Int &f) {
  give(isl_aff_scale_down(take(), f.keep()));
}
void Aff::scaleDown(unsigned f) {
  give(isl_aff_scale_down_ui(take(), f));
}

void Aff::insertDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_aff_insert_dims(take(), type, first, n));
}
void Aff::addDims(isl_dim_type type, unsigned n) {
  give(isl_aff_add_dims(take(), type, n));
}
void Aff::dropDims(isl_dim_type type, unsigned first, unsigned n) {
  give(isl_aff_drop_dims(take(), type, first, n));
}

void Aff::projectDomainOnParams() {
  give(isl_aff_project_domain_on_params(take()));
}

void Aff::alignParams(Space &&model) {
  give(isl_aff_align_params(take(), model.take()));
}
void Aff::gist(Set &&context) {
  give(isl_aff_gist(take(), context.take()));
}
void Aff::gistParams(Set &&context) {
  give(isl_aff_gist_params(take(), context.take()));
}

void Aff::pullbackMultiAff(MultiAff &&ma) {
  give(isl_aff_pullback_multi_aff(take(), ma.take()));
}
