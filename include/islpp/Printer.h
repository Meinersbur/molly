#ifndef ISLPP_PRINTER_H
#define ISLPP_PRINTER_H

#include <llvm/Support/Compiler.h>
#include <assert.h>
#include <string>

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

  template<typename T>
  class List;
} // namespace isl



namespace isl {
  class Printer {
#pragma region Low-level
  private:
    isl_printer *printer;

  public: // Public because otherwise we had to add a lot of friends
    isl_printer *take() { assert(printer); isl_printer *result = printer; printer = nullptr; return result; }
    //isl_printer *takeCopy() const;
    isl_printer *keep() const { return printer; }
  protected:
    void give(isl_printer *printer);
  public:
    static Printer wrap(isl_printer *printer) { Printer result; result.give(printer); return result; }
#pragma endregion

  private:

    Printer() LLVM_DELETED_FUNCTION;
    Printer(const Printer &) LLVM_DELETED_FUNCTION;
    const Printer &operator=(const Printer &) LLVM_DELETED_FUNCTION;


  public:
    ~Printer();

    //IslPrinter(const IslPrinter &printer) { isl_printer_cop }
    Printer(Printer &&that) { this->printer = that.take(); }
    //const IslPrinter &operator=(const IslPrinter &) {}
    const Printer &operator=(Printer &&that) { assert(!this->printer); this->printer = that.take(); return *this; }

#pragma region Creational
    Printer createToFile(Ctx *ctx, FILE *file);
    Printer createToStr(Ctx *ctx);
#pragma endregion

    void print(llvm::raw_ostream &out) const { }
    std::string toString() const;
    void dump() const;


    char*getString() const;
    FILE *getFile() const;
    int getOutputFormat() const;

    void setOutputFormat(int output_format);
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

    void flush();
  }; /* class Printer */
} /* namesapce molly */
#endif /* ISLPP_PRINTER_H */