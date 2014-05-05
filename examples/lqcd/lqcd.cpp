#ifdef WITH_MOLLY
#include <molly.h>
#else /* WITH_MOLLY */
#include <molly_emulation.h>
#endif /* WITH_MOLLY */

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


#if 1
#if 1

#define _STR(X) #X
#define STR(X) _STR(X)

#ifndef LT
#define L  16
#define LT L
#define LX L
#define LY L
#define LZ L
 
#define B 8
#define BT B
#define BX B
#define BY B
#define BZ B

#define P 2
#define PT P
#define PX P
#define PY P
#define PZ P
#endif

#define sBT STR(BT)
#define sBX STR(BX)
#define sBY STR(BY)
#define sBZ STR(BZ)

#define sPT STR(PT)
#define sPX STR(PX)
#define sPY STR(PY)
#define sPZ STR(PZ)



#pragma molly transform("{ [t,x,y,z,d] -> [node[pt,px,py,pz] -> local[t,x,y,z,d]] : 0<=pt<" sPT" and 0<=px<" sPX" and 0<=py<" sPY" and 0<=pz<" sPZ" and " sBT"pt<=t<=" sBT"*(pt+1) and " sBX"px<=x<=" sBX"*(px+1) and " sBY"py<=y<=" sBY"*(py+1) and " sBZ"pz<=z<=" sBZ"*(pz+1) }")
molly::array<su3matrix_t, LT + 1, LX + 1, LY + 1, LZ + 1, 4> gauge;
#define GAUGE_MOD(divident,divisor) divident


#pragma molly transform("{ [t,x,y,z] -> [node[floor(t/" sBT"),floor(x/" sBX"),floor(y/" sBY"),floor(z/" sBZ")] -> local[floor(t/2),x,y,z,t%2]] }")
molly::array<spinor_t, LT, LX, LY, LZ> source, sink;




#if 0
// cheating, no wraparound
#pragma molly transform("{ [t,x,y,z,d] -> [node[pt] -> local[t,x,y,z,d]] : pt=floor(t/4) or (pt<8 and pt=floor((t+1)/4)) }")
molly::array<su3matrix_t, LT+1, LX+1, LY+1, LZ+1, 4> gauge;
#endif

#else

#define LT 6
#define LX 6
#define LY 1
#define LZ 1

#pragma molly transform("{ [t,x,y,z] -> [node[pt,px] -> local[t,x,y,z]] : pt=floor(t/3) and px=floor(x/3) }")
molly::array<spinor_t, LT, LX, LY, LZ> source, sink;

#pragma molly transform("{ [t,x,y,z,d] -> [node[pt,px] -> local[t,x,y,z,d]] : (pt=floor(t/3) and px=floor(x/3)) or (pt=floor((t-1)/3) and px=floor((x-1)/3)) }")
molly::array<su3matrix_t, LT, LX, LY, LZ, 4> gauge;
#endif

#if 1
extern "C" MOLLY_ATTR(process) void HoppingMatrix() {
  for (coord_t t = 0; t < source.length(0); t += 1)
    for (coord_t x = 0; x < source.length(1); x += 1)
      for (coord_t y = 0; y < source.length(2); y += 1)
        for (coord_t z = 0; z < source.length(3); z += 1) {
          
          // T+
          auto result = expand_TUP(gauge[GAUGE_MOD(t + 1, LT)][x][y][z][DIM_T] * project_TUP(source[molly::mod(t + 1, LT)][x][y][z]));

          // T-
          result += expand_TDN(gauge[t][x][y][z][DIM_T] * project_TDN( source[molly::mod(t - 1, LT)][x][y][z]));
#if 1
          // X+
          result += expand_XUP(gauge[t][GAUGE_MOD(x + 1, LX)][y][z][DIM_X] * project_XUP(source[t][molly::mod(x + 1, LX)][y][z]));

          // X-
          result += expand_XDN(gauge[t][x][y][z][DIM_X] * project_XDN(source[t][molly::mod(x - 1, LX)][y][z]));

          // Y+
          result += expand_YUP(gauge[t][x][GAUGE_MOD(y + 1, LY)][z][DIM_Y] * project_YUP(source[t][x][molly::mod(y + 1, LY)][z]));

          // Y-
          result += expand_YDN(gauge[t][x][y][z][DIM_Y] * project_YDN(source[t][x][molly::mod(y - 1, LY)][z]));

          // Z+
          result += expand_ZUP(gauge[t][x][y][GAUGE_MOD(z + 1, LZ)][DIM_Z] * project_ZUP(source[t][x][y][molly::mod(z + 1, LZ)]));

          // Z-
          result += expand_ZDN(gauge[t][x][y][z][DIM_Z] * project_ZDN(source[t][x][y][molly::mod(z + 1, LZ)]));
#endif

          // Writeback
          sink[t][x][y][z] = result;
        }
} // void HoppingMatrix()
#endif
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
  //return spinor_t(su3vector_t(t+2, 1, t+2), su3vector_t(x+2, x+2, 1), su3vector_t(y+2, 1, y+2), su3vector_t(z+2, z+2, 1));
  
  if (t==0&&x==0&&y==0&&z==0)
    return spinor_t(su3vector_t::one_b(), su3vector_t::zero(), su3vector_t::zero(), su3vector_t::zero());

  return spinor_t::zero();
}


