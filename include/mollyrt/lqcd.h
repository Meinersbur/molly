#ifndef MOLLYRT_LQCD_H
#define MOLLYRT_LQCD_H

// C++ std::complex
#include <complex>
typedef std::complex<double> complex_t;

// C99 complex
#include <math.h>
#include <complex.h>
//typedef _Complex double complex_t;  



struct su3matrix_t{
  complex_t c[3][3];

public:
  su3matrix_t() {}

  MOLLY_ATTR(pure) complex_t *operator[](size_t idx) {
    return c[idx];
  }
  MOLLY_ATTR(pure) const complex_t *operator[](size_t idx) const {
    return c[idx];
  }

  static su3matrix_t zero() {
    su3matrix_t result;
    for (auto i = 0; i < 3; i += 1)
      for (auto j = 0; j < 3; j += 1)
        result.c[i][j] = 0;
    return result; // NRVO
  }

  static su3matrix_t one() {
    su3matrix_t result = zero();
    for (auto i = 0; i < 3; i += 1)
      result.c[i][i] = 1;
    return result; // NRVO
  }

  static su3matrix_t mone() {
    su3matrix_t result = zero();
    for (auto i = 0; i < 3; i += 1)
      result.c[i][i] = -1;
    return result; // NRVO
  }

  static su3matrix_t allone() {
    su3matrix_t result;
    for (auto i = 0; i < 3; i += 1)
      for (auto j = 0; j < 3; j += 1)
        result.c[i][j] = 1;
    return result; // NRVO
  }
};


// return arg*I
MOLLY_ATTR(pure) static inline complex_t imul(complex_t arg) {
  // C++ version
  return complex_t(-arg.imag(), arg.real());
}


MOLLY_ATTR(pure) static inline complex_t conj(complex_t arg) {
  //return std::conj(c);
  return complex_t(arg.real(), -arg.imag());
}


struct su3vector_t {
  complex_t c[3];

public:
  MOLLY_ATTR(pure) su3vector_t() /*: c({ 0, 0, 0 })*/ {}
  MOLLY_ATTR(pure) su3vector_t(complex_t c0, complex_t c1, complex_t c2)  {
    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
  }

  static su3vector_t zero() {
    return su3vector_t(0, 0, 0);
  }
  static su3vector_t one_r() {
    return su3vector_t(1, 0, 0);
  }
  static su3vector_t one_g() {
    return su3vector_t(0, 1, 0);
  }
  static su3vector_t one_b() {
    return su3vector_t(0, 0, 1);
  }

  MOLLY_ATTR(pure) const complex_t &operator[](size_t idx) const {
    assert(0 <= idx && idx < 3);
    return c[idx];
  }
  MOLLY_ATTR(pure) complex_t &operator[](size_t idx)  {
    assert(0 <= idx && idx < 3);
    return c[idx];
  }

  MOLLY_ATTR(pure) const su3vector_t &operator+=(su3vector_t rhs) {
    c[0] += rhs[0];
    c[1] += rhs[1];
    c[2] += rhs[2];
    return *this;
  }
  MOLLY_ATTR(pure) const su3vector_t &operator*=(complex_t coeff) {
    c[0] *= coeff;
    c[1] *= coeff;
    c[2] *= coeff;
    return *this;
  }
  
  MOLLY_ATTR(pure) su3vector_t imul() const {
    return su3vector_t(::imul(c[0]), ::imul(c[1]), ::imul(c[2]));
  }
  
  MOLLY_ATTR(pure) su3vector_t operator-() const {
	return su3vector_t(-c[0], -c[1], -c[2]);
  }
};


MOLLY_ATTR(pure) std::ostream &operator<<(std::ostream &os, const su3vector_t &rhs) {
  os << "(" << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << ")";
  return os;
}


