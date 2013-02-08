#include "islpp/printer.h"

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

#include <llvm/Support/raw_ostream.h>

#include <isl/printer.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/aff.h>
#include <isl/polynomial.h>

using namespace isl;
using namespace llvm;
using namespace std;


Printer::~Printer() {
  if (printer)
    isl_printer_free(printer);
}

std::string Printer::toString() const { 
  std::string buf;
  llvm::raw_string_ostream stream(buf);
  return stream.str();
}


void Printer::dump() const { 
  print(llvm::errs());
}


char * Printer::getString() const {
  return isl_printer_get_str(keep());
}

FILE *Printer::getFile() const{
  return isl_printer_get_file(keep());
}
int Printer::getOutputFormat() const {
  return isl_printer_get_output_format(keep());      
}


    void Printer::setOutputFormat(int output_format){
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

    void Printer::flush(){
      give(isl_printer_flush(take()));
    }