MOLLY_ATTR(pure) su3matrix_t initGaugeVal(coord_t t, coord_t x, coord_t y, coord_t z, direction_t dir) {
  t = molly::mod(t, LT);
  x = molly::mod(x, LX);
  y = molly::mod(y, LY);
  z = molly::mod(z, LZ);

  //return su3matrix_t::allone();

  if (t == 0 && x == 0 && y == 0 && z == 0)
    return su3matrix_t::mone();

  return su3matrix_t::zero();
}


MOLLY_ATTR(pure) void checkSpinorVal(coord_t t, coord_t x, coord_t y, coord_t z, spinor_t val) {
  return;

  auto c = val[0][0];
  if (c != complex(t)) {
    std::cerr << "Wrong t (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[1][0];
  if (c != complex(x)) {
    std::cerr << "Wrong x (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[2][0];
  if (c != complex(y)) {
    std::cerr << "Wrong y (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
   c = val[3][0];
  if (c != complex(z)) {
    std::cerr << "Wrong z (" << c << ") at (" << t << "," << x << "," << y << "," << z << ")\n";
  }
}


#if 1
extern "C" MOLLY_ATTR(process) void init() {
//extern "C" void init() {
#if 1
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
#endif
}
#endif


extern "C" MOLLY_ATTR(process) double reduce() {
//extern "C" double reduce() {
#if 0
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      for (coord_t y = 0; y < sink.length(2); y += 1)
        for (coord_t z = 0; z < sink.length(3); z += 1)
          checkSpinorVal(t, x, y, z, source[t][x][y][z]);
#endif

#if 0
  double result = 0; 
  for (coord_t t = 0; t < sink.length(0); t += 1)
    for (coord_t x = 0; x < sink.length(1); x += 1)
      for (coord_t y = 0; y < sink.length(2); y += 1)
        for (coord_t z = 0; z < sink.length(3); z += 1)
          result += norm(sink[t][x][y][z]);
  return result;
#endif
  return 0;
}

#if 0
double sqr(double val) { 
  return val*val; 
}

typedef struct {
	double avgtime;
	double localrmstime;
	double globalrmstime;
	double totcycles;
	double localavgflop;

	ucoord sites_body;
	ucoord sites_surface;
	ucoord sites;

	ucoord lup_body;
	ucoord lup_surface;
	ucoord lup;

	double error;

	mypapi_counters counters;
	bgq_hmflags opts;
	double avgovhtime;
} benchstat;



static void print_stats(benchstat *stats) {
#if PAPI
	int threads = omp_get_num_threads();

	for (mypapi_interpretations j = 0; j < __pi_COUNT; j+=1) {
		printf("%10s|", "");
		char *desc = NULL;

		for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
			char str[80];
			str[0] = '\0';
			benchstat *stat = &stats[i3];
			bgq_hmflags opts = stat->opts;

			double avgtime = stat->avgtime;
			uint64_t lup = stat->lup;
			uint64_t flop = compute_flop(opts, stat->lup_body, stat->lup_surface);
			double flops = (double)flop/stat->avgtime;
			double localrms = stat->localrmstime / stat->avgtime;
			double globalrms = stat->globalrmstime / stat->avgtime;
			ucoord sites = stat->sites;

			double nCycles = stats[i3].counters.native[PEVT_CYCLES];
			double nCoreCycles = stats[i3].counters.corecycles;
			double nNodeCycles = stats[i3].counters.nodecycles;
			double nInstructions = stats[i3].counters.native[PEVT_INST_ALL];
			double nStores = stats[i3].counters.native[PEVT_LSU_COMMIT_STS];
			double nL1IStalls = stats[i3].counters.native[PEVT_LSU_COMMIT_STS];
			double nL1IBuffEmpty = stats[i3].counters.native[PEVT_IU_IBUFF_EMPTY_CYC];

			double nIS1Stalls = stats[i3].counters.native[PEVT_IU_IS1_STALL_CYC];
			double nIS2Stalls = stats[i3].counters.native[PEVT_IU_IS2_STALL_CYC];

			double nCachableLoads = stats[i3].counters.native[PEVT_LSU_COMMIT_CACHEABLE_LDS];
			double nL1Misses = stats[i3].counters.native[PEVT_LSU_COMMIT_LD_MISSES];
			double nL1Hits = nCachableLoads - nL1Misses;

			double nL1PMisses = stats[i3].counters.native[PEVT_L1P_BAS_MISS];
			double nL1PHits = stats[i3].counters.native[PEVT_L1P_BAS_HIT];
			double nL1PAccesses = nL1PHits + nL1PMisses;

			double nL2Misses = stats[i3].counters.native[PEVT_L2_MISSES];
			double nL2Hits = stats[i3].counters.native[PEVT_L2_HITS];
			double nL2Accesses = nL2Misses + nL2Hits;

			double nDcbtHits = stats[i3].counters.native[PEVT_LSU_COMMIT_DCBT_HITS];
			double nDcbtMisses = stats[i3].counters.native[PEVT_LSU_COMMIT_DCBT_MISSES];
			double nDcbtAccesses = nDcbtHits + nDcbtMisses;

			double nXUInstr = stats[i3].counters.native[PEVT_INST_XU_ALL];
			double nAXUInstr = stats[i3].counters.native[PEVT_INST_QFPU_ALL];
			double nXUAXUInstr = nXUInstr + nAXUInstr;

#if 0
			double nNecessaryInstr = 0;
			if (!(opts & hm_nobody))
			nNecessaryInstr += bodySites * (240/*QFMA*/+ 180/*QMUL+QADD*/+ 180/*LD+ST*/)/2;
			if (!(opts & hm_noweylsend))
			nNecessaryInstr += haloSites * (2*3*2/*QMUL+QADD*/+ 4*3/*LD*/+ 2*3/*ST*/)/2;
			if (!(opts & hm_nosurface))
			nNecessaryInstr += surfaceSites * (240/*QFMA*/+ 180/*QMUL+QADD*/+ 180/*LD+ST*/- 2*3*2/*QMUL+QADD*/- 4*3/*LD*/+ 2*3/*LD*/)/2;
			if (!(opts & hm_nokamul))
			nNecessaryInstr += sites * (8*2*3*1/*QFMA*/+ 8*2*3*1/*QMUL*/)/2;
#endif

			uint64_t nL1PListStarted = stats[i3].counters.native[PEVT_L1P_LIST_STARTED];
			uint64_t nL1PListAbandoned= stats[i3].counters.native[PEVT_L1P_LIST_ABANDON];
			uint64_t nL1PListMismatch= stats[i3].counters.native[PEVT_L1P_LIST_MISMATCH];
			uint64_t nL1PListSkips = stats[i3].counters.native[PEVT_L1P_LIST_SKIP];
			uint64_t nL1PListOverruns = stats[i3].counters.native[PEVT_L1P_LIST_CMP_OVRUN_PREFCH];

			double nL1PLatePrefetchStalls = stats[i3].counters.native[PEVT_L1P_BAS_LU_STALL_LIST_RD_CYC];

			uint64_t nStreamDetectedStreams = stats[i3].counters.native[PEVT_L1P_STRM_STRM_ESTB];
			double nL1PSteamUnusedLines = stats[i3].counters.native[PEVT_L1P_STRM_EVICT_UNUSED];
			double nL1PStreamPartiallyUsedLines = stats[i3].counters.native[PEVT_L1P_STRM_EVICT_PART_USED];
			double nL1PStreamLines = stats[i3].counters.native[PEVT_L1P_STRM_LINE_ESTB];
			double nL1PStreamHits = stats[i3].counters.native[PEVT_L1P_STRM_HIT_LIST];

			double nDdrFetchLine = stats[i3].counters.native[PEVT_L2_FETCH_LINE];
			double nDdrStoreLine = stats[i3].counters.native[PEVT_L2_STORE_LINE];
			double nDdrPrefetch = stats[i3].counters.native[PEVT_L2_PREFETCH];
			double nDdrStorePartial = stats[i3].counters.native[PEVT_L2_STORE_PARTIAL_LINE];

			switch (j) {
			case pi_correct:
				desc = "Max error to reference";
				if (opts & hm_withcheck) {
					snprintf(str, sizeof(str), "%g", stat->error);
				}
				break;
			case pi_ramfetchrate:
				desc = "DDR read";
				snprintf(str, sizeof(str), "%.2f GB/s", 128 * nDdrFetchLine / (avgtime * GIBI));
				break;
			case pi_ramstorerate:
				desc = "DDR write";
				snprintf(str, sizeof(str), "%.2f GB/s", 128 * nDdrStoreLine / (avgtime * GIBI));
				break;
			case pi_ramstorepartial:
				desc = "DDR partial writes";
				snprintf(str, sizeof(str), "%.2f %%",  100 * nDdrStorePartial / (nDdrStorePartial+nDdrStoreLine));
				break;
			case pi_l2prefetch:
				desc = "L2 prefetches";
				snprintf(str, sizeof(str), "%.2f %%",  100 * nDdrPrefetch / nDdrFetchLine);
				break;
			case pi_msecs:
				desc = "Iteration time";
				snprintf(str, sizeof(str), "%.3f mSecs",stat->avgtime/MILLI);
				break;
			case pi_cycpersite:
				desc = "per site update";
				snprintf(str, sizeof(str), "%.1f cyc", stat->totcycles / lup);
				break;
			case pi_instrpersite:
				desc = "instr per update";
				snprintf(str, sizeof(str), "%.1f", nInstructions / lup);
				break;
			case pi_fxupersite:
				desc = "FU instr per update";
				snprintf(str, sizeof(str), "%.1f", nAXUInstr / lup);
				break;
			case pi_flops:
				desc = "MFlop/s";
				snprintf(str, sizeof(str), "%.0f MFlop/s", flops/MEGA);
				break;
			case pi_flopsref:
				desc = "Speed";
				snprintf(str, sizeof(str), "%.0f MFlop/s", stat->localavgflop / (avgtime * MEGA));
				break;
			case pi_floppersite:
				desc = "Flop per site";
				snprintf(str, sizeof(str), "%.1f Flop", stat->localavgflop / sites);
				break;
			case pi_localrms:
				desc = "Thread RMS";
				snprintf(str, sizeof(str), "%.1f %%", 100.0*localrms);
				break;
			case pi_globalrms:
				desc = "Node RMS";
				snprintf(str, sizeof(str), "%.1f %%", 100.0*globalrms);
				break;
			case pi_avgovhtime:
				desc = "Threading overhead";
				snprintf(str, sizeof(str), "%.1f cyc", stat->avgovhtime);
				break;
			case pi_detstreams:
				desc = "Detected streams";
				snprintf(str, sizeof(str), "%llu", nStreamDetectedStreams);
				break;
			case pi_l1pstreamunusedlines:
				desc = "Unused (partially) lines";
				snprintf(str, sizeof(str), "%.2f%% (%.2f%%)", 100.0 * nL1PSteamUnusedLines / nL1PStreamLines, 100.0 * nL1PStreamPartiallyUsedLines / nL1PStreamLines);
				break;
			case pi_l1pstreamhitinl1p:
				desc = "Loads that hit in L1P stream";
				snprintf(str, sizeof(str), "%.2f %%", 100.0 * nL1PStreamHits / nCachableLoads);
				break;
			case pi_cpi:
				desc = "Cycles per instruction (Thread)";
				snprintf(str, sizeof(str), "%.3f cpi", nCycles / nInstructions);
				break;
			case pi_corecpi:
				desc = "Cycles per instruction (Core)";
				snprintf(str, sizeof(str), "%.3f cpi", nCoreCycles / nInstructions);
				break;
			case pi_l1istalls:
				desc = "Empty instr buffer";
				snprintf(str, sizeof(str), "%.2f %%", nL1IBuffEmpty / nCycles);
				break;
			case pi_is1stalls:
				desc = "IS1 Stalls (dependency)";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nIS1Stalls / nCycles);
				break;
			case pi_is2stalls:
				desc = "IS2 Stalls (func unit)";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nIS2Stalls / nCycles);
				break;
			case pi_hitinl1:
				desc = "Loads that hit in L1";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nL1Hits / nCachableLoads);
				break;
			case pi_l1phitrate:
				desc = "L1P hit rate";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nL1PHits / nL1PAccesses);
				break;
				//case pi_overhead:
				//desc = "Instr overhead";
				//snprintf(str, sizeof(str), "%.2f %%", 100 * (nInstructions - nNecessaryInstr) / nInstructions);
				//break;
			case pi_hitinl1p:
				desc = "Loads that hit in L1P";
				snprintf(str, sizeof(str), "%f %%" ,  100 * nL1PHits / nCachableLoads);
				break;
			case pi_l2hitrate:
				desc = "L2 hit rate";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nL2Hits / nL2Accesses);
				break;
			case pi_dcbthitrate:
				desc = "dcbt hit rate";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nDcbtHits / nDcbtAccesses);
				break;
			case pi_axufraction:
				desc = "FXU instrs";
				snprintf(str, sizeof(str), "%.2f %%", 100 * nAXUInstr / nXUAXUInstr);
				break;
			case pi_l1pliststarted:
				desc = "List prefetch started";
				snprintf(str, sizeof(str), "%llu", nL1PListStarted);
				break;
			case pi_l1plistabandoned:
				desc = "List prefetch abandoned";
				snprintf(str, sizeof(str), "%llu", nL1PListAbandoned);
				break;
			case pi_l1plistmismatch:
				desc = "List prefetch mismatch";
				snprintf(str, sizeof(str), "%llu", nL1PListMismatch);
				break;
			case pi_l1plistskips:
				desc = "List prefetch skip";
				snprintf(str, sizeof(str), "%llu", nL1PListSkips);
				break;
			case pi_l1plistoverruns:
				desc = "List prefetch overrun";
				snprintf(str, sizeof(str), "%llu", nL1PListOverruns);
				break;
			case pi_l1plistlatestalls:
				desc = "Stalls list prefetch behind";
				snprintf(str, sizeof(str), "%.2f", nL1PLatePrefetchStalls / nCoreCycles);
				break;
			default:
				continue;
			}

			printf("%"SCELLWIDTH"s|", str);
		}

		printf(" %s\n", desc);
	}
