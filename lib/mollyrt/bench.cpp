#define __MOLLYRT
#include "bench.h"

#include <cassert> // assert()
#include <cstdlib> // abort()
#include <cmath>   // sqrt()

#ifdef _MSC_VER
#define snprintf _snprintf
#endif
#ifndef lengthof
#define lengthof(X) (sizeof(X)/sizeof((X)[0]))
#endif
#ifndef STRINGIFY
#define STRINGIFY(V) #V
#endif
#ifndef TOSTRING
#define TOSTRING(V) STRINGIFY(V)
#endif

#define NANO  (1e-9)
#define MICRO (1e-6)
#define MILLI (1e-3)

#define KILO (1e3)
#define MEGA (1e6)
#define GIGA (1e9)
#define TERA (1e12)

#define KIBI (1024.0)
#define MEBI (1024.0*1024.0)
#define GIBI (1024.0*1024.0*1024.0)
#define TEBI (1024.0*1024.0*1024.0*1024.0)


struct bench_iteration_result_t;



#pragma region MPI Support
#if BENCH_MPI
#include <mpi.h>

#define ERROREXIT(...) \
  do { \
    fprintf(stderr, "\n" __VA_ARGS__); \
    abort(); \
          } while (0)

#define MPI_CHECK(CALL)                                         \
  do {                                                          \
/*    MOLLY_DEBUG(#CALL);              */                       \
    auto retval = (CALL);                                       \
    if (retval!=MPI_SUCCESS) {                                  \
      ERROREXIT("MPI fail: %s\nReturned: %d\n", #CALL, retval); \
                            }                                           \
              } while (0)

static inline int benchSelfRank() {
  int myrank;
  MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &myrank));
  return myrank;
}

static inline int benchWorldRanks() {
  int worldranks;
  MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &worldranks));
  return worldranks;
}

static inline double benchTime() {
  return MPI_Wtime();
}

static inline void benchBarrier() {
  MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}

#else
#error Please define BENCH_MPI (and the other macros)
#endif

bool isPrintingRank() {
  return benchSelfRank() == 0;
}

#pragma endregion


