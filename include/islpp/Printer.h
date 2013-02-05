#ifndef ISLPP_PRINTER_H
#define ISLPP_PRINTER_H

#include <llvm/Support/Compiler.h>
#include <assert.h>
#include <string>

struct isl_printer;

namespace llvm {
  class raw_ostream;
}


namespace isl {


  class Printer {
  private:
    isl_printer *printer;

    Printer() LLVM_DELETED_FUNCTION;
     Printer(const Printer &) LLVM_DELETED_FUNCTION;
    const Printer &operator=(const Printer &) LLVM_DELETED_FUNCTION;

  protected:
    Printer(isl_printer *printer) { give(printer); }
    static Printer wrap(isl_printer *printer) { return Printer(printer); }

    isl_printer *take() { assert(printer); isl_printer *result = printer; printer = nullptr; return result; }
    isl_printer *keep() const { return printer; }
    void give(isl_printer *printer) { assert(!this->printer); this->printer = printer; }

  public:
    ~Printer();

    //IslPrinter(const IslPrinter &printer) { isl_printer_cop }
    Printer(Printer &&that) { this->printer = that.take(); }
    //const IslPrinter &operator=(const IslPrinter &) {}
    const Printer &operator=(Printer &&that) { assert(!this->printer); this->printer = that.take(); return *this; }

    void print(llvm::raw_ostream &out) const { }
    std::string toString() const;
    void dump() const;

  }; /* class IslPrinter */
} /* namesapce molly */

#endif /* ISLPP_PRINTER_H */