#endif
}


static void exec_table(benchfunc_t benchmark, bgq_hmflags additional_opts, bgq_hmflags kill_opts,  int j_max, int k_max) {
//static void exec_table(bool sloppiness, hm_func_double hm_double, hm_func_float hm_float, bgq_hmflags additional_opts) {
	benchstat excerpt;

	if (g_proc_id == 0)
		printf("%10s|", "");
	for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
		if (g_proc_id == 0)
			printf("%-"SCELLWIDTH"s|", flags_desc[i3]);
	}
	if (g_proc_id == 0)
		printf("\n");
	print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * lengthof(flags));
	if (g_proc_id == 0)
		printf("\n");
	for (int i2 = 0; i2 < lengthof(omp_threads); i2 += 1) {
		int threads = omp_threads[i2];

		if (g_proc_id == 0)
			printf("%-10s|", omp_threads_desc[i2]);

		benchstat stats[lengthof(flags)];
		for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
			bgq_hmflags hmflags = flags[i3];
			hmflags = (hmflags | additional_opts) & ~kill_opts;

			benchstat result = runbench(benchmark, hmflags, k_max, j_max, threads);
			stats[i3] = result;

			if (threads == 64 && i3 == 3) {
				excerpt = result;
			}

			char str[80] = { 0 };
			if (result.avgtime == 0)
				snprintf(str, sizeof(str), "~ %s", (result.error > 0.001) ? "X" : "");
			else
				snprintf(str, sizeof(str), "%.2f mlup/s%s", (double) result.lup / (result.avgtime * MEGA), (result.error > 0.001) ? "X" : "");
			if (g_proc_id == 0)
				printf("%"SCELLWIDTH"s|", str);
			if (g_proc_id == 0)
				fflush(stdout);
		}
		if (g_proc_id == 0)
			printf("\n");

		if (g_proc_id == 0) {
			print_stats(stats);
		}

		print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * lengthof(flags));
		if (g_proc_id == 0)
			printf("\n");
	}

	if (g_proc_id == 0) {
		printf("Hardware counter excerpt (64 threads, nocom):\n");
		mypapi_print_counters(&excerpt.counters);
	}
	if (g_proc_id == 0)
		printf("\n");
}
#endif