#pragma region BG/Q Support
#define L1P_CHECK(RTNCODE)                                                                 \
  do {                                                                                   \
      int mpi_rtncode = (RTNCODE);                                                       \
      if (mpi_rtncode != 0) {                                                  \
      fprintf(stderr, "L1P call %s at %s:%d failed: errorcode %d\n", #RTNCODE, __FILE__, __LINE__, mpi_rtncode);  \
      assert(!"L1P call " #RTNCODE " failed");                                       \
      abort();                                                                       \
                            }                                                                                  \
          } while (0)

#define BGPM_ERROR(cmd)                                                                                      \
	do {                                                                                                       \
		int RC = (cmd);                                                                                       \
		if (RC) {                                                                                   \
			 fprintf(stderr, "MK_BGPM call failed with code %d at line %d on MPI rank %d thread %d: %s\n", RC, __LINE__,  g_proc_id, Kernel_ProcessorID(), TOSTRING(cmd)); \
                            		}                                                                                                        \
          } while (0)

#if BENCH_BGQ
#define MYPAPI_SETS 6
#include <upci/events.h> // UPCI_NUM_EVENTS
static inline uint64_t bgq_wcycles() {
  return GetTimeBase();
}


static void mypapi_init() {
  BGPM_ERROR(Bgpm_Init(BGPM_MODE_SWDISTRIB));

  int tid = Kernel_ProcessorID();
  int cid = Kernel_ProcessorCoreID();
  int sid = Kernel_ProcessorThreadID();
  for (int i = 0; i < MYPAPI_SETS; i += 1) {
    PuEventSets[i][tid] = Bgpm_CreateEventSet();
    assert(PuEventSets[i][tid] >= 0);
  }

  if (tid == 0) {
    for (int i = 0; i < MYPAPI_SETS; i += 1) {
      L2EventSet[i] = Bgpm_CreateEventSet();
      assert(L2EventSet[i] >= 0);
    }
  }

  int j = 0;
  {
    int pues = PuEventSets[j][tid];
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_INST_ALL));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_IU_IL1_MISS_CYC));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_IU_IBUFF_EMPTY_CYC));
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_MISMATCH)); // Conflict set #3 // core address does not match a list address

    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_HITS));
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_MISSES));
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_FETCH_LINE)); // L2 lines loaded from main memory
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_STORE_LINE)); // L2 lines stored to main memory
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_PREFETCH));
      BGPM_ERROR(Bgpm_AddEvent(l2es, PEVT_L2_STORE_PARTIAL_LINE));
    }
  }

  j += 1;
  {
    int pues = PuEventSets[j][tid];
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_LSU_COMMIT_STS)); // Number of completed store commands.
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_LSU_COMMIT_LD_MISSES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_BAS_STRM_LINE_ESTB)); // Conflict set #1 // Lines established for stream prefetch
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_SKIP)); // Conflict set #2 // core address matched a non head of queue list address (per thread)
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_STARTED)); // Conflict set #4 // List prefetch process was started
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_OVF_MEM)); // Conflict set #5 // Written pattern exceeded allocated buffer

    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_DeleteEventSet(l2es));
      L2EventSet[j] = -1;
    }
  }

  j += 1;
  {
    int pues = PuEventSets[j][tid];
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_LSU_COMMIT_DCBT_MISSES)); // Number of completed dcbt[st][ls][ep] commands that missed the L1 Data Cache.
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_LSU_COMMIT_DCBT_HITS)); // Number of completed dcbt[st][ls][ep] commands that missed the L1 Data Cache.
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_CMP)); // Conflict set #1 // core address was compared against list
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_ABANDON)); // Conflict set #5 // A2 loads mismatching pattern resulted in abandoned list prefetch
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_LIST_CMP_OVRUN_PREFCH)); // Conflict set #6 // core address advances faster than prefetch lines can be established dropping prefetches

    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_DeleteEventSet(l2es));
      L2EventSet[j] = -1;
    }
  }

  j += 1;
  {
    int pues = PuEventSets[j][tid];
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_LSU_COMMIT_CACHEABLE_LDS)); // Number of completed cache-able load commands. (without dcbt)
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_INST_XU_ALL)); // All XU instructions completed (instructions which use A2 FX unit - UPC_P_XU_OGRP_*).
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_INST_QFPU_ALL)); // Count all completed instructions which processed by the QFPU unit (UPC_P_AXU_OGRP_*)
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_BAS_MISS));// Conflict set #4
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_BAS_HIT)); // Conflict set #2 // Hits in prefetch directory
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_BAS_LU_STALL_LIST_RD_CYC)); // Conflict Set #1 // Cycles lookup was held while list fetched addresses

    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_DeleteEventSet(l2es));
      L2EventSet[j] = -1;
    }
  }

  j += 1;
  {
    int pues = PuEventSets[j][tid];
    //BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_IU_IS1_STALL_CYC)); // Register Dependency Stall
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_IU_IS2_STALL_CYC)); // Instruction Issue Stall
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_STRM_EVICT_UNUSED)); // Conflict set #4 // per core // Lines fetched and never hit evicted
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_STRM_EVICT_PART_USED)); // Conflict set #5 // per core // Line fetched and only partially used evicted
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_STRM_LINE_ESTB)); // Conflict set #1 // per core // lines established for any reason and thread



    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_DeleteEventSet(l2es));
      L2EventSet[j] = -1;
    }
  }

  j += 1;
  {
    int pues = PuEventSets[j][tid];
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_CYCLES));
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_STRM_STRM_ESTB)); // Conflict set #1 // per thread // streams detected and established
    BGPM_ERROR(Bgpm_AddEvent(pues, PEVT_L1P_STRM_HIT_LIST)); // Conflict set #4 // per thread // Hits for lines fetched by list engine

    if (tid == 0) {
      int l2es = L2EventSet[j];
      BGPM_ERROR(Bgpm_DeleteEventSet(l2es));
      L2EventSet[j] = -1;
    }
  }

}

static void mypapi_free() {
  BGPM_ERROR(Bgpm_Disable());
}


static void mypapi_start(int i) {
#ifdef OMP
  if (omp_in_parallel()) {
    // Already in parallel, no need to start threads
    mypapi_start_work(i);
  } else {
    // Start counters in all threads
#pragma omp parallel
    {
      mypapi_start_work(i);
    }
  }
#else
  mypapi_start_work(i);
#endif
}


static void mypapi_stop(bench_result_t &result) {
  static mypapi_counters result; // static to force it shared between all threads, even if this func is called by all threads (i.e. in a #pragma omp parallel)
#ifdef OMP
  if (omp_in_parallel()) {
    // Already in parallel, no need to start threads
    mypapi_stop_work(&result);
  } else {
    // Start counters in all threads
#pragma omp parallel
    {
      mypapi_stop_work(&result);
    }
  }
#else
  mypapi_stop_work(&result);
#endif
  return result;
}


