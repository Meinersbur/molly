#define __MOLLYRT
#include "bench.h"

#include "molly.h"
#include "mypapi.h"
#include <mpi.h>

#ifdef BGQ_SPI
#include <cmath>
#include <bgpm/include/bgpm.h>
#include <hwi/include/bqc/A2_inlines.h> // GetTimeBase()
#include <l1p/pprefetch.h>
#include <l1p/sprefetch.h>
#endif

#include <math.h>



#ifndef STRINGIFY
#define STRINGIFY(V) #V
#endif
#ifndef TOSTRING
#define TOSTRING(V) STRINGIFY(V)
#endif

#define lengthof(X) (sizeof(X)/sizeof((X)[0]))
#define CELLWIDTH 15
#define SCELLWIDTH TOSTRING(CELLWIDTH)

#define L1P_CHECK(RTNCODE)                                                                 \
  do {                                                                                   \
      int mpi_rtncode = (RTNCODE);                                                       \
      if (mpi_rtncode != MPI_SUCCESS) {                                                  \
      fprintf(stderr, "L1P call %s at %s:%d failed: errorcode %d\n", #RTNCODE, __FILE__, __LINE__, mpi_rtncode);  \
      assert(!"L1P call " #RTNCODE " failed");                                       \
      abort();                                                                       \
    }                                                                                  \
  } while (0)


  
  
using namespace molly;
  
static int omp_threads[] = { 1 };
static const char *omp_threads_desc[] = { "1" };




#define DEFOPTS (0)
static const int flags[] = {
        DEFOPTS,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitdisable,
  DEFOPTS                       | hm_prefetchimplicitconfirmed,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitconfirmed,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitoptimistic
};
static const char *flags_desc[] = {
    "pf default",
    "pf disable",
    "pf stream",
    "pf confirmed",
    "pf optimistic"
  };

typedef struct {
  int j_max;
  const benchfunc_t *benchfunc;
  bgq_hmflags opts;
  benchstat result;
  uint64_t nStencilsPerTest;
  uint64_t nFlopsPerStencil;
} master_args;

typedef struct {
  int set;
  mypapi_counters result;
} mypapi_work_t;



static void print_repeat(const char * const str, const int count) {
  auto g_proc_id = __molly_cluster_mympirank();
  if (g_proc_id == 0) {
    for (int i = 0; i < count; i += 1) {
      printf("%s", str);
    }
  }
}

static double sqr(double val) {
  return val*val;  
}


static void benchmark_setup_worker(void *argptr, size_t tid, size_t threads) {
  mypapi_init();

#ifndef NDEBUG
  //feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
#endif

#ifdef BGQ_SPI
  const master_args *args = (const master_args *)argptr;
  const int j_max = args->j_max;
  const benchfunc_t &benchfunc = *args->benchfunc;
  const bgq_hmflags opts = args->opts;
  const bool nocom = opts & hm_nocom;
  const bool nooverlap = opts & hm_nooverlap;
  const bool nokamul = opts & hm_nokamul;
  const bool prefetchlist = opts & hm_prefetchlist;
  const bool noprefetchstream = opts & hm_noprefetchstream;
  const bool noprefetchexplicit = opts & hm_noprefetchexplicit;
  const bool noweylsend = opts & hm_noweylsend;
  const bool nobody = opts & hm_nobody;
  const bool nosurface = opts & hm_nosurface;
  const bool experimental = opts & hm_experimental;
  const bgq_hmflags implicitprefetch = (bgq_hmflags)(opts & (hm_prefetchimplicitdisable | hm_prefetchimplicitoptimistic | hm_prefetchimplicitconfirmed));

  L1P_StreamPolicy_t pol;
  switch (implicitprefetch) {
  case hm_prefetchimplicitdisable:
    pol = noprefetchstream ? L1P_stream_disable : L1P_confirmed_or_dcbt/*No option to selectively disable implicit stream*/;
    break;
  case hm_prefetchimplicitoptimistic:
    pol = L1P_stream_optimistic;
    break;
  case hm_prefetchimplicitconfirmed:
    pol = noprefetchstream ? L1P_stream_confirmed : L1P_confirmed_or_dcbt;
    break;
  default:
    // Default setting
    pol = L1P_confirmed_or_dcbt;
    break;
  }

  L1P_CHECK(L1P_SetStreamPolicy(pol));

  //if (g_proc_id==0) {
  //	L1P_StreamPolicy_t poli;
  //	L1P_CHECK(L1P_GetStreamPolicy(&poli));
  //	printf("Prefetch policy: %d\n",poli);
  //}

  // Test whether it persists between parallel sections
  //L1P_StreamPolicy_t getpol = 0;
  //L1P_CHECK(L1P_GetStreamPolicy(&getpol));
  //if (getpol != pol)
  //	fprintf(stderr, "MK StreamPolicy not accepted\n");

  //L1P_CHECK(L1P_SetStreamDepth());
  //L1P_CHECK(L1P_SetStreamTotalDepth());

  //L1P_GetStreamDepth
  //L1P_GetStreamTotalDepth

  // Peter Boyle's setting
  // Note: L1P_CFG_PF_USR_pf_stream_establish_enable is never set in L1P_SetStreamPolicy
  //uint64_t *addr = ((uint64_t*)(Kernel_L1pBaseAddress() + L1P_CFG_PF_USR_ADJUST));
  //*addr |=  L1P_CFG_PF_USR_pf_stream_est_on_dcbt | L1P_CFG_PF_USR_pf_stream_optimistic | L1P_CFG_PF_USR_pf_stream_prefetch_enable | L1P_CFG_PF_USR_pf_stream_establish_enable; // Enable everything???

  if (prefetchlist) {
    L1P_PatternConfigure(1024*1024);
  }
#endif
}


static uint64_t bgq_wcycles() {
#ifdef BGQ_SPI
  return GetTimeBase();
#else
  return 0;
#endif
}


static void donothing(void *arg, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  DelayTimeBase(1600*100);
#endif
}


static void L1Pstart(void *arg_untyped, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  int *record = (int*)arg_untyped;
  L1P_PatternStart(*record);
  L1P_PatternPause();
#endif
}
 

static void L1Pstop(void *arg_untyped, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  L1P_PatternStop();
#endif
}


static void mypapi_start_worker(void *arg_untyped, size_t tid, size_t threads) {
  mypapi_work_t *arg = (mypapi_work_t *)arg_untyped;
  mypapi_start(arg->set);
}

static void mypapi_stop_worker(void *arg_untyped, size_t tid, size_t threads) {
  mypapi_work_t *arg = (mypapi_work_t *)arg_untyped;
    arg->result = mypapi_stop();
}

static void benchmark_free_worker(void *argptr, size_t tid, size_t threads) {
  master_args *args = (master_args*)argptr;
  bgq_hmflags opts = args->opts;
  bool prefetchlist = opts | hm_prefetchlist;

  mypapi_free();
#ifdef BGQ_SPI
  if (prefetchlist) {
    L1P_PatternUnconfigure();
  }
#endif
}



//static uint64_t flopaccumulator = 0;
//void __molly_add_flops(uint64_t flops) {
//  flopaccumulator += flops;
//}

typedef void (*bgq_worker_func)(void *arg, size_t tid, size_t threads);
typedef int (*bgq_master_func)(void *arg);


static int bgq_parallel(bgq_master_func master_func, void *master_arg) {
  assert(master_func);
  return (*master_func)(master_arg);
}


static void bgq_master_call(bgq_worker_func worker_func, void *arg) {
  assert(worker_func);
  (*worker_func)(arg, 0, 1);
}


static void bgq_master_sync() {
}


static int runcheck(bgq_hmflags hmflags) {
  return 0;
}

typedef  int64_t scoord;

static bool g_bgq_dispatch_l1p = false;
static int64_t g_nproc = -1;


static int benchmark_master(void *argptr) {
  master_args * const args = (master_args *) argptr;
  auto nFlopsPerStencil = args->nFlopsPerStencil;
  auto nStencilsPerTest = args->nStencilsPerTest;
  const int j_max = args->j_max;
  const benchfunc_t &benchfunc = *args->benchfunc;
  const bgq_hmflags opts = args->opts;
  const bool nocom = opts & hm_nocom;
  //const bool nooverlap = opts & hm_nooverlap;
  //const bool nokamul = opts & hm_nokamul;
  const bool prefetchlist = opts & hm_prefetchlist;
  //const bool noprefetchstream = opts & hm_noprefetchstream;
  //const bool noprefetchexplicit = opts & hm_noprefetchexplicit;
  //const bool noweylsend = opts & hm_noweylsend;
  //const bool nobody = opts & hm_nobody;
  //const bool nosurface = opts & hm_nosurface;
  //const bool experimental = opts & hm_experimental;
  bool floatprecision = opts & hm_floatprecision;
  //const bgq_hmflags implicitprefetch = opts & (hm_prefetchimplicitdisable | hm_prefetchimplicitoptimistic | hm_prefetchimplicitconfirmed);
  bool withcheck = opts & hm_withcheck;

  // Setup thread options (prefetch setting, performance counters, etc.)
  bgq_master_call(&benchmark_setup_worker, argptr);


  double err = 0;
  if (withcheck) {
    err = runcheck(opts);
  }


  // Give us a fresh environment
  if (nocom) {
    //for (ucoord d = 0; d < PHYSICAL_LD; d+=1) {
      //memset(g_bgq_sec_recv_double[d], 0, bgq_section_size(bgq_direction2section(d, false)));
    //}
  }
  //for (ucoord k = 0; k < k_max; k+=1) {
    //random_spinor_field(g_spinor_field[k], VOLUME/2, 0);
  //}
  const int warmups = 2;

  uint64_t sumotime = 0;
  for (int i = 0; i < 20; i += 1) {
    uint64_t start_time = bgq_wcycles();
    donothing(NULL, 0, 0);
    uint64_t mid_time = bgq_wcycles();
    bgq_master_call(&donothing, NULL);
    uint64_t stop_time = bgq_wcycles();

    uint64_t time = (stop_time - mid_time) - (mid_time- start_time);
    sumotime += time;
  }
  double avgovhtime = (double)sumotime / 20.0;

  static mypapi_work_t mypapi_arg;
  double localsumtime = 0;
  double localsumsqtime = 0;
  uint64_t localsumcycles=0;
  uint64_t localsumflop = 0;
  mypapi_counters counters;
  counters.init = false;
  int iterations = warmups; // Warmup phase
  iterations += j_max;
  if (iterations < warmups + MYPAPI_SETS)
    iterations = warmups + MYPAPI_SETS;

  for (int i = 0; i < iterations; i += 1) {
    //master_print("Starting iteration %d of %d\n", j+1, iterations);
    bool isWarmup = (i < warmups);
    int j = i - warmups;
    bool isPapi = !isWarmup && (i >= iterations - MYPAPI_SETS);
    int papiSet = i - (iterations - MYPAPI_SETS);
    bool isJMax = (0 <= j) && (j < j_max);

    double start_time;
    uint64_t start_cycles;
    uint64_t start_flop;
    if (isJMax) {
      //start_flop = flopaccumulator;
    }
    if (isPapi) {
      bgq_master_sync();
      mypapi_arg.set = papiSet;
      bgq_master_call(mypapi_start_worker, &mypapi_arg);
    }
    if (isJMax) {
      start_time = MPI_Wtime();
      start_cycles = bgq_wcycles();
    }

    if (prefetchlist) {
      int record = isWarmup;
      // TODO: Might be done in bgq_dispatch, such no additional call is required
      bgq_master_call(L1Pstart, &record);
      bgq_master_sync();
      // TODO: If L1Pstart and L1Pstop are special-cased, this could be done just around the loop
      g_bgq_dispatch_l1p = true;
    }

    {
      // The main benchmark
      //for (int k = 0; k < k_max; k += 1) {
        // Note that flops computation assumes that readWeyllayout is used
        benchfunc(j, opts);
      //}

      bgq_master_sync(); // Wait for all threads to finish, to get worst thread timing
    }

    if (prefetchlist) {
      bgq_master_sync();
      g_bgq_dispatch_l1p = false;
      bgq_master_call(L1Pstop, NULL);
    }

    double end_time;
    uint64_t end_cycles;
    if (isJMax) {
      end_cycles = bgq_wcycles();
      end_time = MPI_Wtime();
    }
    if (isPapi) {
      bgq_master_call(mypapi_stop_worker, &mypapi_arg);
#ifdef PAPI
      counters = mypapi_merge_counters(&counters, &mypapi_arg.result);
#endif
    }

    if (isJMax) {
      double duration = end_time - start_time;
      localsumtime += duration;
      localsumsqtime += sqr(duration);
      localsumcycles += (end_cycles - start_cycles);
      //localsumflop += (flopaccumulator - start_flop);
    }
  }

  bgq_master_call(&benchmark_free_worker, argptr);

  ucoord its = j_max;
  double localavgtime = localsumtime / its;
  double localavgsqtime = sqr(localavgtime);
  double localrmstime = sqrt((localsumsqtime / its) - localavgsqtime);
  double localcycles = (double)localsumcycles / (double)its;
  //double localavgflop = (double)localsumflop / (double)its;

  double localtime[] = { localavgtime, localavgsqtime, localrmstime };
  double sumreduce[3] = { -1, -1, -1 };
  MPI_Allreduce(&localtime, &sumreduce, 3, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  double sumtime = sumreduce[0];
  double sumsqtime = sumreduce[1];
  double sumrmstime = sumreduce[2];

  double avgtime = sumtime / g_nproc;
  double avglocalrms = sumrmstime / g_nproc;
  double rmstime = sqrt((sumsqtime / g_nproc) - sqr(avgtime));

  // Assume k_max lattice site updates, even+odd sites
  //ucoord sites = LOCAL_LT * LOCAL_LX * LOCAL_LY * LOCAL_LZ;
  //ucoord sites_body = PHYSICAL_BODY * PHYSICAL_LK * PHYSICAL_LP;
  //ucoord sites_surface = PHYSICAL_SURFACE * PHYSICAL_LK * PHYSICAL_LP;
  //assert(sites == sites_body+sites_surface);
  //assert(sites == VOLUME);

  //ucoord lup_body = k_max * sites_body;
  //ucoord lup_surface = k_max * sites_surface;
  ucoord lup = nStencilsPerTest * j_max;
  auto flops = nFlopsPerStencil * lup;

  benchstat *result = &args->result;
  result->avgtime = avgtime;
  result->localrmstime = avglocalrms;
  result->globalrmstime = rmstime;
  result->totcycles = localcycles;
  //result->localavgflop = localavgflop;

  result->nStencilsPerTest = nStencilsPerTest;
  result->nFlopsPerStencil = nFlopsPerStencil;
  
  //result->sites_surface = sites_surface;
  //result->sites_body = sites_body;
  //result->sites = sites;
  //result->lup_surface = lup_surface;
  //result->lup_body = lup_body;
  result->lup = lup;
  //result->flops = flops / avgtime;
  result->error = err;
  result->counters = counters;
  result->opts = opts;
  result->avgovhtime = avgovhtime;
  return EXIT_SUCCESS;
}


static benchstat runbench(const benchfunc_t &benchfunc, bgq_hmflags opts, int j_max, int ompthreads, uint64_t nStencilsPerTest, uint64_t nFlopsPerStencil) {
#ifdef OMP
  omp_set_num_threads(ompthreads);
#endif
  master_args args;
  args.j_max = j_max;
  args.benchfunc = &benchfunc;
  args.opts = opts;
  args.nStencilsPerTest = nStencilsPerTest;
  args.nFlopsPerStencil = nFlopsPerStencil;
  int retcode = bgq_parallel(&benchmark_master, &args);
  assert(retcode == EXIT_SUCCESS);
  if (retcode != EXIT_SUCCESS) {
    exit(retcode);
  }

  return args.result;
}





static void print_stats(benchstat *stats) {
#if PAPI
#ifdef OMP
  int threads = omp_get_num_threads();
#else
  int threads = 1;
#endif
  
  for (int j = 0; j < __pi_COUNT; j+=1) {
    printf("%10s|", "");
    char *desc = NULL;

    for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
      char str[80];
      str[0] = '\0';
      benchstat *stat = &stats[i3];
      bgq_hmflags opts = stat->opts;

      double avgtime = stat->avgtime;
      uint64_t lup = stat->lup;
      double lups = lup / avgtime;
      //uint64_t flop = compute_flop(opts, stat->lup_body, stat->lup_surface);
      uint64_t flop = stat->lup * stat->nStencilsPerTest * stat->nFlopsPerStencil;
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
	case pi_mlups:
	  desc = "sites per sec";
	  snprintf(str, sizeof(str), "%.2f mlup/s", lup / MEGA);
	  break;
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

      printf("%" SCELLWIDTH "s|", str);
    }

    printf(" %s\n", desc);
  }
#endif
}

int g_proc_id;

static void exec_table(const benchfunc_t &benchmark, bgq_hmflags additional_opts, bgq_hmflags kill_opts, int j_max, uint64_t nStencilsPerTest, uint64_t nFlopsPerStencil) {
//static void exec_table(bool sloppiness, hm_func_double hm_double, hm_func_float hm_float, bgq_hmflags additional_opts) {
  g_nproc = __molly_cluster_rank_count();
  g_proc_id = __molly_cluster_mympirank();
  
  benchstat excerpt;

  if (g_proc_id == 0)
    printf("%10s|", "");
  for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
    if (g_proc_id == 0) {
      auto flagdesc = flags_desc[i3];
      printf("%-" SCELLWIDTH "s|", flagdesc);
    }
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
      bgq_hmflags hmflags = (bgq_hmflags)flags[i3];
      hmflags = (bgq_hmflags)((hmflags | additional_opts) & ~kill_opts);
 
      benchstat result = runbench(benchmark, hmflags, j_max, threads,  nStencilsPerTest,  nFlopsPerStencil);
      stats[i3] = result;

      if (threads==1 && i3 == 0) {
        excerpt = result;
      }
 
      char str[80] = { 0 };
      if (result.avgtime == 0)
        snprintf(str, sizeof(str), "~ %s", (result.error > 0.001) ? "X" : "");
      else
        //snprintf(str, sizeof(str), "%.2f mlup/s%s", (double) result.lup / (result.avgtime * MEGA), (result.error > 0.001) ? "X" : "");
	snprintf(str, sizeof(str), "%.2f msecs", result.avgtime / MILLI);
      if (g_proc_id == 0)
        printf("%" SCELLWIDTH "s|", str);
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


void molly::exec_bench(const benchfunc_t &func, int nTests, uint64_t nStencilsPerTest, uint64_t nFlopsPerStencil) {
  printf("MK nFlopsPerStencil : %llu\n", nFlopsPerStencil);
  exec_table(func, (bgq_hmflags)0, (bgq_hmflags)0, nTests, nStencilsPerTest, nFlopsPerStencil);
}
