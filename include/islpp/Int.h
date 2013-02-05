#ifndef ISLPP_INT_H
#define ISLPP_INT_H

#include <isl/int.h>
#include <gmpxx.h>
#include <llvm/Support/raw_ostream.h>

namespace isl {

  class Int {
  private:
    isl_int val;

#pragma region Low-level functions
  public :
    /*const (isl does not use const) */ isl_int &keep() const { return const_cast<isl_int&>(val); }
    isl_int *change() { return &val; }
#pragma endregion

  public:
    Int() {
      isl_int_init(this->val);
    }
    ~Int() {
      isl_int_clear(val);
    }

    void print(llvm::raw_ostream &out, int base = 10) const {
      char *s = mpz_get_str(0, base, keep());
      out << s;
      isl_int_free_str(s);
    }
    std::string toString() const{
      std::string buf;
      llvm::raw_string_ostream stream(buf);
      return stream.str();
    }
    void dump() const {
      print(llvm::errs());
    }


    /* implicit */ Int (const Int &val) {
      isl_int_init(this->val);
      isl_int_set(this->val, val.keep());
    }
    Int &operator=(const Int &val) {
      isl_int_set(this->val, val.keep());
    }

    /* implicit */ Int(signed long int val) {
      isl_int_init(this->val);
      isl_int_set_si(this->val, val);
    }
    Int &operator=(signed long int val) {
      isl_int_set_si(this->val, val);
    }

    /* implicit */ Int(unsigned long int val) {
      isl_int_init(this->val);
      isl_int_set_ui(this->val, val);
    }
    Int &operator=(unsigned long int val) {
      isl_int_set_ui(this->val, val);
    }


    signed long int asSi() const {
      return isl_int_get_si(keep());
    }
    unsigned long int asUi() const {
      return isl_int_get_ui(keep());
    }


    void abs() {
      isl_int_abs(val, val);
    }

  }; // class Int


  inline bool operator==(const Int &lhs, const Int &rhs) {
    return isl_int_eq(lhs.keep(), rhs.keep());
  }

  inline Int abs(const Int &arg) {
    Int result;
    mpz_abs(*result.change(), arg.keep());
    return result;
  }


} // namespace isl
#endif /* ISLPP_INT_H */