static void mypapi_print_counters(const bench_iteration_result_t &result) {
    fprintf(stderr, "*******************************************************************************\n");
    fprintf(stderr, "Set=%d eventset=%d thread=%d core=%d smt=%d omp=%d\n", counters->set, counters->eventset, counters->threadid, counters->coreid, counters->smtid, counters->ompid);
    if (counters->corecycles) {
      fprintf(stderr, "%10llu = %-30s\n", counters->corecycles, "Core cycles");
    }
    for (int i = 0; i < lengthof(counters->native); i += 1) {
      if (counters->active[i]) {
        uint64_t val = counters->native[i];
        //const char *label = Bgpm_GetEventIdLabel(i);
        Bgpm_EventInfo_t info;
        BGPM_ERROR(Bgpm_GetEventIdInfo(i, &info));

        fprintf(stderr, "%10llu = %-30s (%s)\n", val, info.label, info.desc);
      }
    }
    fprintf(stderr, "*******************************************************************************\n");
}

#else
#define UPCI_NUM_EVENTS 0
#define MYPAPI_SETS 0
static inline uint64_t bgq_wcycles() {
  return 0;
}

static inline void mypapi_init() {}
static inline void mypapi_free() {}
static inline void mypapi_start(int i) {}
static inline void mypapi_stop(bench_iteration_result_t &result) {}

static void mypapi_print_counters(const bench_iteration_result_t &result) {}

#endif



#pragma endregion


#pragma region Mutithreading
#if BENCH_BGQ_DISPATCH
#define MAX_THREADS 64
#include "bgq/bgq_dispatch.h"
#else 
#define MAX_THREADS 1

typedef void(*bgq_worker_func)(void *arg, size_t tid, size_t threads);
typedef int(*bgq_master_func)(void *arg);


static int bgq_parallel(bgq_master_func master_func, void *master_arg) {
  assert(master_func);
  return (*master_func)(master_arg);
}


struct bgq_master_functor_t {
  const std::function<int()> *func;
};
static int bgq_parallel_function_client(void *argptr) {
  const auto &arg = *reinterpret_cast<bgq_master_functor_t*>(argptr);
  return (*arg.func)();
}
static int bgq_parallel(const std::function<int()> &master_func) {
  bgq_master_functor_t arg;
  arg.func = &master_func;
  return bgq_parallel(&bgq_parallel_function_client, &arg);
}
static void bgq_set_threadcount(size_t nThreads) {
#ifdef OMP
  omp_set_num_threads(nThreads);
#else
  if (nThreads != 1) {
    printf("Multiple threads in non-OpenMP build\n"); abort();
  }
#endif
}


static void bgq_master_call(bgq_worker_func worker_func, void *arg) {
  assert(worker_func);
  (*worker_func)(arg, 0, 1);
}


struct bgq_functor_t {
  const std::function<void(size_t, size_t)> *func;
};
static void bgq_master_functor_call_client(void *argptr, size_t tid, size_t threads) {
  const auto &arg = *reinterpret_cast<bgq_functor_t*>(argptr);
  (*arg.func)(tid, threads);
}
static void bgq_master_call(const std::function<void(size_t, size_t)> &func) {
  bgq_functor_t arg;
  arg.func = &func;
  bgq_master_call(&bgq_master_functor_call_client, &arg);
}
static void bgq_master_sync() {
}
#endif
#pragma endregion


#pragma region Stats

static inline double sqr(double val) {
  return val*val;
}


struct bench_statistics {
  uint64_t count;
  double sum;
  double sqrsum;

  bench_statistics() : count(0), sum(0), sqrsum(0) {}
  void push(double val) {
    count += 1;
    sum += val;
    sqrsum += sqr(val);
  }