MOLLY_ATTR(pure) su3vector_t operator+(su3vector_t lhs, su3vector_t rhs) {
  return su3vector_t(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}
MOLLY_ATTR(pure) su3vector_t operator-(su3vector_t lhs, su3vector_t rhs) {
  return su3vector_t(lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}
MOLLY_ATTR(pure) su3vector_t operator*(complex_t coeff, su3vector_t val) {
  return su3vector_t(coeff * val[0], coeff * val[1], coeff * val[2]);  
}



class fullspinor_t;
std::ostream &operator<<(std::ostream &os, const fullspinor_t &rhs);

struct halfspinor_t {
  su3vector_t v[2];

public:
  MOLLY_ATTR(pure) halfspinor_t() {}
  MOLLY_ATTR(pure) halfspinor_t(su3vector_t v0, su3vector_t v1) {
    v[0] = v0;
    v[1] = v1;
  }

  MOLLY_ATTR(pure) const su3vector_t &operator[](size_t idx) const { return v[idx]; }
  MOLLY_ATTR(pure) su3vector_t &operator[](size_t idx)  { return v[idx]; }
  
  MOLLY_ATTR(pure) const halfspinor_t &operator*=(complex_t coeff) {
    v[0] *= coeff;
    v[1] *= coeff;
    return *this;
  }
};

MOLLY_ATTR(pure) halfspinor_t operator*(complex_t coeff, halfspinor_t val) {
  return halfspinor_t(coeff * val[0], coeff * val[1]);  
}


struct fullspinor_t {
  su3vector_t v[4];

public:
  MOLLY_ATTR(pure) fullspinor_t() {}
  MOLLY_ATTR(pure) fullspinor_t(su3vector_t v0, su3vector_t v1, su3vector_t v2, su3vector_t v3)  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }

  static fullspinor_t zero() {
    return fullspinor_t(su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());
  }
  
  MOLLY_ATTR(pure) const su3vector_t &operator[](size_t idx) const {
    return v[idx];
  }
  MOLLY_ATTR(pure) su3vector_t &operator[](size_t idx)  {
    return v[idx];
  }

  MOLLY_ATTR(pure) const fullspinor_t &operator+=(fullspinor_t rhs) {
    v[0] += rhs[0];
    v[1] += rhs[1];
    v[2] += rhs[2];
    v[3] += rhs[3];
    return *this;
  }
};
typedef fullspinor_t spinor_t;


MOLLY_ATTR(pure) std::ostream &operator<<(std::ostream &os, const fullspinor_t &rhs) {
  os << "(" << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << ", " << rhs[3] << ")";
  return os;
}

MOLLY_ATTR(pure) std::ostream &operator<<(std::ostream &os, const halfspinor_t &rhs) {
  os << "(" << rhs[0] << ", " << rhs[1] << ")";
  return os;
}

MOLLY_ATTR(pure) std::ostream &operator<<(std::ostream &os, const su3matrix_t &rhs) {
  os << "((" << rhs[0][0] << ", " << rhs[0][1] << ", " << rhs[0][2] << "),("
    << rhs[1][0] << ", " << rhs[1][1] << ", " << rhs[1][2] << "),("
    << rhs[2][0] << ", " << rhs[2][1] << ", " << rhs[2][2] << "))";
  return os;
}

typedef enum {
  DIM_T, DIM_X, DIM_Y, DIM_Z
} dimension_t;

typedef enum {
  DIR_TUP, DIR_TDN, DIR_XUP, DIR_XDN, DIR_YUP, DIR_YDN, DIR_ZUP, DIR_ZDOWN
} direction_t;

MOLLY_ATTR(pure) su3vector_t operator*(su3matrix_t m, su3vector_t v) {
  return su3vector_t(
    m.c[0][0] * v[0] + m.c[0][1] * v[1] + m.c[0][2] * v[2],
    m.c[1][0] * v[0] + m.c[1][1] * v[1] + m.c[1][2] * v[2],
    m.c[2][0] * v[0] + m.c[2][1] * v[1] + m.c[2][2] * v[2]);
}

MOLLY_ATTR(pure) halfspinor_t operator*(su3matrix_t m, halfspinor_t v) {
  return halfspinor_t(m*v[0], m*v[1]);
}


MOLLY_ATTR(pure) halfspinor_t operatormul(int64_t t, int64_t x, int64_t y, int64_t z, su3matrix_t m, halfspinor_t v) {
  return halfspinor_t(m*v[0], m*v[1]);
}


MOLLY_ATTR(pure) double norm(su3vector_t vec) {
  double result = 0;
  for (auto i = 0; i < 3; i += 1)
    result += norm(vec[i]);
  return result;
}


MOLLY_ATTR(pure) double norm(fullspinor_t spinor) {
  double result = 0;
  for (auto i = 0; i < 4; i += 1)
    result += norm(spinor[i]);
  return result;
}






MOLLY_ATTR(pure) halfspinor_t project_TUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_TDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] - spinor[2], spinor[1] - spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_XUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[3].imul(), spinor[1] + spinor[2].imul());
}

MOLLY_ATTR(pure) halfspinor_t project_XDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] - spinor[3].imul(), spinor[1] - spinor[2].imul());
}

MOLLY_ATTR(pure) halfspinor_t project_YUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[3], spinor[1] - spinor[2]);
}

MOLLY_ATTR(pure) halfspinor_t project_YDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] - spinor[3], spinor[1] + spinor[2]);
}

MOLLY_ATTR(pure) halfspinor_t project_ZUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2].imul(), spinor[1] - spinor[3].imul());
}

MOLLY_ATTR(pure) halfspinor_t project_ZDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] - spinor[2].imul(), spinor[1] + spinor[3].imul());
}


MOLLY_ATTR(pure) fullspinor_t expand_TUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_TDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], -weyl[0], -weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_XUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], -weyl[1].imul(), -weyl[0].imul());
}

MOLLY_ATTR(pure) fullspinor_t expand_XDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[1].imul(), weyl[0].imul());
}

MOLLY_ATTR(pure) fullspinor_t expand_YUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], -weyl[1], weyl[0]);
}

MOLLY_ATTR(pure) fullspinor_t expand_YDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[1], -weyl[0]);
}
 
MOLLY_ATTR(pure) fullspinor_t expand_ZUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], -weyl[0].imul(), weyl[1].imul());
}

MOLLY_ATTR(pure) fullspinor_t expand_ZDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0].imul(), -weyl[1].imul());
}

typedef int64_t coord_t;




#define _STR(X) #X
#define STR(X) _STR(X)


#endif /* MOLLYRT_LQCD_H */