#if 0 
int test(benchstat *result) {
  MPI_Barrier();

  double duration_sum= 0;
  double duration_sqsum= 0;
  for (auto i=0; i<nRounds;i+=1) {
    double start_time = MPI_Wtime();
    
    HoppingMatrix();
    
    double stop_time = MPI_Wtime();
    double duration = stop_time-start_time;
    duration_sum += duration;
    duration_sqsum += duration*duration;
  }
  
  
    double localavgtime = duration_sum / its;
    double localavgsqtime = sqr(localavgtime);
    double localrmstime = sqrt((duration_sqsum / its) - localavgsqtime);
    double localcycles = (double)localsumcycles / (double)its;
    double localavgflop = (double)localsumflop / (double)its;

    double localtime[] = { localavgtime, localavgsqtime, localrmstime };
    double sumreduce[3] = { -1, -1, -1 };
    MPI_Allreduce(&localtime, &sumreduce, 3, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
    double sumtime = sumreduce[0];
    double sumsqtime = sumreduce[1];
    double sumrmstime = sumreduce[2];


    double avgtime = sumtime / g_nproc;
    double avglocalrms = sumrmstime / g_nproc;
    double rmstime = sqrt((sumsqtime / g_nproc) - sqr(avgtime));

    // Assume k_max lattice site updates, even+odd sites
    ucoord sites = LOCAL_LT * LOCAL_LX * LOCAL_LY * LOCAL_LZ;
    ucoord sites_body = PHYSICAL_BODY * PHYSICAL_LK * PHYSICAL_LP;
    ucoord sites_surface = PHYSICAL_SURFACE * PHYSICAL_LK * PHYSICAL_LP;
    assert(sites == sites_body+sites_surface);
    assert(sites == VOLUME);

    ucoord lup_body = k_max * sites_body;
    ucoord lup_surface = k_max * sites_surface;
    ucoord lup = lup_body + lup_surface;
    assert(lup == k_max * sites);

    result->avgtime = avgtime;
    result->localrmstime = avglocalrms;
    result->globalrmstime = rmstime;
    result->totcycles = localcycles;
    result->localavgflop = localavgflop;

    result->sites_surface = sites_surface;
    result->sites_body = sites_body;
    result->sites = sites;
    result->lup_surface = lup_surface;
    result->lup_body = lup_body;
    result->lup = lup;
    //result->flops = flops / avgtime;
    result->error = err;
    result->counters = counters;
    result->opts = opts;
    result->avgovhtime = avgovhtime;
    return EXIT_SUCCESS;
}
#endif

 
void bench() {
   int nTests = 10;
   int nRounds = 10;
   
  molly::exec_bench([nRounds] (int k, molly::bgq_hmflags flags) {
    for (auto i=0; i<nRounds;i+=1) {
      HoppingMatrix();
    }
  }, nTests);
  
#if 0
  int nRounds = 10;
  int nTests = 10;
  

  
  for (auto j=0; j<nTests;j+=1) {
    test();
  }
  
  double duration_avg = duration_sum / nTests;
  double duration_rms = sqrt( (duration_sqsum / nTests) - sqr(duration_avg) );

  double localtime[] = { duration_avg, duration_avg*duration_avg, duration_rms };
  double sumreduce[3] = { -1, -1, -1 };
  MPI_Allreduce(&localtime, &sumreduce, 3, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD );
  double sumtime = sumreduce[0];
  double sumsqtime = sumreduce[1];
  double sumrmstime = sumreduce[2];
  
    auto nRanks = PT*PX*PY*PZ;
    auto nTotalIterations = nRanks * nIterationsPerRank;
    auto nSitesPerIteration = LT*LX*LY*LZ;
    auto nTotalSites = nRanks*nIterationsPerRank*nSitesPerIteration;
  
    benchstat stats;
    	benchstat *result = &stats;
	result->avgtime = avgtime;
	result->localrmstime = avglocalrms;
	result->globalrmstime = rmstime;
	result->totcycles = localcycles;
	result->localavgflop = localavgflop;

	result->sites_surface = sites_surface;
	result->sites_body = sites_body;
	result->sites = sites;
	result->lup_surface = lup_surface;
	result->lup_body = lup_body;
	result->lup = lup;
	//result->flops = flops / avgtime;
	result->error = err;
	result->counters = counters;
	result->opts = opts;
	result->avgovhtime = avgovhtime;
    
    if (__molly_isMaster()) {
      std::cout << ;
    }
#endif
}


int main(int argc, char *argv[]) {
  init();

  // Warmup+ check result
  HoppingMatrix();

  auto result = reduce();
  if (__molly_isMaster())
    std::cout << ">>>> Result = " << result << '\n';

  bench();
  
  //call_test();
  
  return 0;
}