  void pushGlobal(double val) {
    double local[] = { val, sqr(val) };
    double global[2];
    MPI_Allreduce(&local, &global, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    int ranks;
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &ranks));
    sum += global[0];
    sqrsum += global[1];
    count += ranks;
  }

  void pushGlobal() {
    double local[] = { sum, sqrsum };
    double global[3];
    MPI_Allreduce(&local, &global, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    uint64_t globCount;
    MPI_Allreduce(&count, &globCount, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    //MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &count));
    sum = global[0];
    sqrsum = global[1];
    count = globCount;
  }

  //double sum() const { return sum; }
  double avg() const { return sum / count; }
  double rms() const { return sqrt(sqrsum / count); }
  double variance() const { return (sqrsum / count) - sqr(avg()); } // mean squared error / variance
  double stddeviation() const { return sqrt(variance()); }
};


#if 0
struct stat_of_stats {
  uint64_t count;
  double avgsum;

  uint64_t totalcount;
  double totalsum;

  stat_of_stats() : count(0), avgsum(0), totalcount(0), totalsum(0) {}

  template<typename Stat>
  void push(const Stat &stat) {
    count += 1;
    avgsum += stat.avg();

    totalcount += stat.count;
    totalsum += stat.sum;
  }

  double totalAvg() const { return totalsum / totalcount; }
};
#endif

#pragma endregion


struct bench_iteration_result_t {
  int itId;
  int threadId;
  int rankId;

  // per thread
  double duration;
  uint64_t cycles;
  uint64_t native[UPCI_NUM_EVENTS <= 0 ? 1 : UPCI_NUM_EVENTS];
  bool hasNative[UPCI_NUM_EVENTS <= 0 ? 1 : UPCI_NUM_EVENTS];

  bench_iteration_result_t() : itId(-1), threadId(-1), rankId(-1) {
    for (auto i = 0; i < lengthof(hasNative); i += 1) {
      hasNative[i] = false;
    }
  }
  // bench_iteration_result_t(int itId, int threadId, int rankId) : itId(itId), threadId(threadId), rankId(rankId) {}
};


struct bench_thread_result_t {
  int threadId;
  int rankId;

  bench_statistics duration;
  bench_statistics cycles;

  bench_iteration_result_t excerpt;

  bench_thread_result_t() : threadId(-1), rankId(-1) {}
  // bench_thread_result_t( int threadId, int rankId) : threadId(threadId), rankId(rankId) {}

  void push(const bench_iteration_result_t &iter) {
    assert(threadId == iter.threadId);
    assert(rankId == iter.rankId);

    duration.push(iter.duration);
    cycles.push(iter.cycles);

    if (excerpt.threadId == -1) {
      excerpt = iter;
    } else {
      excerpt.itId = -1;
      for (auto i = 0; i < UPCI_NUM_EVENTS; i += 1) {
        if (!excerpt.hasNative[i]) {
          excerpt.hasNative[i] = iter.hasNative[i];
          excerpt.native[i] = iter.native[i];
        }
      }
    }
    assert(excerpt.threadId != -1);
  }
};


struct bench_node_result_t {
  int rankId;

  bench_statistics durationAvg;
  bench_statistics cyclesAvg;

  bench_iteration_result_t excerpt;

  bench_statistics nodeDuration;
  bench_statistics globalDuration;

  bench_node_result_t() :rankId(-1) {}
  //bench_node_result_t( int rankId) : rankId(rankId) {}

  void push(const bench_thread_result_t &thread) {
    assert(rankId == thread.rankId);

    durationAvg.push(thread.duration.avg());
    cyclesAvg.push(thread.cycles.avg());
    if (excerpt.threadId == -1) {
      excerpt = thread.excerpt;
    }
    assert(excerpt.threadId != -1);
  }
};


struct bench_global_result_t {
  double error;

  bench_statistics durationAvgAvg;
  bench_statistics cyclesAvgAvg;

  bench_iteration_result_t excerpt;


  bench_global_result_t() : error(0){}

  void push(const bench_node_result_t &node) {
    durationAvgAvg.pushGlobal(node.durationAvg.avg());
    cyclesAvgAvg.pushGlobal(node.cyclesAvg.avg());

    if (excerpt.threadId == -1) {
      excerpt = node.excerpt;
    }
    assert(excerpt.threadId != -1);
  }
};



struct bench_exec_config_setup {
  const bench_exec_info_t &exec;
  bench_exec_config_setup(const bench_exec_info_t &exec) : exec(exec){}
  void operator() (size_t tid, size_t threads) { // Worker initialization
    mypapi_init();

#ifndef NDEBUG
    //feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
#endif

#if BENCH_BGQ
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
      L1P_PatternConfigure(1024 * 1024);
    }
#endif
  }
};


