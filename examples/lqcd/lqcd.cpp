
#ifdef WITH_MOLLY
#include <molly.h>
#else /* WITH_MOLLY */
#include <molly_emulation.h>
#endif /* WITH_MOLLY */

#include <iostream>

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
};

struct su3vector_t {
  complex c[3];

public:
  MOLLY_ATTR(pure) su3vector_t() /*: c({ 0, 0, 0 })*/ {}
#if defined(__clang__) || defined(__GNUC__)
  MOLLY_ATTR(pure) su3vector_t(complex c0, complex c1, complex c2)  : c{c0,c1,c2} {}
#else
  MOLLY_ATTR(pure) su3vector_t(complex c0, complex c1, complex c2)  {
    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
  }
#endif

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
    return c[idx]; 
  }
  MOLLY_ATTR(pure) complex &operator[](size_t idx)  { 
    return c[idx];
  }

  MOLLY_ATTR(pure) const su3vector_t &operator+=(su3vector_t rhs) {
    c[0] += rhs[0];
    c[1] += rhs[1];
    c[2] += rhs[2];
    return *this;
  }
};

MOLLY_ATTR(pure) su3vector_t operator+(su3vector_t lhs, su3vector_t rhs) {
  return su3vector_t(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[3]);
}

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
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_XUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_XDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_YUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_YDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_ZUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

MOLLY_ATTR(pure) halfspinor_t project_ZDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
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


#if 1
#if 1
#define L 12
#define LT L
#define LX L
#define LY L
#define LZ L

//#pragma molly transform("{ [t,x,y,z] -> [node[0,0,0,0] -> local[t,x,y,z]] }")
#pragma molly transform("{ [t,x,y,z] -> [node[pt,px,py,pz] -> local[t,x,y,z]] : pt=floor(t/4) and px=floor(x/4) and py=floor(y/4) and pz=floor(z/4) }")
molly::array<spinor_t, LT, LX, LY, LZ> source, sink;

//#pragma molly transform("{ [t,x,y,z,d] -> [node[0,0,0,0] -> local[t,x,y,z,d]] }")
#pragma molly transform("{ [t,x,y,z,d] -> [node[pt,px,py,pz] -> local[t,x,y,z,d]] : (pt=floor(t/4) and px=floor(x/4) and py=floor(y/4) and pz=floor(z/4)) or (pt=floor((t-1)/4) and px=floor((x-1)/4) and py=floor((y-1)/4) and pz=floor((z-1)/4)) }")
molly::array<su3matrix_t, LT, LX, LY, LZ, 4> gauge;
#else

#define LT 6
#define LX 6
#define LY 1
#define LZ 1

#pragma molly transform("{ [t,x,y,z] -> [node[pt,px] -> local[t,x,y,z]] : pt=floor(t/3) and px=floor(x/3) }")
molly::array<spinor_t, LT, LX, LY, LZ> source, sink;

#pragma molly transform("{ [t,x,y,z,d] -> [node[pt,px] -> local[t,x,y,z,d]] :  (pt=floor(t/3) and px=floor(x/3)) or (pt=floor((t-1)/3) and px=floor((x-1)/3)) }")
molly::array<su3matrix_t, LT, LX, LY, LZ, 4> gauge;
#endif


extern "C" MOLLY_ATTR(process) void HoppingMatrix() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1) {

          // T+
          auto result = expand_TUP(gauge[t][x][y][z][DIM_T] * project_TUP(source[molly::mod(t + 1, LT)][x][y][z]));

          // T-
          result += expand_TDN(gauge[molly::mod(t - 1, LT)][x][y][z][DIM_T] * project_TUP(source[molly::mod(t - 1, LT)][x][y][z]));

          // X+
          result += expand_XUP(gauge[t][x][y][z][DIM_X] * project_TUP(source[t][molly::mod(x + 1, LX)][y][z]));

          // X-
          result += expand_XDN(gauge[t][molly::mod(x - 1, LX)][y][z][DIM_X] * project_TUP(source[t][molly::mod(x - 1, LX)][y][z]));

          // Y+
          result += expand_YUP(gauge[t][x][y][z][DIM_Y] * project_TUP(source[t][x][molly::mod(y + 1, LY)][z]));

          // Y-
          result += expand_YDN(gauge[t][x][molly::mod(y - 1, LY)][z][DIM_Y] * project_TUP(source[t][x][molly::mod(y - 1, LY)][z]));

          // Z+
          result += expand_ZUP(gauge[t][x][y][z][DIM_Z] * project_TUP(source[t][x][y][molly::mod(z + 1, LZ)]));

          // Z-
          result += expand_ZDN(gauge[t][x][y][molly::mod(z - 1, LZ)][DIM_Z] * project_TUP(source[t][x][y][molly::mod(z + 1, LZ)]));


          // Writeback
          sink[t][x][y][z] = result;
        }
} // void HoppingMatrix()
#else

#define LT 6
#define LX 4

#pragma molly transform("{ [t,x] -> [node[pt] -> local[t,x]] : pt=floor(t/3) }")
molly::array<spinor_t, LT, LX> source, sink;

#pragma molly transform("{ [t,x,d] -> [node[pt] -> local[t,x,d]] : pt=floor(t/3) or pt=floor((t-1)/3) }")
molly::array<su3matrix_t, LT, LX, 2> gauge;

extern "C" void HoppingMatrix() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1) {

      // T+
      auto result = expand_TUP(gauge[t][x][DIM_T] * project_TUP(source[molly::mod(t + 1, LT)][x]));

      // T-
      result += expand_TDN(gauge[molly::mod(t - 1, LT)][x][DIM_T] * project_TDN(source[molly::mod(t - 1, LT)][x]));

      // Writeback
      sink[t][x] = result;
    }
} // void HoppingMatrix()

#endif



MOLLY_ATTR(pure) spinor_t initSpinorVal(coord_t t, coord_t x, coord_t y, coord_t z) {
  if (t==0&&x==0&&y==0&&z==0)
    return spinor_t(su3vector_t::one_b(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());

  return spinor_t::zero();
}


MOLLY_ATTR(pure) su3matrix_t initGaugeVal(coord_t t, coord_t x, coord_t y, coord_t z, direction_t dir) {
  if (t == 0 && x == 0 && y == 0 && z == 0)
    return su3matrix_t::mone();

  return su3matrix_t::zero();
}


extern "C" MOLLY_ATTR(process) void init() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1)
          source[t][x][y][z] = initSpinorVal(t, x, y, z);

  for (coord_t t = 0; t < gauge.length(0); t += 1)
    for (coord_t x = 0; x < gauge.length(1); x += 1)
      for (coord_t y = 0; y < gauge.length(2); y += 1)
        for (coord_t z = 0; z < gauge.length(3); z += 1)
          for (coord_t d = 0; d < gauge.length(4); d += 1)
            gauge[t][x][y][z][d] = initGaugeVal(t, x, y, z, static_cast<direction_t>(2 * d));
}




extern "C" MOLLY_ATTR(process) double reduce() {
  double result = 0;

  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      for (coord_t y = 0; y < sink.length(2); y += 1)
        for (coord_t z = 0; z < sink.length(3); z += 1)
          result += norm(sink[t][x][y][z]);
  return result;
}


int main(int argc, char *argv[]) {
  init();

  HoppingMatrix();

  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << result << '\n';

  return 0;
}
