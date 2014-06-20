
#ifdef WITH_MOLLY
#include <molly.h>
#else /* WITH_MOLLY */
#include <molly_emulation.h>
#endif /* WITH_MOLLY */

#include <bench.h>

#include <iostream>
#include <cassert>

// C++ std::complex
#include <complex>
typedef std::complex<double> complex;

// C99 complex
#include <math.h>
#include <complex.h>
//typedef _Complex double complex; 



struct su3matrix_t{
  complex c[3][3];

public:
  su3matrix_t() {}

  MOLLY_ATTR(pure) complex *operator[](size_t idx) {
    return c[idx];
  }
  MOLLY_ATTR(pure) const complex *operator[](size_t idx) const {
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
MOLLY_ATTR(pure) static inline complex imul(complex arg) {
  // C++ version
  return complex(-arg.imag(), arg.real());
}


MOLLY_ATTR(pure) static inline complex conj(complex c) {
  return std::conj(c);
}


struct su3vector_t {
  complex c[3];

public:
  MOLLY_ATTR(pure) su3vector_t() /*: c({ 0, 0, 0 })*/ {}
  MOLLY_ATTR(pure) su3vector_t(complex c0, complex c1, complex c2)  {
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

  MOLLY_ATTR(pure) const complex &operator[](size_t idx) const {
    assert(0 <= idx && idx < 3);
    return c[idx];
  }
  MOLLY_ATTR(pure) complex &operator[](size_t idx)  {
    assert(0 <= idx && idx < 3);
    return c[idx];
  }

  MOLLY_ATTR(pure) const su3vector_t &operator+=(su3vector_t rhs) {
    c[0] += rhs[0];
    c[1] += rhs[1];
    c[2] += rhs[2];
    return *this;
  }
};


MOLLY_ATTR(pure) std::ostream &operator<<(std::ostream &os, const su3vector_t &rhs) {
  os << "(" << rhs[0] << ", " << rhs[1] << ", " << rhs[2] << ")";
  return os;
}


MOLLY_ATTR(pure) su3vector_t operator+(su3vector_t lhs, su3vector_t rhs) {
  return su3vector_t(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
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
};
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
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_XUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_XDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_YUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_YDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_ZUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

MOLLY_ATTR(pure) halfspinor_t project_ZDN(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}


MOLLY_ATTR(pure) fullspinor_t expand_TUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_TDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_XUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_XDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_YUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_YDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_ZUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

MOLLY_ATTR(pure) fullspinor_t expand_ZDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

typedef int64_t coord_t;



#define _STR(X) #X
#define STR(X) _STR(X)

#ifndef LT
#define L  16
#define LT L
#define LX L
 
#define B 8
#define BT B
#define BX B

#define P 2
#define PT P
#define PX P
#endif

#define sBT STR(BT)
#define sBX STR(BX)
#define sBY STR(BY)
#define sBZ STR(BZ)

#define sPT STR(PT)
#define sPX STR(PX)
#define sPY STR(PY)
#define sPZ STR(PZ)

#ifndef WITH_KAMUL
//#error Must define WITH_KAMUL to 0 or 1!
#define WITH_KAMUL 0
#endif


#pragma molly transform("{ [t,x,d] -> [node[pt,px] -> local[t,x,d]] : 0<=pt<" sPT" and 0<=px<" sPX" and " sBT"pt<=t<=" sBT"*(pt+1) and " sBX"px<=x<=" sBX"*(px+1) }")
molly::array<su3matrix_t, LT + 1, LX + 1, 4> gauge;
#define GAUGE_MOD(divident,divisor) divident

#pragma molly transform("{ [t,x] -> [node[floor(t/" sBT"),floor(x/" sBX")] -> local[floor(t/2),x,t%2]] }")
molly::array<spinor_t, LT, LX> source, sink;



extern "C" MOLLY_ATTR(process) void HoppingMatrix() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1) {
          fullspinor_t result;
	  
          // T+
	  {
	    auto halfspinor = project_TUP(source[molly::mod(t + 1, LT)][x]);
	    halfspinor = gauge[GAUGE_MOD(t + 1, LT)][x][DIM_T] * halfspinor;
#if WITH_KAMUL
	    halfspinor *= ka[0];
#endif
            result = expand_TUP(halfspinor);
	  }

          // T-
	  {
	    auto halfspinor = project_TDN(source[molly::mod(t - 1, LT)][x]);
	    halfspinor = gauge[t][x][DIM_T] * halfspinor;
#if WITH_KAMUL
	    halfspinor *= conj(ka[0]);
#endif
            result += expand_TUP(halfspinor);
	  }
          

          // X+
	  {
	    auto halfspinor = project_XUP(source[t][molly::mod(x + 1, LX)]);
	    halfspinor = gauge[t][GAUGE_MOD(x + 1, LX)][DIM_X] * halfspinor;
#if WITH_KAMUL
	    halfspinor *= ka[1];
#endif
            result = expand_XUP(halfspinor);
	  }
          
          // X-
	  {
	    auto halfspinor = project_XDN(source[t][molly::mod(x - 1, LX)]);
	    halfspinor = gauge[t][x][DIM_X] * halfspinor;
#if WITH_KAMUL
	    halfspinor *= conj(ka[1]);
#endif
            result += expand_XUP(halfspinor);
	  }
	  

      // Writeback
      sink[t][x] = result;
    }
} // void HoppingMatrix()



MOLLY_ATTR(pure) spinor_t initSpinorVal(coord_t t, coord_t x) {
  if (t == 0 && x == 0)
    return spinor_t(su3vector_t::one_b(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());

  return spinor_t::zero();
}


MOLLY_ATTR(pure) su3matrix_t initGaugeVal(coord_t t, coord_t x, direction_t dir) {
  t = molly::mod(t,LT);
  x = molly::mod(x,LX);

  if (t == 0 && x == 0)
    return su3matrix_t::mone();

  return su3matrix_t::zero();
}


MOLLY_ATTR(pure) void checkSpinorVal(coord_t t, coord_t x, spinor_t val) {
}


extern "C" MOLLY_ATTR(process) void init() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      source[t][x] = initSpinorVal(t, x);


  for (coord_t t = 0; t < gauge.length(0); t += 1)
    for (coord_t x = 0; x < gauge.length(1); x += 1)
      for (coord_t d = 0; d < gauge.length(2); d += 1)
        gauge[t][x][d] = initGaugeVal(t, x, static_cast<direction_t>(2 * d));
}




extern "C" MOLLY_ATTR(process) double reduce() {
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      checkSpinorVal(t, x, source[t][x]);

  double result = 0;
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      result += norm(sink[t][x]);
  return result;
}


static void bench() {
   int nTests = 10;
   int nRounds = 10;
   
  molly::exec_bench([nRounds] (int k, molly::bgq_hmflags flags) {
    for (auto i=0; i<nRounds;i+=1) {
      HoppingMatrix();
    }
  }, nTests, LT*LX, /*operator+=*/3*(4*3*2) + 4*(/*project*/2*3*2 + /*su3mm*/2*(9*(2+4)+6*2)) );
}


int main(int argc, char *argv[]) {
  init();

  HoppingMatrix();

  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << result << '\n';

  bench();
  
  return 0;
}