struct bench_exec_config_release {
  const bench_exec_info_t &exec;
  bench_exec_config_release(const bench_exec_info_t &exec) : exec(exec){}
  void operator() (size_t tid, size_t threads) { // Worker initialization
    mypapi_free();

#if BENCH_BGQ
    L1P_PatternUnconfigure();
#endif
  }
};


enum pprefetch_mode {
  l1p_disable = 0,
  l1p_play = 1 << 0,
  l1p_learn = 1 << 1
};


struct bench_iteration {
  const bench_exec_info_t &exec;
  const int iteration;
  const int rankId;
  const int bgpmSet;
  const pprefetch_mode l1pmode;

  bench_iteration_result_t *results;

  bench_iteration(bench_iteration_result_t *results, const bench_exec_info_t &exec, int iteration, int rankId, int bgpmSet, pprefetch_mode l1pmode) : results(results), exec(exec), iteration(iteration), rankId(rankId), bgpmSet(bgpmSet), l1pmode(l1pmode) {}
  void operator() (size_t tid, size_t threads) {
    auto &measure = results[tid];
    measure.itId = iteration;
    measure.threadId = tid;
    measure.rankId = rankId;

    if (exec.preparefunc) exec.preparefunc(tid, threads);
    bool bgpming = bgpmSet >= 0;

    if (l1pmode) {
#if BENCH_BGQ
      L1P_PatternStart(l1pmode & l1p_learn);
      L1P_PatternPause();
#endif
    }

    if (bgpming) {
      mypapi_start(bgpmSet);
    }

    if (l1pmode) {
#if BENCH_BGQ
      L1P_PatternResume();
#endif
    }

    auto start_time = benchTime();
    auto start_cycles = bgq_wcycles();
    exec.func(tid, threads);
    auto stop_cycles = bgq_wcycles();
    auto stop_time = benchTime();

    if (l1pmode) {
#if BENCH_BGQ
      L1p_PatternPause();
#endif
    }

    if (bgpming) {
      mypapi_stop(measure);
    }

    if (l1pmode) {
#if BENCH_BGQ
      L1P_PatternStop();
#endif
    }


    measure.duration = stop_time - start_time;
    measure.cycles = stop_cycles - start_cycles;
  }
};




struct bench_iter_conf_t {
  pprefetch_mode l1pmode;
  int bgpmSet;
  bool benching;
};

static const bench_iter_conf_t bench_iterconfs[] = {
    { l1p_disable, -1, false }, // Warmup
    { l1p_disable, -1, false },
    { l1p_disable, -1, true }, // Benchmarking
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, -1, true },
    { l1p_disable, 0, false }, // BGPM
    { l1p_disable, 1, false },
    { l1p_disable, 2, false },
    { l1p_disable, 3, false },
    { l1p_disable, 4, false },
    { l1p_disable, 5, false },
};


static const bench_iter_conf_t bench_ppreferch_iterconfs[] = {
    { l1p_disable, -1, false }, // Warmup
    { l1p_disable, -1, false },
    { l1p_learn, -1, false }, // L1p learn 
    { l1p_play, -1, true }, // Benchmarking
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, 0, false }, // BGPM
    { l1p_play, 1, false },
    { l1p_play, 2, false },
    { l1p_play, 3, false },
    { l1p_play, 4, false },
    { l1p_play, 5, false },
};


struct bench_exec_config_master {
  bench_global_result_t &global;
  const bench_exec_info_t &exec;
  const int rankId;
  int nThreads;
  bench_exec_config_master(bench_global_result_t &result, const bench_exec_info_t &exec, int rankId, int nThreads) :global(result), exec(exec), rankId(rankId), nThreads(nThreads) {};
  int operator()() {
    auto iterconfigs = exec.pprefetch ? bench_ppreferch_iterconfs : bench_iterconfs;
    auto totalIterations = exec.pprefetch ? lengthof(bench_ppreferch_iterconfs) : lengthof(bench_iterconfs);

    // Setup thread options (prefetch setting, performance counters, etc.)
    bgq_master_call(bench_exec_config_setup(exec));


    bench_node_result_t nodeResult;
    nodeResult.rankId = rankId;

    bench_thread_result_t threadsIter[MAX_THREADS];
    for (auto i = 0; i < nThreads; i += 1) {
      auto &thread = threadsIter[i];
      thread.rankId = rankId;
      thread.threadId = i;
    }

    for (int i = 0; i < totalIterations; i += 1) {
      auto &iterconfig = iterconfigs[i];
      auto bgpmset = iterconfig.bgpmSet;
      auto benching = iterconfig.benching;
      auto l1pmode = iterconfig.l1pmode;

      // setup
      bench_iteration_result_t iterResults[MAX_THREADS];
      bench_iteration iter(iterResults, exec, i, rankId, bgpmset, l1pmode);

      // execute
      benchBarrier();
      auto start_time = benchTime();
      bgq_master_call(iter);
      bgq_master_sync(); // Wait for all threads to finish, to get worst thread timing
      auto stop_time_node = benchTime();
      benchBarrier();
      auto stop_time_global = benchTime();

      if (benching) {
        double nodeDuration = stop_time_node - start_time;
        double globalDuration = stop_time_global - start_time;

        nodeResult.nodeDuration.push(nodeDuration);
        nodeResult.globalDuration.push(globalDuration);
        for (auto i = 0; i < nThreads; i += 1) {
          threadsIter[i].push(iterResults[i]);
        }
      }
    }

    for (auto i = 0; i < nThreads; i += 1) {
      nodeResult.push(threadsIter[i]);
    }


    global.push(nodeResult);

    // Release thread options
    bgq_master_call(bench_exec_config_release(exec));


    return EXIT_SUCCESS;
  }
};



