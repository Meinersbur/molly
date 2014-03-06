
#include <molly.h>

// C++ std::complex
#include <complex>
//typedef std::complex<double> complex;

// C99 complex
#include <math.h>
#include <complex.h>
typedef _Complex double complex; 


typedef struct {
  complex c[3][3];
} su3matrix_t;

typedef struct {
  complex c[3];
} su3vector_t;

typedef struct {
  su3_vector v[2];
} halfspinor_t;
typedef struct {
  su3_vector v[4];
} fullspinor_t;
typedef fullspinor_t spinor_t;

typedef enum {
  DIM_T,DIM_X,DIM_Y,DIM_Z;
} dimension_t;

typedef enum {
  DIR_TUP,DIR_TDN,DIR_XUP,DIR_XDN,DIR_YUP,DIR_YDN,DIR_ZUP,DIR_ZDOWN;
} direction_t;


#define L 8
#define LT L
#define LX L
#define LY L
#define LZ L

#pragma molly transform("[t,x,y,z] -> [], [t,x,y,z]")
molly::array<spinor_t, LT, LX, LY, LZ> source,sink;

#pragma molly transform("[t,x,y,z,d] -> [], [t,x,y,z,d]")
molly::array<su3matrix_t, LT, LX, LY, LZ, 4> gauge;


halfspinor_t project_TUP(fullspinor_t spinor) {
  return halfspinor_t(spinor[0] + spinor[2], spinor[1] + spinor[3]);
}

halfspinor_t project_TDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_XUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_XDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_YUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_YDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_ZUP(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}

halfspinor_t project_ZDN(fullspinor_t spinor) {
  halfspinor_t result;
  result[0] = spinor[0] + spinor[2];
  result[1] = spinor[1] + spinor[3];
  return result;
}


fullspinor_t expand_TUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_TDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_XUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_XDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_YUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_YDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_ZUP(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}

fullspinor_t expand_ZDN(halfspinor_t weyl) {
  return fullspinor_t(weyl[0], weyl[1], weyl[0], weyl[1]);
}



typedef int64_t coord_t;



void HoppingMatrix() {

  for (coord_t t = 0; t < source.length(0); t+=1) 
    for (coord_t x = 0; x < source.length(1); x+=1) 
      for (coord_t y = 0; y < source.length(2); y+=1) 
        for (coord_t z = 0; z < source.length(3); z+=1) {

          // T+
          auto result = expand_TUP(gauge[t,x,y,z,DIM_T]*project_TUP(source[molly::mod(t+1,LT),x,y,z]));
          
          // T-
          result += expand_TDN(gauge[molly::mod(t-1,LT),x,y,z,DIM_T]*project_TUP(source[molly::mod(t-1,LT),x,y,z]));
          
          // X+
          result += expand_XUP(gauge[t,x,y,z,DIM_X]*project_TUP(source[t,molly::mod(x+1,LX),y,z]));
          
           // X-
          result += expand_XDN(gauge[t,molly::mod(x-1,LX),y,z,DIM_X]*project_TUP(source[t,molly::mod(x-1,LX),y,z]));
          
          // Y+
          result += expand_YUP(gauge[t,x,y,z,DIM_Y]*project_TUP(source[t,x,molly::mod(y+1,LY),z]));
          
           // Y-
          result += expand_YDN(gauge[t,x,molly::mod(y-1,LY),z,DIM_Y]*project_TUP(source[t,x,molly::mod(y-1,LY),z]));

          // Z+
          result += expand_ZUP(gauge[t,x,y,z,DIM_Z]*project_TUP(source[t,x,y,molly::mod(z+1,LZ)]));
          
          // Z-
          result += expand_ZDN(gauge[t,x,y,molly::mod(z01,LZ),DIM_Z]*project_TUP(source[t,x,y,molly::mod(z+1,LZ)]));
          
           
            // Writeback
            sink[t,x,y,z,s,v] = result;
        }
} // void HoppingMatrix()



int main(int argc, char *argv[]) {
  // TODO: Initialize source,gauge
  HoppingMatrix();
  return 0;
}
