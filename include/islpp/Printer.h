#ifndef ISLPP_PRINTER_H
#define ISLPP_PRINTER_H

#include "islpp_common.h"

#include "islpp/Multi.h"
#include "Pw.h"
#include "Multi.h"
#include <llvm/Support/Compiler.h>
#include <assert.h>
#include <string>
#include <isl/printer.h>
#include "Union.h"
#include "Islfwd.h"

struct isl_printer;

namespace llvm {
  class raw_ostream;
} // namespace llvm


namespace isl {

  enum FormatEnum {
    FormatIsl = ISL_FORMAT_ISL,
    FormatOmega = ISL_FORMAT_OMEGA,
    FormatC = ISL_FORMAT_C,
    FormatPolylib = ISL_FORMAT_POLYLIB,
    FormatExtPolylib = ISL_FORMAT_EXT_POLYLIB,
    FormatLatex = ISL_FORMAT_LATEX
  };
  namespace Format {
    static const FormatEnum Isl = FormatIsl;
    static const FormatEnum Omega = FormatOmega;
    static const FormatEnum C = FormatC;
    static const FormatEnum Polylib = FormatPolylib;
    static const FormatEnum ExtPolylib = FormatExtPolylib;
    static const FormatEnum Latex = FormatLatex;
  }


  class Printer {

#pragma region Low-level
  private:
    isl_printer *printer;
    bool ownsFile;

  public: // Public because otherwise we had to add a lot of friends
    isl_printer *take() { assert(printer); isl_printer *result = printer; printer = nullptr; return result; }
    //isl_printer *takeCopy() const;
    isl_printer *keep() const { return printer; }
  public:
    void give(isl_printer *printer);
    void give(isl_printer *printer, bool ownsFile);

    static Printer wrap(isl_printer *printer, bool ownsFile) { Printer result; result.give(printer, ownsFile); return result; }
#pragma endregion

  private:
    Printer(const Printer &printer) LLVM_DELETED_FUNCTION;
    const Printer &operator=(const Printer &) LLVM_DELETED_FUNCTION;
  public:
    Printer() : printer(nullptr), ownsFile(false) {};
    Printer(Printer &&printer) : printer(printer.take()), ownsFile(printer.ownsFile) {};
    ~Printer();

    const Printer &operator=(Printer &&printer) { this->give(printer.take()); this->ownsFile = printer.ownsFile; return *this; };


#pragma region Creational
    static Printer createToFile(Ctx *ctx, FILE *file);
    static Printer createToStr(Ctx *ctx);
    //TODO: This would be better if we had a Printer that prints to an llvm::raw_ostream or std::ostream
    // But we'd need to modify ISL such that it calls some callback if new chars arrive 
#pragma endregion

    //FIXME: Overloaded print with different meaning ("printTo")
    void print(llvm::raw_ostream &out) const;
    std::string toString() const;
    void dump() const;

    std::string getString() const;
    FILE *getFile() const;
    int getOutputFormat() const;

    void setOutputFormat(FormatEnum output_format);
    void setIndent(int indent); 
    void indent(int indent);
    void setPrefix(const char *prefix);
    void setSuffix(const char *suffix);

    //TODO: The overloading of print(llvm::raw_ostream) is not good
    // Maybe rename to write()
    void print(double d);
    void print(const BasicSet &bset);
    void print(const Map &map);
    void print(const UnionSet &uset);
    void print(const UnionMap &umap);
    void print(const List<Set> &list);
    void print(const Aff &aff);
    void print(const PwAff &pwaff);
    void print(const MultiAff &maff);
    void print(const PwMultiAff &pma);
    void print(const UnionPwMultiAff &upma);
    void print(const MultiPwAff &mpa);
    void print(const QPolynomial &qp);
    void print(const PwQPolynomial &pwqp);
    void print(const UnionPwQPolynomial &upwqp);
    void print(const PwQPolynomialFold &pwf);
    void print(const UnionPwQPolynomialFold &upwf);
    void print(const Constraint &constraint);
    void print(const Id &id);
    void print(const Val &v);
    void print(const Vec &v);
    void print(const Space &space);
    void print(const MultiVal &);
    void print(const BasicMap &);
    void print(const Point &point);
    void print(const AstExpr &);
    void print(const AstNode &);
    void print(const PwAffList &);
    void print(const AffList &);

    void flush();
  }; // class Printer
} // namesapce molly
#endif /* ISLPP_PRINTER_H */