static void bench_exec_config(bench_global_result_t &result, const bench_exec_info_t &exec, int rankId, int nThreads) {
  bench_exec_config_master masterfunc(result, exec, rankId, nThreads);

  bgq_set_threadcount(nThreads);
  auto retcode = bgq_parallel(masterfunc);
  assert(retcode == EXIT_SUCCESS);
  if (retcode != EXIT_SUCCESS) {
    fprintf(stderr, "Master program returned error code %d\n", retcode);
    exit(retcode);
  }
}


static void print_repeat(const char * const str, const int count) {
  if (!isPrintingRank())
    return;

  for (int i = 0; i < count; i += 1) {
    printf("%s", str);
  }
}


typedef enum {
  pi_mlups,
  pi_flopsref,
  pi_floppersite,
  pi_msecs,
  pi_cycpersite,

  pi_localrms,
  pi_globalrms,
  pi_avgovhtime,

  pi_cpi,
  pi_l1istalls,
  pi_axufraction,
  //pi_overhead,

  pi_is1stalls,
  pi_is2stalls,

  pi_hitinl1,
  pi_l1phitrate,
  pi_l2hitrate,
  pi_dcbthitrate,

  pi_detstreams,
  pi_l1pstreamunusedlines,

  pi_ramfetchrate,
  pi_ramstorerate,
  pi_ramstorepartial,
  pi_l2prefetch,

  __pi_COUNT,
  pi_correct,
  pi_corecpi,
  pi_instrpersite,
  pi_fxupersite,
  pi_flops,
  pi_l1pstreamhitinl1p,
  pi_hitinl1p,
  pi_l1pliststarted,
  pi_l1plistabandoned,
  pi_l1plistmismatch,
  pi_l1plistskips,
  pi_l1plistoverruns,
  pi_l1plistlatestalls
} mypapi_interpretations;


#define CELLWIDTH 15
#define SCELLWIDTH TOSTRING(CELLWIDTH)

