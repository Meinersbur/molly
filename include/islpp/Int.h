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
#ifndef NDEBUG
    std::string _printed;
#endif
    isl_int val;

#pragma region Low-level functions
  public :
    /*const (isl does not use const) */ isl_int &keep() const { return const_cast<isl_int&>(val); }
    isl_int *change() { 
#ifndef NDEBUG
      _printed.clear(); /*do not control what happens after return*/ 
#endif
      return &val; 
    }
    void give(const isl_int &val) { 
      isl_int_set(this->val, val); 
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }

    void updated() {
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }

    static Int wrap(const isl_int &val) { Int result; result.give(val); return result; }
#pragma endregion

  public:
    Int() {
      isl_int_init(this->val);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }
    /* implicit */ Int(const Int &v) {
      isl_int_init(this->val);
      isl_int_set(this->val, v.keep());
#ifndef NDEBUG
      this->_printed = v._printed;
#endif
    }
    /* implicit */ Int(Int &&v) {
      isl_int_init(this->val);
      isl_int_swap(this->val, v.val);
#ifndef NDEBUG
      this->_printed = std::move(v._printed);
#endif
    }
    ~Int() {
      isl_int_clear(val);
#ifndef NDEBUG
      _printed.clear();
#endif
    }

    const Int &operator=(const Int &v) {
      isl_int_set(this->val, v.val);
#ifndef NDEBUG
      this->_printed = v._printed;
#endif
      return *this;
    }
    const Int &operator=(Int &&v) {
      isl_int_swap(this->val, v.keep());
#ifndef NDEBUG
      this->_printed = std::move(v._printed);
#endif
      return *this;
    }

    /* implicit */ Int(signed long int v) {
      isl_int_init(this->val);
      isl_int_set_si(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }
    const Int &operator=(signed long int v) {
      isl_int_set_si(this->val, v);
      return *this;
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }

    /* implicit */ Int(unsigned long int v) {
      isl_int_init(this->val);
      isl_int_set_ui(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }
    const Int &operator=(unsigned long int v) {
      isl_int_set_ui(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
      return *this;
    }

    // Otherwise the compiler will complain about ambiguity
    /* implicit */ Int(int v) {
      isl_int_init(this->val);
      isl_int_set_si(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }
    const Int &operator=(int v) {
      isl_int_set_si(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
      return *this;
    }

    /*implicit*/ Int(unsigned int v) {
      isl_int_init(this->val);
      isl_int_set_ui(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
    }
    const Int &operator=(unsigned int v) {
      isl_int_set_ui(this->val, v);
#ifndef NDEBUG
      llvm::raw_string_ostream os(_printed);
      print(os);
#endif
      return *this;
    }

    void print(llvm::raw_ostream &out, int base = 10) const;
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

    Int &operator+=(const Int &that) {
      isl_int_add(this->val, this->val, that.val);
      this->updated();
      return *this;
    }

    Int &operator-=(const Int &that) {
      isl_int_sub(this->val, this->val, that.val);
      this->updated();
      return *this;
    }

  }; // class Int


  static inline bool operator==(const Int &lhs, const Int &rhs) {
    return isl_int_eq(lhs.keep(), rhs.keep());
  }

  static inline bool operator==(const Int &lhs, int rhs) { 
    return isl_int_cmp_si(lhs.keep(), rhs) == 0;
  }
  static inline bool operator!=(const Int &lhs, int rhs) { 
    return isl_int_cmp_si(lhs.keep(), rhs) != 0;
  }


  static inline Int abs(const Int &arg) {
    Int result;
    mpz_abs(*result.change(), arg.keep());
    result.updated();
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
    result.updated();
    return result;
  }


  static inline Int operator/(const Int &lhs, unsigned long int rhs) {
    Int result;
    mpz_tdiv_q_ui(*result.change(), lhs.keep(), rhs);
    result.updated();
    return result;
  }


    static inline Int operator*(const Int &lhs, const Int &rhs) {
    Int result;
    isl_int_mul(*result.change(), lhs.keep(), rhs.keep()); 
    result.updated();
    return result;
  }


  static inline Int operator*(const Int &lhs, unsigned long int rhs) {
    Int result;
    isl_int_mul_ui(*result.change(), lhs.keep(), rhs); 
    result.updated();
    return result;
  }


  static inline std::ostream& operator<<(std::ostream &os, const isl::Int &val) {
    val.print(os);
    return os;
  }


  static inline llvm::raw_ostream& operator<<(llvm::raw_ostream &out, const isl::Int &val) {
    val.print(out);
    return out;
  }

} // namespace isl

#endif /* ISLPP_INT_H */
