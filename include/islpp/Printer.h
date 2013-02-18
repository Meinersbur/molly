#ifndef ISLPP_PRINTER_H
#define ISLPP_PRINTER_H

#include <llvm/Support/Compiler.h>
#include <assert.h>
#include <string>
#include <isl/printer.h>

struct isl_printer;

namespace llvm {
  class raw_ostream;
} // namespace llvm
namespace isl {
  class Ctx;
  class Set;
  class BasicSet;
  class Map;
  class UnionSet;
  class UnionMap;
  class Aff;
  class PwAff;
  class MultiPwAff;
  class UnionPwMultiAff;
  class PwMultiAff;
  class QPolynomial;
  class PwQPolynomial;
  class UnionPwQPolynomial;
  class PwQPolynomialFold;
  class UnionPwQPolynomialFold;
  class MultiAff;
  class Constraint;

  template<typename T>
  class List;
} // namespace isl



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
  protected:
    void give(isl_printer *printer);
    void give(isl_printer *printer, bool ownsFile);
  public:
    static Printer wrap(isl_printer *printer, bool ownsFile) { Printer result; result.give(printer, ownsFile); return result; }
#pragma endregion

  private:
    Printer(const Printer &printer) LLVM_DELETED_FUNCTION;
    const Printer &operator=(const Printer &) LLVM_DELETED_FUNCTION;
  public:
    Printer() : printer(nullptr), ownsFile(false) {};
    Printer(Printer &&printer) : printer(printer.take()), ownsFile(printer.ownsFile) {};
    ~Printer();

    const Printer &operator=(Printer &&printer) { this->give(printer.take()); this->ownsFile = printer.ownsFile; };


#pragma region Creational
    static Printer createToFile(Ctx *ctx, FILE *file);
    static Printer createToStr(Ctx *ctx);
#pragma endregion

    void print(llvm::raw_ostream &out) const { }
    std::string toString() const;
    void dump() const;

    char*getString() const;
    FILE *getFile() const;
    int getOutputFormat() const;

    void setOutputFormat(FormatEnum output_format);
    void setIndent(int indent); 
    void indent(int indent);
    void setPrefix(const char *prefix);
    void setSuffix(const char *suffix);

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

    void flush();
  }; // class Printer
} // namesapce molly
#endif /* ISLPP_PRINTER_H */