static const char *print_stat(const std::vector<bench_global_result_t> &stats, const std::vector<bench_exec_info_t> &configs, mypapi_interpretations j) {
  auto nConfigs = stats.size();
  const char *desc = NULL;
  for (int i3 = 0; i3 < nConfigs; i3 += 1) {
    char str[80];
    str[0] = '\0';
    const auto &stat = stats[i3];
    const auto &config = configs[i3];
    //bgq_hmflags opts = stat->opts;

    auto avgtime = stat.durationAvgAvg.avg();
    auto lup = config.nStencilsPerCall;
    auto lups = lup / avgtime;
    auto flop = config.nStencilsPerCall * config.nFlopsPerCall;
    auto flops = (double)flop / avgtime;
    //auto localrms = stat->localrmstime / stat->avgtime;
    //auto globalrms = stat->globalrmstime / stat->avgtime;
    //auto sites = stat->sites;


#if BENCH_BGQ
    const auto &native = stat.excerpt.native;
    auto nCycles = native[PEVT_CYCLES];
    //auto nCycles = stat.cyclesAvgAvg.avg();
    //auto nCoreCycles = stats[i3].counters.corecycles;
    //auto nNodeCycles = stats[i3].counters.nodecycles;
    auto nInstructions = native[PEVT_INST_ALL];
    auto nStores = native[PEVT_LSU_COMMIT_STS];
    auto nL1IStalls = native[PEVT_LSU_COMMIT_STS];
    auto nL1IBuffEmpty = native[PEVT_IU_IBUFF_EMPTY_CYC];

    auto nIS1Stalls = native[PEVT_IU_IS1_STALL_CYC];
    auto nIS2Stalls = native[PEVT_IU_IS2_STALL_CYC];

    auto nCachableLoads = native[PEVT_LSU_COMMIT_CACHEABLE_LDS];
    auto nL1Misses = native[PEVT_LSU_COMMIT_LD_MISSES];
    auto nL1Hits = nCachableLoads - nL1Misses;

    auto nL1PMisses = native[PEVT_L1P_BAS_MISS];
    auto nL1PHits = native[PEVT_L1P_BAS_HIT];
    auto nL1PAccesses = nL1PHits + nL1PMisses;

    auto nL2Misses = native[PEVT_L2_MISSES];
    auto nL2Hits = native[PEVT_L2_HITS];
    auto nL2Accesses = nL2Misses + nL2Hits;

    auto nDcbtHits = native[PEVT_LSU_COMMIT_DCBT_HITS];
    auto nDcbtMisses = native[PEVT_LSU_COMMIT_DCBT_MISSES];
    auto nDcbtAccesses = nDcbtHits + nDcbtMisses;

    auto nXUInstr = native[PEVT_INST_XU_ALL];
    auto nAXUInstr = native[PEVT_INST_QFPU_ALL];
    auto nXUAXUInstr = nXUInstr + nAXUInstr;

    uint64_t nL1PListStarted = native[PEVT_L1P_LIST_STARTED];
    uint64_t nL1PListAbandoned = native[PEVT_L1P_LIST_ABANDON];
    uint64_t nL1PListMismatch = native[PEVT_L1P_LIST_MISMATCH];
    uint64_t nL1PListSkips = native[PEVT_L1P_LIST_SKIP];
    uint64_t nL1PListOverruns = native[PEVT_L1P_LIST_CMP_OVRUN_PREFCH];

    double nL1PLatePrefetchStalls = native[PEVT_L1P_BAS_LU_STALL_LIST_RD_CYC];

    uint64_t nStreamDetectedStreams = native[PEVT_L1P_STRM_STRM_ESTB];
    double nL1PSteamUnusedLines = native[PEVT_L1P_STRM_EVICT_UNUSED];
    double nL1PStreamPartiallyUsedLines = native[PEVT_L1P_STRM_EVICT_PART_USED];
    double nL1PStreamLines = native[PEVT_L1P_STRM_LINE_ESTB];
    double nL1PStreamHits = native[PEVT_L1P_STRM_HIT_LIST];

    double nDdrFetchLine = native[PEVT_L2_FETCH_LINE];
    double nDdrStoreLine = native[PEVT_L2_STORE_LINE];
    double nDdrPrefetch = native[PEVT_L2_PREFETCH];
    double nDdrStorePartial = native[PEVT_L2_STORE_PARTIAL_LINE];
#endif


    switch (j) {
    case pi_mlups:
      desc = "sites per sec";
      snprintf(str, sizeof(str), "%.2f mlup/s", lup / MEGA);
      break;
    case pi_correct:
      desc = "Max error to reference";
      if (config.comparefunc && config.reffunc) {
        snprintf(str, sizeof(str), "%g", stat.error);
      }
      break;
    case pi_msecs:
      desc = "Iteration time";
      snprintf(str, sizeof(str), "%.3f mSecs", avgtime / MILLI);
      break;
    case pi_flops:
      desc = "MFlop/s";
      snprintf(str, sizeof(str), "%.0f MFlop/s", flops / MEGA);
      break;
#if 0
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
#endif
#if BENCH_BGQ
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
      snprintf(str, sizeof(str), "%.2f %%", 100 * nDdrStorePartial / (nDdrStorePartial + nDdrStoreLine));
      break;
    case pi_l2prefetch:
      desc = "L2 prefetches";
      snprintf(str, sizeof(str), "%.2f %%", 100 * nDdrPrefetch / nDdrFetchLine);
      break;
    case pi_cycpersite:
      desc = "per site update";
      snprintf(str, sizeof(str), "%.1f cyc", nCycles / lup);
      break;
    case pi_instrpersite:
      desc = "instr per update";
      snprintf(str, sizeof(str), "%.1f", nInstructions / lup);
      break;
    case pi_fxupersite:
      desc = "FU instr per update";
      snprintf(str, sizeof(str), "%.1f", nAXUInstr / lup);
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
    case pi_hitinl1p:
      desc = "Loads that hit in L1P";
      snprintf(str, sizeof(str), "%f %%", 100 * nL1PHits / nCachableLoads);
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
#endif
    default:
      desc = NULL;
    }

    printf("%" SCELLWIDTH "s|", str);
  }

  return desc;
}


