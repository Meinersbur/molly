#include "islpp/Printer.h"

#include "islpp/Ctx.h"
#include "islpp/BasicSet.h"
#include "islpp/Map.h"
#include "islpp/UnionSet.h"
#include "islpp/UnionMap.h"
#include "islpp/SetList.h"
#include "islpp/Aff.h"
#include "islpp/PwAff.h"
#include "islpp/MultiAff.h"
#include "islpp/PwMultiAff.h"
#include "islpp/UnionPwMultiAff.h"
#include "islpp/MultiPwAff.h"
#include "islpp/QPolynomial.h"
#include "islpp/PwQPolynomial.h"
#include "islpp/UnionPwQPolynomial.h"
#include "islpp/PwQPolynomialFold.h"
#include "islpp/UnionPwQPolynomialFold.h"
#include "islpp/Constraint.h"
#include "islpp/Id.h"
#include "islpp/Val.h"
#include "islpp/Point.h"
#include "islpp/AstExpr.h"
#include "islpp/AstNode.h"
#include "islpp/PwAffList.h"

#include <llvm/Support/raw_ostream.h>

#include <isl/printer.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/aff.h>
#include <isl/polynomial.h>
#include <isl/point.h>
#include <isl/ast.h>

using namespace isl;
using namespace llvm;
using namespace std;


void Printer::give(isl_printer *printer) {
  give(printer, this->ownsFile);
}


void Printer::give(isl_printer *printer, bool ownsFile) {
  if (this->printer) {
    if (this->ownsFile) {
      FILE *file = getFile();
      assert(file);
      isl_printer_free(this->printer);
      fclose(file);
    } else {
      isl_printer_free(this->printer);
    }
  }
  this->printer = printer;
  this->ownsFile = ownsFile;
} 


Printer::~Printer() {
  if (printer) {
    if (ownsFile) {
      FILE *file = getFile();
      assert(file);
      isl_printer_free(printer);
      fclose(file);
    } else {
      isl_printer_free(printer);
    }
  }
}


Printer Printer::createToFile(Ctx *ctx, FILE *file){
  return Printer::wrap(isl_printer_to_file(ctx->keep(), file), false);
}


Printer Printer::createToStr(Ctx *ctx) {
  return Printer::wrap(isl_printer_to_str(ctx->keep()), false);
}


void Printer::print(llvm::raw_ostream &out) const { 
  //TODO: Can we somehow avoid that isl makes an extra copy?
  char *pchar = isl_printer_get_str(printer);
  out << pchar;
  free(pchar);
}


std::string Printer::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  print(stream);
  return stream.str();
}


void Printer::dump() const { 
  print(llvm::errs());
}


/// same as toString
std::string Printer::getString() const {
  // This does one unnecessary copy
  char *pchar = isl_printer_get_str(keep());
  std::string result(pchar); // Does a copy
  free(pchar);
  return result;
}


FILE *Printer::getFile() const{
  return isl_printer_get_file(keep());
}
int Printer::getOutputFormat() const {
  return isl_printer_get_output_format(keep());      
}


void Printer::setOutputFormat(FormatEnum output_format){
  give(isl_printer_set_output_format(take(), output_format));
}
void Printer::setIndent(int indent) {
  give(isl_printer_set_indent(take(), indent));
}
void Printer::indent(int indent) {
  give(isl_printer_indent(take(), indent));
}
void Printer::setPrefix(const char *prefix) {
  give(isl_printer_set_prefix(take(), prefix));
}
void Printer::setSuffix(const char *suffix) {
  give(isl_printer_set_suffix(take(), suffix));
}


void Printer::print(double d){
  give(isl_printer_print_double(take(), d));
}
void Printer::print(const BasicSet &bset){
  give(isl_printer_print_basic_set(take(), bset.keep()));
}
void Printer::print(const Map &map){
  give(isl_printer_print_map(take(), map.keep()));
}
void Printer::print(const UnionSet &uset){
  give(isl_printer_print_union_set(take(), uset.keep()));
}
void Printer::print(const UnionMap &umap){
  give(isl_printer_print_union_map(take(), umap.keep()));
}


void Printer::print(const List<Set> &list){
  give(isl_printer_print_set_list(take(), list.keep()));
}
void Printer::print(const Aff &aff) {
  give(isl_printer_print_aff(take(), aff.keep()));
}
void Printer::print(const PwAff &pwaff) {
  give(isl_printer_print_pw_aff(take(), pwaff.keep()));
}
void Printer::print(const MultiAff &maff) {
  give(isl_printer_print_multi_aff(take(), maff.keep()));
}
void Printer::print(const PwMultiAff &pma) {
  give(isl_printer_print_pw_multi_aff(take(), pma.keep()));
}
void Printer::print(const UnionPwMultiAff &upma){
  give(isl_printer_print_union_pw_multi_aff(take(), upma.keep()));
}
void Printer::print(const MultiPwAff &mpa){
  give(isl_printer_print_multi_pw_aff(take(), mpa.keep()));
}
void Printer::print(const QPolynomial &qp) {
  give(isl_printer_print_qpolynomial(take(), qp.keep()));
}
void Printer::print(const PwQPolynomial &pwqp) {
  give(isl_printer_print_pw_qpolynomial(take(), pwqp.keep()));
}
void Printer::print(const UnionPwQPolynomial &upwqp) {
  give(isl_printer_print_union_pw_qpolynomial(take(), upwqp.keep()));
}
void Printer::print(const PwQPolynomialFold &pwf){
  give(isl_printer_print_pw_qpolynomial_fold(take(), pwf.keep()));
}
void Printer::print(const UnionPwQPolynomialFold &upwf) {
  give(isl_printer_print_union_pw_qpolynomial_fold(take(), upwf.keep()));
}
void Printer::print(const Constraint &constraint) {
  give(isl_printer_print_constraint(take(), constraint.keep()));
}
void Printer::print(const Id &id) {
  give(isl_printer_print_id(take(), id.keep()));
}
void Printer::print(const Val &v) {
  give(isl_printer_print_val(take(), v.keep()));
}
void Printer::print(const Vec &v) {
  give(isl_printer_print_vec(take(), v.keep()));
}


void Printer::print(const Space &space) {
  give(isl_printer_print_space(take(), space.keep()));
}


void Printer::print(const LocalSpace &space) {
  give(isl_printer_print_local_space(take(), space.keep()));
}


void Printer::print(const MultiVal &mval) {
  //give(isl_printer_print_multi_val(take(), mval.keep()));
}


void Printer::print(const BasicMap &space) {
  give(isl_printer_print_basic_map(take(), space.keep()));
}


void Printer::print(const Point &point) {
  give(isl_printer_print_point(take(), point.keep()));
}


void Printer::print(const AstExpr &expr) {
  give(isl_printer_print_ast_expr(take(), expr.keep()));
}


void Printer::print(const AstNode &node) {
  give(isl_printer_print_ast_node(take(), node.keep()));
}


void Printer::print(const PwAffList &list) {
  give(isl_printer_print_pw_aff_list(take(), list.keep()));
}


void Printer::print(const AffList &list) {
  give(isl_printer_print_aff_list(take(), list.keep()));
}


void Printer::flush(){
  give(isl_printer_flush(take()));
}
