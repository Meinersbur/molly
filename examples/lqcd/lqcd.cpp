
#include <molly.h>

// C++ std::complex
#include <complex>
typedef std::complex<double> complex;
 
// C99 complex
#include <math.h>
#include <complex.h>
//typedef _Complex double complex; 


typedef struct {
  complex c[3][3];

} su3matrix_t;

typedef struct su3vector_t {
  complex c[3];
   
public:
  [[molly::pure]] su3vector_t() : c({0,0,0}) {}
  [[molly::pure]] su3vector_t(complex c0, complex c1, complex c2) : c({ c0, c1, c2 }) {
    // c[0] = c0;
    // c[1] = c1;
    // c[2] = c2;
  }

  [[molly::pure]] const complex &operator[](size_t idx) const { return c[idx]; }
  [[molly::pure]] complex &operator[](size_t idx)  { return c[idx]; }

  [[molly::pure]] const su3vector_t &operator+=(su3vector_t rhs) {
    c[0] += rhs[0];
    c[1] += rhs[1];
    c[2] += rhs[2];
    return *this;
  }
} su3vector_t;

[[molly::pure]] su3vector_t operator+(su3vector_t lhs, su3vector_t rhs) {
  return su3vector_t(lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[3]);
}

typedef struct halfspinor_t {
  su3vector_t v[2];

public:
  [[molly::pure]] halfspinor_t() {}
  [[molly::pure]] halfspinor_t(su3vector_t v0, su3vector_t v1) {
    v[0] = v0;
    v[1] = v1;
  }

  [[molly::pure]] const su3vector_t &operator[](size_t idx) const { return v[idx]; }
  [[molly::pure]] su3vector_t &operator[](size_t idx)  { return v[idx]; }
} halfspinor_t;
typedef struct fullspinor_t {
  su3vector_t v[4];

public:
  [[molly::pure]] fullspinor_t() {}
  [[molly::pure]] fullspinor_t(su3vector_t v0, su3vector_t v1, su3vector_t v2, su3vector_t v3)  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    v[3] = v3;
  }

  [[molly::pure]] const su3vector_t &operator[](size_t idx) const { return v[idx]; }
  [[molly::pure]] su3vector_t &operator[](size_t idx)  { return v[idx]; }

  [[molly::pure]] const fullspinor_t &operator+=(fullspinor_t rhs) {
    v[0] += rhs[0];
    v[1] += rhs[1];
    v[2] += rhs[2];
    v[3] += rhs[3];
    return *this;
  }
} fullspinor_t;
typedef fullspinor_t spinor_t;





typedef enum {
  DIM_T, DIM_X, DIM_Y, DIM_Z
} dimension_t;

typedef enum {
  DIR_TUP, DIR_TDN, DIR_XUP, DIR_XDN, DIR_YUP, DIR_YDN, DIR_ZUP, DIR_ZDOWN
} direction_t;

[[molly::pure]] su3vector_t operator*(su3matrix_t m, su3vector_t v) {
  return su3vector_t(
    m.c[0][0] * v[0] + m.c[0][1] * v[1] + m.c[0][2] * v[2],
    m.c[1][0] * v[0] + m.c[1][1] * v[1] + m.c[1][2] * v[2],
    m.c[2][0] * v[0] + m.c[2][1] * v[1] + m.c[2][2] * v[2]);
}

[[molly::pure]] halfspinor_t operator*(su3matrix_t m, halfspinor_t v) {
  return halfspinor_t(m*v[0], m*v[1]);
}



[[molly::pure]] halfspinor_t project_TUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

[[molly::pure]] halfspinor_t project_TDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_XUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_XDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_YUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_YDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_ZUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

[[molly::pure]] halfspinor_t project_ZDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}


[[molly::pure]] fullspinor_t expand_TUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_TDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_XUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_XDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_YUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_YDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_ZUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

[[molly::pure]] fullspinor_t expand_ZDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

typedef int64_t coord_t;


#if 1
#if 0
#define L 8
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
#define LX 3
#define LY 3
#define LZ 3

#pragma molly transform("{ [t,x,y,z] -> [node[pt] -> local[t,x,y,z]] : pt=floor(t/3) }")
molly::array<spinor_t, LT, LX, LY, LZ> source, sink;

#pragma molly transform("{ [t,x,y,z,d] -> [node[pt] -> local[t,x,y,z,d]] : pt=floor(t/3) or pt=floor((t-1)/3) }")
molly::array<su3matrix_t, LT, LX, LY, LZ, 4> gauge;
#endif

extern "C" void HoppingMatrix() {
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


int main(int argc, char *argv[]) {
  // TODO: Initialize source,gauge
  HoppingMatrix();
  return 0;
}