static void print_stats(const std::vector<bench_global_result_t> &stats, const std::vector<bench_exec_info_t> &execs, size_t threads) {
  for (int j = 0; j < __pi_COUNT; j += 1) {
    printf("%10s|", "");
    auto desc = print_stat(stats, execs, (mypapi_interpretations)j);
    if (desc)
      printf(" %s\n", desc);
    else
      printf("\n");
  }
}


static int omp_threads[] = { 1 };
static const char *omp_threads_desc[] = { "1" };

void bench_exec(const std::vector<bench_exec_info_t> &configs) {
  auto worldRanks = benchWorldRanks();
  auto selfRank = benchSelfRank();
  auto printing = isPrintingRank();
  auto nConfigs = configs.size();

  if (printing) {
    printf("%10s|", "");
    for (int i3 = 0; i3 < nConfigs; i3 += 1) {
      printf("%-" SCELLWIDTH "s|", configs[i3].desc);
    }
    printf("\n");

    print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * nConfigs);
    printf("\n");
  }
  bench_global_result_t excerpt;
  for (int i2 = 0; i2 < lengthof(omp_threads); i2 += 1) {
    auto threads = omp_threads[i2];
    //if (threads > maxThreads)
    //  break;

    if (printing)
      printf("%-10s|", omp_threads_desc[i2]);

    std::vector<bench_global_result_t> stats;
    stats.resize(nConfigs);

    for (int i3 = 0; i3 < nConfigs; i3 += 1) {
      const auto &config = configs[i3];
      auto &result = stats[i3];

      bench_exec_config(stats[i3], config, selfRank, threads);

      if (threads == 1 && i3 == 0) {
        excerpt = result;
      }

      if (printing) {
        char str[80] = { 0 };
        auto avg = result.durationAvgAvg.avg();
        if (avg <= 0)
          snprintf(str, sizeof(str), "~ %s", (result.error > 0.001) ? "X" : "");
        else
          //snprintf(str, sizeof(str), "%.2f mlup/s%s", (double) result.lup / (result.avgtime * MEGA), (result.error > 0.001) ? "X" : "");
          snprintf(str, sizeof(str), "%.2f msecs", avg / MILLI);
        printf("%" SCELLWIDTH "s|", str);
        fflush(stdout);
      }
    }
    if (printing) {
      printf("\n");
      print_stats(stats, configs, threads);
      print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * nConfigs);
      printf("\n");
    }
  }

  if (printing) {
    printf("Hardware counter excerpt (1 thread, first config):\n");
    mypapi_print_counters(excerpt.excerpt);
    printf("\n");
  }
}


struct bench_nullfunc_t {
  void operator()(size_t, size_t) {}
};
static bench_nullfunc_t bench_nullfunc;

struct bench_nullcmpfunc_t {
  double operator()(size_t, size_t) { return 0; }
};
static bench_nullcmpfunc_t bench_nullcmpfunc;



bench_exec_info_t::bench_exec_info_t() : desc(""), preparefunc(bench_nullfunc), func(bench_nullfunc), reffunc(bench_nullfunc), comparefunc(bench_nullcmpfunc),
nStencilsPerCall(0), nFlopsPerCall(0), nLoadedBytesPerCall(0), nStoredBytesPerCall(0), nWorkingSet(0), prefetch(prefetch_default), pprefetch(false) {}


















#if 0


#include "molly.h"
#include "mypapi.h"
#include <mpi.h>


#ifdef BGQDISPATCH
#else
#endif

#ifdef BGPM
#include <bgpm/include/bgpm.h>
#else
#endif

#ifdef BGQLISTPREFETCH
#include <l1p/pprefetch.h>
#else
#endif


#ifdef BGQ_SPI
#include <cmath>

#include <hwi/include/bqc/A2_inlines.h> // GetTimeBase()

#include <l1p/sprefetch.h>
#endif

#include <math.h>












using namespace molly;






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

#endif

