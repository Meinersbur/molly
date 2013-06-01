#ifndef ISLPP_INT_H
#define ISLPP_INT_H

#include "islpp_common.h"
#include <isl/int.h>
//#include <gmpxx.h>
#include <llvm/Support/raw_ostream.h>
#include <ostream>
#include <llvm/Support/raw_os_ostream.h>


namespace isl {
  /// @brief Currently a wrapper for gmp_mpz
  /// gmp_mpz does not use reference counting, so some operations may do unnecessay copies
  /// Solutions:
  /// - Use gmpxx.h
  /// - More rvalue references
  /// - Implement refcounting ourselves
  class Int {
  private:
    isl_int val;

#pragma region Low-level functions
  public :
    /*const (isl does not use const) */ isl_int &keep() const { return const_cast<isl_int&>(val); }
    isl_int *change() { return &val; }
    void give(const isl_int &val) { isl_int_set(this->val, val); }

    static Int wrap(const isl_int &val) { Int result; result.give(val); return result; }
#pragma endregion

  public:
    Int() {
      isl_int_init(this->val);
    }
    /* implicit */ Int(const Int &v) {
      isl_int_init(this->val);
      isl_int_set(this->val, v.keep());
    }
    /* implicit */ Int(Int &&v) {
      // Not sure whether this is a good idea
      memcpy(&val, &v.val, sizeof(val));
      memset(&v.val, 0, sizeof(v.val));
    }
    ~Int() {
      isl_int_clear(val);
    }

    const Int &operator=(const Int &v) {
      isl_int_set(this->val, v.val);
      return *this;
    }
    const Int &operator=(Int &&v) {
      isl_int_swap(this->val, v.keep());
      return *this;
    }

    /* implicit */ Int(signed long int v) {
      isl_int_init(this->val);
      isl_int_set_si(this->val, v);
    }
    const Int &operator=(signed long int v) {
      isl_int_set_si(this->val, v);
      return *this;
    }

    /* implicit */ Int(unsigned long int v) {
      isl_int_init(this->val);
      isl_int_set_ui(this->val, v);
    }
    const Int &operator=(unsigned long int v) {
      isl_int_set_ui(this->val, v);
      return *this;
    }

    // Otherwise the compiler will complain about ambiguity
    /* implicit */ Int(int v) {
      isl_int_init(this->val);
      isl_int_set_si(this->val, v);
    }
    const Int &operator=(int v) {
      isl_int_set_si(this->val, v);
      return *this;
    }

    void print(llvm::raw_ostream &out, int base = 10) const {
      char *s = mpz_get_str(0, base, keep());
      out << s;
      isl_int_free_str(s);
    }
    void print(std::ostream &out, int base = 10) const {
      llvm::raw_os_ostream raw(out);
      print(raw);
    }
    std::string toString() const{
      std::string buf;
      llvm::raw_string_ostream stream(buf);
      return stream.str();
    }
    void dump() const {
      print(llvm::errs());
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

    bool isZero() {
      return isl_int_is_zero(val);
    }

    bool isOne() {
      return isl_int_is_one(val);
    }

    bool isNeg() {
      return isl_int_is_neg(val);
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



  static inline bool operator<(const Int &lhs, const Int &rhs) {
    return isl_int_lt(lhs.keep(), rhs.keep());
  }


  static inline bool operator>(const Int &lhs, const Int &rhs) {
    return isl_int_gt(lhs.keep(), rhs.keep());
  }

  static inline Int operator-(const Int &lhs, const Int &rhs) {
    Int result;
    isl_int_sub(*result.change(), lhs.keep(), rhs.keep());
    return result;
  }


  static inline Int operator/(const Int &lhs, unsigned long int rhs) {
    Int result;
    mpz_tdiv_q_ui(*result.change(), lhs.keep(), rhs);
    return result;
  }
} // namespace isl


static inline std::ostream& operator<<(std::ostream &os, const isl::Int &val) {
  val.print(os);
  return os;
}


static inline llvm::raw_ostream& operator<<(llvm::raw_ostream &out, const isl::Int &val) {
  val.print(out);
  return out;
}


#endif /* ISLPP_INT_H */
