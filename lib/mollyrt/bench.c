#define __MOLLYRT
#include "bench.h"

#include <assert.h> // assert()
#include <stdlib.h> // abort()
#include <math.h>   // sqrt()
#include <stdio.h>  // fprintf, stderr
#include <string.h> // memset
#include <sys/time.h> // gettimeofday

#if defined(__cplusplus)
#include <functional>
#include <vector>
#endif

#if BENCH_BGQ

#ifdef __clang__
#include "hwi/include/common/compiler_support.h"
__INLINE__ uint64_t Kernel_GetJobID(); // Error in IBM header files: Kernel_GetJobID not forward declared as inline in process.h
#endif
#include <spi/include/kernel/process.h>
#include <spi/include/kernel/location.h>

// Link to library: /bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a
#include <upci/events.h> // UPCI_NUM_EVENTS

#if 0
#include <upci/upc_c.h> // UPC_Ctr_Mode_t
#include <upci/punit_config.h> // Upci_Punit_Cfg_t

__INLINE__ int Kernel_Upci_ResetInts(uint64_t intStatusMask); // Error in IBM header files: Kernel_Upci_ResetInts not forward declared as inline in upci.h
struct sUpci_Node_Parms;
typedef struct sUpci_Node_Parms Upci_Node_Parms_t;
__INLINE__ int Kernel_Upci_GetNodeParms(Upci_Node_Parms_t *pNodeParms); // dito
struct sUpci_KDebug;
typedef struct sUpci_KDebug Upci_KDebug_t;
__INLINE__ int Kernel_Upci_GetKDebug(Upci_KDebug_t *pKDebug); // dito
__INLINE__ int Kernel_Upci_Mode_Init ( unsigned upcMode, UPC_Ctr_Mode_t ctrMode, unsigned unit ); // dito
__INLINE__ int Kernel_Upci_Mode_Free ( ); // dito
struct sUpci_A2PC;
typedef struct sUpci_A2PC Upci_A2PC_t;
__INLINE__ int Kernel_Upci_A2PC_ApplyRegs( Upci_A2PC_t *pA2pc); // dito
struct sUpci_A2PC_Val;
typedef struct sUpci_A2PC_Val Upci_A2PC_Val_t;
__INLINE__ int Kernel_Upci_A2PC_GetRegs( Upci_A2PC_Val_t *pA2pcVal); // dito
__INLINE__ int Kernel_Upci_Punit_Cfg_Apply( Upci_Punit_Cfg_t *pCfg, unsigned unitId); // dito
__INLINE__ int Kernel_Upci_Punit_Cfg_Attach( Upci_Punit_Cfg_t *pCfg, unsigned unitId); // dito
__INLINE__ int Kernel_Upci_SetBgpmThread(); // dito
__INLINE__ int Kernel_Upci_ClearBgpmThread(); // dito
__INLINE__ int Kernel_Upci_Wait4MailboxEmpty(); // dito
__INLINE__ int Kernel_Upci_SetPmSig(int sig); // dito
__INLINE__ int Kernel_Upci_ResetInts(uint64_t intStatusMask); // dito
__INLINE__ int Kernel_Upci_GetCNKCounts( Upci_CNKCtrType_t ctrType, PerfCountItem_t *ctrBuff, int buffLen); // dito
#include <kernel/upci.h> // Upci_CNKCtrType_t
//__INLINE__ int Kernel_Upci_GetCNKCounts( Upci_CNKCtrType_t ctrType, PerfCountItem_t *ctrBuff, int buffLen); // dito
#endif
#include <bgpm/include/bgpm.h>

// Link to library: /bgsys/drivers/ppcfloor/spi/lib/libSPI_l1p.a
// Link to library: /bgsys/drivers/ppcfloor/spi/lib/libSPI_cnk.a
#include <l1p/pprefetch.h>
#include <l1p/sprefetch.h>
#else
#define UPCI_NUM_EVENTS 0
#endif

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

              
typedef enum {
  per_none,
  per_thread,
  per_core,
  per_node
} mypapi_counter_mode;


typedef struct bench_iteration_result_t {
  int itId;
  int threadId;
  int coreId;
  int rankId;

  // per thread
    uint64_t start_cycles;
  double start_time;
  
  double duration;
  uint64_t cycles;
  uint64_t native[UPCI_NUM_EVENTS <= 0 ? 1 : UPCI_NUM_EVENTS];
  mypapi_counter_mode active[UPCI_NUM_EVENTS <= 0 ? 1 : UPCI_NUM_EVENTS];

  //bench_iteration_result_t() : itId(-1), threadId(-1), rankId(-1) {
  //  for (auto i = 0; i < lengthof(hasNative); i += 1) {
  //    hasNative[i] = false;
  //  }
  //}
} bench_iteration_result_t;


#pragma region MPI Support
#if BENCH_MPI
#define MPICH_SKIP_MPICXX
#include <mpi.h>

#define ERROREXIT(...) \
  do { \
    fprintf(stderr, "\n" __VA_ARGS__); \
    abort(); \
              } while (0)

#define MPI_CHECK(CALL)                                         \
  do {                                                          \
/*    MOLLY_DEBUG(#CALL);              */                       \
    int retval = (CALL);                                       \
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

static bool isPrintingRank() {
  return benchSelfRank() == 0;
}


static bool isPrintingThread() {
  return (Kernel_ProcessorID()==0) && isPrintingRank();
}


#pragma endregion


#pragma region BG/Q Support
#define L1P_CHECK(RTNCODE)                                                                 \
  do {                                                                                   \
      int mpi_rtncode = (RTNCODE);                                                       \
      if (mpi_rtncode != 0) {                                                  \
        fprintf(stderr, "L1P call %s at %s:%d failed: errorcode %d\n", #RTNCODE, __FILE__, __LINE__, mpi_rtncode);  \
        abort(); \
      }                                                                                  \
   } while (0)

#define KERNEL_CHECK(RTNCODE)                                                                 \
  do {                                                                                   \
      int mpi_rtncode = (RTNCODE);                                                       \
      if (mpi_rtncode != 0) {                                                  \
        fprintf(stderr, "Kernel call %s at %s:%d failed: errorcode %d\n", #RTNCODE, __FILE__, __LINE__, mpi_rtncode);  \
        abort(); \
      }                                                                                  \
   } while (0)
   
#define BGPM_ERROR(cmd)                                                                                      \
	do {                                                                                                       \
		int RC = (cmd);                                                                                       \
		if (RC) {                                                                                   \
			 fprintf(stderr, "MK_BGPM call failed with code %d at line %d on MPI rank %d thread %d: %s\n", RC, __LINE__,  benchSelfRank(), Kernel_ProcessorID(), TOSTRING(cmd)); \
			 abort(); \
                                    		}                                                                                                        \
              } while (0)


              
#if BENCH_BGQ
#define MYPAPI_SETS 6

static inline uint64_t bgq_wcycles() {
  return GetTimeBase();
}

//int PAPI_Events[256];
//long long PAPI_Counters[256];

static long long xCyc;
static long long xNsec;
static double xNow;
static double xWtime;
#ifdef OMP
static double xOmpTime;
#endif


static double now2(){
   struct timeval t; double f_t;
   gettimeofday(&t, NULL);
   f_t = t.tv_usec; f_t = f_t/1000000.0; f_t +=t.tv_sec;
   return f_t;
}

static int mypapi_threadid() {
#ifdef OMP
	return omp_get_thread_num();
#else
	return -1;
#endif
}

static unsigned long int mypapi_getthreadid() {
	return Kernel_ProcessorID();
}

static double mypapi_wtime() {
	Personality_t personality;
	BGPM_ERROR(Kernel_GetPersonality(&personality, sizeof(Personality_t)));
	double freq = MEGA * personality.Kernel_Config.FreqMHz;
	long long cycles = GetTimeBase();
	return cycles / freq;
}

static double mypapi_omp_wtime() {
#ifdef OMP
  return omp_get_wtime();
#else
  return 0;
#endif
}


static int PuEventSets[MYPAPI_SETS][64] = {0};
static int L2EventSet[MYPAPI_SETS] = {0};

void mypapi_init() {
  // Call in each thread
  // Setups which performance counters to query
 //if (isPrintingRank()) fprintf(stderr, "MK mypapi_init()\n");
  
		BGPM_ERROR(Bgpm_Init(BGPM_MODE_SWDISTRIB));

		int tid = Kernel_ProcessorID();
		int cid = Kernel_ProcessorCoreID();
		int sid = Kernel_ProcessorThreadID();
		for (int i = 0; i < MYPAPI_SETS; i += 1) {
			PuEventSets[i][tid] = Bgpm_CreateEventSet();
			assert(PuEventSets[i][tid] >= 0);
		}

		// L2 events for the complete node, just the main thread to capture them
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
		
		j+=1;
		assert(j == MYPAPI_SETS);
}


static void mypapi_free() {
  BGPM_ERROR(Bgpm_Disable());
}


static int activeEventSet = -1;

static void mypapi_start(int i) {
  // Start performance counting of the given event set
	int tid = Kernel_ProcessorID();
	int cid = Kernel_ProcessorCoreID();
	int sid = Kernel_ProcessorThreadID();

	if (tid == 0) {
	  activeEventSet = i;
	}

	int pues = PuEventSets[i][tid];
	BGPM_ERROR(Bgpm_Apply(pues));
	if (tid == 0) {
		int l2es = L2EventSet[i];
		if (l2es > 0) {
			BGPM_ERROR(Bgpm_Apply(l2es));
			BGPM_ERROR(Bgpm_Start(l2es));
		}
	}
	BGPM_ERROR(Bgpm_Start(pues));
}


static void mypapi_stop(void) {
	int tid = Kernel_ProcessorID();
	int cid = Kernel_ProcessorCoreID();
	int sid = Kernel_ProcessorThreadID();
	int i = activeEventSet;
	assert(i >= 0);
	assert(i < MYPAPI_SETS);

	int pues = PuEventSets[i][tid];
	BGPM_ERROR(Bgpm_Stop(pues));
	if (tid == 0) {
		int l2es = L2EventSet[i];
		if (l2es >= 0) {
			BGPM_ERROR(Bgpm_Stop(l2es));
		}
	}
}


static void mypapi_bgpm_read(bench_iteration_result_t *result, int eventset, int set) {
	int threadid = Kernel_ProcessorID(); /* 0..63, Kernel_ProcessorCoreID() << 2 + Kernel_ProcessorThreadID() */
	int coreid = Kernel_ProcessorCoreID(); /* 0..15 */
	int smtid = Kernel_ProcessorThreadID(); /* 0..3 */

	int numEvts = Bgpm_NumEvents(eventset);
	assert(numEvts > 0);

	uint64_t cnt;
	for (int i = 0; i < numEvts; i += 1) {
		int eventid = Bgpm_GetEventId(eventset, i);
		mypapi_counter_mode mode = per_none;

		switch (eventid) {
		case PEVT_L1P_BAS_LU_STALL_SRT:
		case PEVT_L1P_BAS_LU_STALL_SRT_CYC:
		case PEVT_L1P_BAS_LU_STALL_MMIO_DCR:
		case PEVT_L1P_BAS_LU_STALL_MMIO_DCR_CYC:
		case PEVT_L1P_BAS_LU_STALL_STRM_DET:
		case PEVT_L1P_BAS_LU_STALL_STRM_DET_CYC:
		case PEVT_L1P_BAS_LU_STALL_LIST_RD:
		case PEVT_L1P_BAS_LU_STALL_LIST_RD_CYC:
		case PEVT_L1P_BAS_ST:
		case PEVT_L1P_BAS_LU_STALL_LIST_WRT:
		case PEVT_L1P_BAS_LU_STALL_LIST_WRT_CYC:

		case PEVT_L1P_STRM_LINE_ESTB:
		case PEVT_L1P_STRM_HIT_FWD:
		case PEVT_L1P_STRM_L1_HIT_FWD:
		case PEVT_L1P_STRM_EVICT_UNUSED:
		case PEVT_L1P_STRM_EVICT_PART_USED:
		case PEVT_L1P_STRM_REMOTE_INVAL_MATCH:
		case PEVT_L1P_STRM_DONT_CACHE:
		case PEVT_L1P_STRM_LINE_ESTB_ALL_LIST:
			// Per core counters
		  mode = per_core;
			if (smtid != 0)
				continue;
			break;
		case PEVT_L2_HITS:
		case PEVT_L2_MISSES:
		case PEVT_L2_FETCH_LINE:
		case PEVT_L2_STORE_LINE:
		case PEVT_L2_PREFETCH:
		case PEVT_L2_STORE_PARTIAL_LINE:
			// Per node counters, store just one event
		  mode = per_node;
			if (threadid != 0)
				continue;
			break;
			
		default:
		  mode = per_thread;
		}

		uint64_t cnt;
		BGPM_ERROR(Bgpm_ReadEvent(eventset, i, &cnt));
		result->native[eventid] = cnt;
		result->active[eventid] = mode;
	}
}



static void mypapi_fetch(bench_iteration_result_t *result) {
	int tid = Kernel_ProcessorID();
	int cid = Kernel_ProcessorCoreID();
	int sid = Kernel_ProcessorThreadID();
	int i = activeEventSet;
	assert(i >= 0);
	assert(i < MYPAPI_SETS);

	int pues = PuEventSets[i][tid];
	mypapi_bgpm_read(result, pues, i);
	if (tid == 0) {
		int l2es = L2EventSet[i];
		if (l2es >= 0) {
			mypapi_bgpm_read(result, l2es, i);
		}
	}
}


static void mypapi_print_counters(const bench_iteration_result_t *result) {
  printf("*******************************************************************************\n");
  printf("it=%d tid=%d rank=%d\n", result->itId, result->threadId, result->rankId);
  //if (counters->corecycles) {
  //  fprintf(stderr, "%10llu = %-30s\n", counters->corecycles, "Core cycles");
  //}
  for (int i = 0; i < lengthof(result->native); i += 1) {
    if (!result->active[i]) 
      continue;
    
    const char *per = "";
     switch(result->active[i]) {
       case per_thread:
	 //per = "thread";
	 break;
       case per_core:
	 per = ", core";
	 break;
       case per_node:
       per = ", node";
       break;
    }
      
      uint64_t val = result->native[i];
      //const char *label = Bgpm_GetEventIdLabel(i);
      Bgpm_EventInfo_t info;
      BGPM_ERROR(Bgpm_GetEventIdInfo(i, &info));

   
      printf("%10llu = %-30s (%s%s)\n", val, info.label, info.desc, per);
  }
  printf("*******************************************************************************\n");
}

#else
#define UPCI_NUM_EVENTS 0
#define MYPAPI_SETS 0
static inline uint64_t bgq_wcycles() {
  return 0;
}

static void mypapi_init(void) {}
static void mypapi_free(void) {}
static void mypapi_start(int i) {}
static void mypapi_stop(void) {}
static void mypapi_fetch(bench_iteration_result_t *result) {}

static void mypapi_print_counters(const bench_iteration_result_t *result) {}
#endif



#pragma endregion


#pragma region Mutithreading
#if 1
#define MAX_THREADS 64
#include "bgq_dispatch.h"
#else 
#define MAX_THREADS 1
#error Untested code
typedef void(*bgq_worker_func)(void *arg, size_t tid, size_t threads);
typedef int(*bgq_master_func)(void *arg);


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

#endif
#pragma endregion


#pragma region Stats

static inline double sqr(double val) {
  return val*val;
}


typedef struct {
  uint64_t count;
  double sum;
  double sqrsum;
} bench_statistics;

static inline void bench_statistics_ctor(bench_statistics *stat) {
  stat->count = 0;
  stat->sum = 0;
  stat->sqrsum = 0;
}


static inline void bench_statistics_push(bench_statistics *stat, double val) {
  stat->count += 1;
  stat->sum += val;
  stat->sqrsum += sqr(val);
}

static inline  void bench_statistics_pushGlobal(bench_statistics *stat, double val) {
  double local[] = { val, sqr(val) };
  double global[2];
  MPI_Allreduce(&local, &global, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  int ranks;
  MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &ranks));
  stat->sum += global[0];
  stat->sqrsum += global[1];
  stat->count += ranks;
}

#if 0
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
#endif

//double sum() const { return sum; }
double bench_statistics_avg(const bench_statistics *stat)  { return stat->sum / stat->count; }
double bench_statistics_rms(const bench_statistics *stat)  { return sqrt(stat->sqrsum / stat->count); }
double bench_statistics_variance(const bench_statistics *stat)  { return (stat->sqrsum / stat->count) - sqr(bench_statistics_avg(stat)); } // mean squared error / variance
double bench_statistics_stddeviation(const bench_statistics *stat)  { return sqrt(bench_statistics_variance(stat)); }



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






static void bench_iteration_result_ctor(bench_iteration_result_t *obj) {
  memset(obj, 0, sizeof(*obj));
  obj->itId = -1;
  obj->threadId = -1;
  obj->coreId = -1;
  obj->rankId = -1;
}


typedef struct {
  int threadId;
  int coreId;
  int rankId;

  bench_statistics duration;
  bench_statistics cycles;

  bench_iteration_result_t excerpt;
} bench_thread_result_t;


static void bench_thread_result_ctor(bench_thread_result_t *obj) {
  obj->threadId = -1;
  obj->coreId = -1;
  obj->rankId = -1;
  bench_statistics_ctor(&obj->duration);
  bench_statistics_ctor(&obj->cycles);
  bench_iteration_result_ctor(&obj->excerpt);
}


static void bench_thread_result_bgpmonly(bench_thread_result_t *obj, const bench_iteration_result_t *iter) {
  if (obj->excerpt.threadId == -1) {
    obj->excerpt = *iter;
  } else {
    obj->excerpt.itId = -1;
    for (int i = 0; i < UPCI_NUM_EVENTS; i += 1) {
      if (!obj->excerpt.active[i]) {
        obj->excerpt.active[i] = iter->active[i];
        obj->excerpt.native[i] = iter->native[i];
      }
    }
  }
  assert(obj->excerpt.threadId != -1);
}


static void bench_thread_result_push(bench_thread_result_t *obj, const bench_iteration_result_t *iter) {
  //fprintf(stderr, "MK obj->threadId=%d iter->threadId=%d\n", obj->threadId,iter->threadId);
  if (obj->threadId==-1) 
    obj->threadId = iter->threadId;
  if (obj->coreId==-1)
    obj->coreId = iter->coreId;
  if (obj->rankId==-1)
    obj->rankId=iter->rankId;
  
  assert(obj->threadId == iter->threadId);
  assert(obj->coreId == iter->coreId);
  assert(obj->rankId == iter->rankId);

  bench_statistics_push(&obj->duration, iter->duration);
  bench_statistics_push(&obj->cycles, iter->cycles);

  bench_thread_result_bgpmonly(obj, iter);
}




typedef struct {
  int rankId;

  bench_statistics durationAvg;
  bench_statistics cyclesAvg;

  bench_iteration_result_t excerpt;

  bench_statistics nodeDuration;
  bench_statistics globalDuration;
  
  // supporting threads
  bool threads[64];
  bool cores[16];
  int nThreads;
  int nCores;
} bench_node_result_t;

static void bench_node_result_ctor(bench_node_result_t *self, int rankId) {
  self->rankId = rankId;
  bench_statistics_ctor(&self->durationAvg);
  bench_statistics_ctor(&self->cyclesAvg);
  bench_iteration_result_ctor(&self->excerpt);
  bench_statistics_ctor(&self->nodeDuration);
  bench_statistics_ctor(&self->globalDuration);
  
  memset(&self->threads, 0, sizeof(self->threads));
  memset(&self->cores, 0, sizeof(self->cores));
  self->nThreads = 0;
  self->nCores = 0;
}

static void bench_node_result_push(bench_node_result_t *self, const bench_thread_result_t *thread) {
  assert(self->rankId == thread->rankId);

  bench_statistics_push(&self->durationAvg, bench_statistics_avg(&thread->duration));
  bench_statistics_push(&self->cyclesAvg, bench_statistics_avg(&thread->cycles));

  if (self->excerpt.threadId == -1) {
    self->excerpt = thread->excerpt;
  }
  assert(self->excerpt.threadId != -1);
  
  assert(0 <= thread->threadId && thread->threadId < 64);
  if (!self->threads[thread->threadId]) {
    self->nThreads += 1;
    self->threads[thread->threadId] = true;
  }
  assert(0 <= thread->coreId && thread->coreId < 16);
  if (!self->cores[thread->coreId]) {
    self->nCores += 1;
    self->cores[thread->coreId] = true;
  }
}



typedef struct {
  double error;

  bench_statistics durationAvgAvg;
  bench_statistics cyclesAvgAvg;
}bench_global_result_t;


static void bench_global_result_ctor(bench_global_result_t *self){
  self->error = 0;
  bench_statistics_ctor(&self->durationAvgAvg);
  bench_statistics_ctor(&self->cyclesAvgAvg);
}


static void bench_global_result_push(bench_global_result_t *self, const bench_node_result_t *node) {
  bench_statistics_pushGlobal(&self->durationAvgAvg, bench_statistics_avg(&node->durationAvg));
  bench_statistics_pushGlobal(&self->cyclesAvgAvg, bench_statistics_avg(&node->cyclesAvg));
}





void bench_exec_config_setup(void *uarg, size_t tid, size_t nThreads) { // Worker initialization
  const bench_exec_info_t *arg = (const bench_exec_info_t *)uarg;
  //printf("MK bench_exec_config_setup %llu %llu", tid, nThreads);
  mypapi_init();

#ifndef NDEBUG
  //feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
#endif

#if BENCH_BGQ
prefetch_stream_mode_t prefetch = arg->prefetch;
bool pprefetch = arg->pprefetch;

switch(prefetch) {
  case prefetch_default:
    break;
  case prefetch_disable:
    L1P_CHECK(L1P_SetStreamPolicy(L1P_stream_disable));
    break;
  case prefetch_confirmed:
    L1P_CHECK(L1P_SetStreamPolicy(L1P_confirmed_or_dcbt));
    break;
  case prefetch_optimistic:
    L1P_CHECK(L1P_SetStreamPolicy(L1P_stream_optimistic));
    break;
}

  if (pprefetch) {
    L1P_PatternConfigure(1024 * 1024); //TODO: what length ?
  }
#endif
}


static void bench_exec_config_release(void *argptr, size_t tid, size_t threads) {
  bench_exec_info_t *arg = (bench_exec_info_t *)argptr;

  mypapi_free();

#if BENCH_BGQ
  L1P_PatternUnconfigure();
#endif
}



typedef enum  {
  l1p_disabled = 0,
  l1p_play = 1 << 0,
  l1p_learn = 1 << 1
} pprefetch_mode;


typedef struct  {
  const bench_exec_info_t *exec;
  int iteration;
  int rankId;
  int bgpmSet;
  pprefetch_mode l1pmode;

  bench_iteration_result_t *results;
} bench_iteration_arg_t;




void bench_iteration_enter(void *argptr, size_t tid, size_t threads) {
    bench_iteration_arg_t* arg = (bench_iteration_arg_t*)argptr;
  const bench_exec_info_t *exec = arg->exec;
  const int iteration = arg->iteration;
  const int rankId = arg->rankId;
  const int bgpmSet = arg->bgpmSet;
   bool bgpming = bgpmSet >= 0;
  const pprefetch_mode l1pmode = arg->l1pmode;
  bench_iteration_result_t *results = arg->results;
  
  //fprintf(stderr, "MK_enter it=%d / rankId=%d benchSelfRank=%d / tid=%d Kernel_ProcessorID=%d\n", iteration, rankId, benchSelfRank(), tid, Kernel_ProcessorID());
  bench_iteration_result_t *measure = &results[tid];
  measure->itId = iteration;
  measure->threadId = Kernel_ProcessorID();
  measure->coreId = Kernel_ProcessorCoreID();
  measure->rankId = rankId;

  if (exec->preparefunc) (*exec->preparefunc)(exec->data, tid, threads);
 
  if (l1pmode) {
#if BENCH_BGQ
    L1P_PatternStart(l1pmode &l1p_learn);
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

  measure->start_time = benchTime();
  measure->start_cycles = bgq_wcycles();
  assert(exec->func);
}


void bench_iteration_exit(void *argptr, size_t tid, size_t threads) {
  bench_iteration_arg_t* arg = (bench_iteration_arg_t*)argptr;
  const bench_exec_info_t *exec = arg->exec;
  const int iteration = arg->iteration;
  const int rankId = arg->rankId;
  const int bgpmSet = arg->bgpmSet;
  bool bgpming = bgpmSet >= 0;
  const pprefetch_mode l1pmode = arg->l1pmode;
  bench_iteration_result_t *results = arg->results;
  bench_iteration_result_t *measure = &results[tid];
   
  assert(measure->itId == iteration);
  assert(measure->threadId==Kernel_ProcessorID());
  assert(measure->coreId == Kernel_ProcessorCoreID());
  assert(measure->rankId==rankId);
  
  uint64_t stop_cycles = bgq_wcycles();
  double stop_time = benchTime();
   
  if (l1pmode) {
#if BENCH_BGQ
    L1P_PatternPause();
#endif
  }
  
  if (bgpming) {
    mypapi_stop();
    mypapi_fetch(measure);
  }

  if (l1pmode) {
#if BENCH_BGQ
    L1P_PatternStop();
#endif
  }

  measure->duration = stop_time - measure->start_time;
  measure->cycles = stop_cycles - measure->start_cycles;
}


void bench_iteration(void *argptr, size_t tid, size_t threads) {
  bench_iteration_arg_t* arg = (bench_iteration_arg_t*)argptr;
   const bench_exec_info_t *exec = arg->exec;

  bench_iteration_enter(argptr, tid, threads);
  (*exec->func)(exec->data, tid, threads);
  bench_iteration_exit(argptr, tid, threads);
}


typedef struct  {
  pprefetch_mode l1pmode;
  int bgpmSet;
  bool benching;
} bench_iter_conf_t;

static const bench_iter_conf_t bench_iterconfs[] = {
  // Warmup
    { l1p_disabled, -1, false },
    { l1p_disabled, -1, false },

    // Benchmarking
    { l1p_disabled, -1, true },
    { l1p_disabled, -1, true },
    { l1p_disabled, -1, true },
    { l1p_disabled, -1, true },
    { l1p_disabled, -1, true },
    //{ l1p_disabled, -1, true },
    //{ l1p_disabled, -1, true },
    //{ l1p_disabled, -1, true },
    //{ l1p_disabled, -1, true },
    //{ l1p_disabled, -1, true },

    // BGPM
    { l1p_disabled, 0, false },
    { l1p_disabled, 1, false },
    { l1p_disabled, 2, false },
    { l1p_disabled, 3, false },
    { l1p_disabled, 4, false },
    { l1p_disabled, 5, false },
};


static const bench_iter_conf_t bench_ppreferch_iterconfs[] = {
  // Warmup
    { l1p_disabled, -1, false },
    { l1p_disabled, -1, false },

    // L1p learn 
    { l1p_learn, -1, false },

    // Benchmarking
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    { l1p_play, -1, true },
    //{ l1p_play, -1, true },
    //{ l1p_play, -1, true },
    //{ l1p_play, -1, true },
    //{ l1p_play, -1, true },
    //{ l1p_play, -1, true },

    // BGPM
    { l1p_play, 0, false },
    { l1p_play, 1, false },
    { l1p_play, 2, false },
    { l1p_play, 3, false },
    { l1p_play, 4, false },
    { l1p_play, 5, false },
};







typedef struct {
  bench_global_result_t *global;
  bench_node_result_t *rankResult;
  const bench_exec_info_t *exec;
  int rankId;
  int nThreads;
  //bench_exec_config_master(bench_global_result_t &result, const bench_exec_info_t &exec, int rankId, int nThreads) :global(result), exec(exec), rankId(rankId), nThreads(nThreads) {};
} bench_exec_config_master_data;

static int bench_exec_config_master(void *uarg) {
  bench_exec_config_master_data *arg = (bench_exec_config_master_data*)uarg;
  bench_global_result_t *global = arg->global;
  const bench_exec_info_t *exec = arg->exec;
  assert(exec);
  const int rankId = arg->rankId;
  const int nThreads = arg->nThreads;

  const bench_iter_conf_t *iterconfigs = exec->pprefetch ? &bench_ppreferch_iterconfs[0] : &bench_iterconfs[0];
  int totalIterations = exec->pprefetch ? lengthof(bench_ppreferch_iterconfs) : lengthof(bench_iterconfs);

  // Setup thread options (prefetch setting, performance counters, etc.)
  switch(exec->ompmode) {
    case omp_single:
      bench_exec_config_setup((void*)exec, 0, 1);
      break;
      
    case omp_plain: {
      bgq_adhoc_call(&bench_exec_config_setup, (void*)exec);
    } break;
      
    case omp_parallel:
    case omp_dispatch:
      bgq_master_call(&bench_exec_config_setup, (void*)exec);
      break;
  }
  
  bench_node_result_t *nodeResult = arg->rankResult;
  bench_thread_result_t threadsIter[MAX_THREADS];
  for (int i = 0; i < nThreads; i += 1) {
    bench_thread_result_t *thread = &threadsIter[i];
    bench_thread_result_ctor(thread);
  }

  for (int i = 0; i < totalIterations; i += 1) {
    const bench_iter_conf_t *iterconfig = &iterconfigs[i];
    assert(iterconfig);
    int bgpmset = iterconfig->bgpmSet;
    bool bgpming = bgpmset >= 0;
    bool benching = iterconfig->benching;
    pprefetch_mode l1pmode = iterconfig->l1pmode;

    // setup
    bench_iteration_result_t iterResults[MAX_THREADS];
    bench_iteration_arg_t iter;
    iter.results = &iterResults[0];
    iter.exec = exec;
    iter.iteration = i;
    iter.bgpmSet = bgpmset;
    iter.l1pmode = l1pmode;
    iter.rankId = rankId;

    // execute
    assert(exec);
    assert(exec->func);
    assert(exec->data);
    benchBarrier();
    double start_time = benchTime();
    switch(exec->ompmode) {
    case omp_single:
	  bench_iteration_enter(&iter, 0, 1);
	  (*exec->func)(exec->data, 0, 1);
	  bench_iteration_exit(&iter, 0, 1);
	  break;
	
    case omp_plain:{
	  bgq_adhoc_call(&bench_iteration_enter, (void*)&iter);
      (*exec->func)(exec->data, 0, nThreads);
	  bgq_adhoc_call(&bench_iteration_exit, (void*)&iter);
    } break;
    
    case omp_parallel:
	  bgq_master_call(&bench_iteration_enter, &iter);
	  (*exec->func)(exec->data, 0, nThreads);
	  bgq_master_call(&bench_iteration_exit, &iter);
	  break;

    case omp_dispatch:
	  bgq_master_call(bench_iteration, &iter);
	  break;
    }
    bgq_master_sync(); // Wait for all threads to finish, to get worst thread timing
    double stop_time_node = benchTime();
    benchBarrier();
    double stop_time_global = benchTime();

    if (benching) {
      double nodeDuration = stop_time_node - start_time;
      double globalDuration = stop_time_global - start_time;

      bench_statistics_push(&nodeResult->nodeDuration, nodeDuration);
      bench_statistics_push(&nodeResult->globalDuration, globalDuration);
      for (int i = 0; i < nThreads; i += 1) {
        bench_thread_result_push(&threadsIter[i], &iterResults[i]);
      }
    } else if (bgpming) {
      for (int i = 0; i < nThreads; i += 1) {
        bench_thread_result_bgpmonly(&threadsIter[i], &iterResults[i]);
      }
    }
  }

  for (int i = 0; i < nThreads; i += 1) {
    bench_node_result_push(nodeResult, &threadsIter[i]);
  }

  bench_global_result_push(global, nodeResult);

  // Release thread options
  switch(exec->ompmode) {
    case omp_single:
      bench_exec_config_release((void*)exec, 0, 1);
      break;
      
    case omp_plain: {
      bgq_adhoc_call(&bench_exec_config_release, (void*)exec);
    } break;
      
    case omp_parallel:
    case omp_dispatch:
      bgq_master_call(bench_exec_config_release, (void*)exec);
      break;
  }

  return EXIT_SUCCESS;
}




static void bench_exec_config(bench_global_result_t *result, bench_node_result_t *rankResult, const bench_exec_info_t *exec, int rankId, int nThreads) {
  //bench_exec_config_master masterfunc(result, exec, rankId, nThreads);
  bench_exec_config_master_data masterdata;
  masterdata.exec = exec;
  masterdata.rankId = rankId;
  masterdata.nThreads = nThreads;
  masterdata.global = result;
  masterdata.rankResult = rankResult;
  
  bgq_set_threadcount(nThreads);
  int retcode;
  switch(exec->ompmode) {
    case omp_single:
    case omp_plain:
      retcode = bench_exec_config_master(&masterdata);
      break;
      
    case omp_parallel:
    case omp_dispatch:
       retcode = bgq_parallel(bench_exec_config_master, &masterdata);
     break;
  }

  assert(retcode == EXIT_SUCCESS);
  if (retcode != EXIT_SUCCESS) {
    fprintf(stderr, "Master program returned error code %d\n", retcode);
    exit(retcode);
  }
}


static void print_repeat(const char * const str, size_t count) {
  if (!isPrintingRank())
    return;

  for (int i = 0; i < count; i += 1) {
    printf("%s", str);
  }
}


typedef enum {
  pi_cores,
  pi_ws,
  pi_flopPerStencil,
  
  pi_mlups,
  pi_mflops,
  pi_flopsPerNode,
  pi_flopsPerRank,
  pi_flopsPerCore,
  pi_flopsPerThread,
  pi_readbw,
  pi_writebw,
  __pi_COUNT,
  
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

static const char *print_stat(const bench_global_result_t *stats, const bench_node_result_t *nodeStats, const bench_exec_info_t *configs, size_t nConfigs, mypapi_interpretations j) {
  const char *desc = NULL;
  for (int i3 = 0; i3 < nConfigs; i3 += 1) {
    char str[80];
    str[0] = '\0';
    const bench_exec_info_t *config = &configs[i3];
    const bench_global_result_t *stat = &stats[i3];
    const bench_node_result_t *nodeStat = &nodeStats[i3];

    uint64_t lup = config->nStencilsPerCall;
    uint64_t flop = config->nFlopsPerCall;
    uint64_t nLoadedBytes = config->nLoadedBytesPerCall;
    uint64_t nStoredBytes = config->nStoredBytesPerCall;
    uint64_t nWorkingSet = config->nWorkingSet;
    
    int nRanks = benchWorldRanks();
    int nThreadsPerRank = nodeStat->nThreads;
    //assert(nThreadsPerRank == Kernel_ProcessorCount());
    int nCoresPerRank = nodeStat->nCores;
    int nThreads = nRanks * nThreadsPerRank;
    int nCores = nRanks * nCoresPerRank;
    assert(nRanks % Kernel_ProcessCount() == 0);
    int nNodes = nRanks / Kernel_ProcessCount();
    
    double avgtime = bench_statistics_avg(&stat->durationAvgAvg);
    
    double lups = lup / avgtime;
    
    double flops = (double)flop / avgtime;
    //auto localrms = stat->localrmstime / stat->avgtime;
    //auto globalrms = stat->globalrmstime / stat->avgtime;
    //auto sites = stat->sites;
    
   


#if BENCH_BGQ
    const uint64_t *native = &nodeStat->excerpt.native[0];
    uint64_t nCycles = native[PEVT_CYCLES];
    //auto nCycles = stat.cyclesAvgAvg.avg();
    //auto nCoreCycles = stats[i3].counters.corecycles;
    //auto nNodeCycles = stats[i3].counters.nodecycles;
    uint64_t nInstructions = native[PEVT_INST_ALL];
    uint64_t nStores = native[PEVT_LSU_COMMIT_STS];
    uint64_t nL1IStalls = native[PEVT_LSU_COMMIT_STS];
    uint64_t nL1IBuffEmpty = native[PEVT_IU_IBUFF_EMPTY_CYC];

    uint64_t nIS1Stalls = native[PEVT_IU_IS1_STALL_CYC];
    uint64_t nIS2Stalls = native[PEVT_IU_IS2_STALL_CYC];

    uint64_t nCachableLoads = native[PEVT_LSU_COMMIT_CACHEABLE_LDS];
    uint64_t nL1Misses = native[PEVT_LSU_COMMIT_LD_MISSES];
    uint64_t nL1Hits = nCachableLoads - nL1Misses;

    uint64_t nL1PMisses = native[PEVT_L1P_BAS_MISS];
    uint64_t nL1PHits = native[PEVT_L1P_BAS_HIT];
    uint64_t nL1PAccesses = nL1PHits + nL1PMisses;

    uint64_t nL2Misses = native[PEVT_L2_MISSES];
    uint64_t nL2Hits = native[PEVT_L2_HITS];
    uint64_t nL2Accesses = nL2Misses + nL2Hits;

    uint64_t nDcbtHits = native[PEVT_LSU_COMMIT_DCBT_HITS];
    uint64_t nDcbtMisses = native[PEVT_LSU_COMMIT_DCBT_MISSES];
    uint64_t nDcbtAccesses = nDcbtHits + nDcbtMisses;

    uint64_t nXUInstr = native[PEVT_INST_XU_ALL];
    uint64_t nAXUInstr = native[PEVT_INST_QFPU_ALL];
    uint64_t nXUAXUInstr = nXUInstr + nAXUInstr;

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
    case pi_cores:
      desc = "Cores per rank";
      snprintf(str, sizeof(str), "%d", nCoresPerRank);
      break;
    case pi_flopPerStencil:
      desc = "Flop per stencil";
      snprintf(str, sizeof(str), "%.0f", flop / (double)lup);
      break;
    case pi_ws:
      desc = "Working set";
      snprintf(str, sizeof(str), "%.1f MB", nWorkingSet / MEGA);
      break;
      
    case pi_mlups:
      desc = "Stencils per sec";
      snprintf(str, sizeof(str), "%.2f mlup/s", lup / MEGA);
      break;
      
    case pi_mflops:
      desc = "Flop per sec";
      snprintf(str, sizeof(str), "%.0f mflop/s", flops / MEGA);
      break;
    case pi_flopsPerNode:
      desc = "Flop per node and sec";
      snprintf(str, sizeof(str), "%.0f mflop/s", flops / (nNodes * MEGA));
      break;
    case pi_flopsPerRank:
      desc = "Flop per rank and sec";
      snprintf(str, sizeof(str), "%.0f mflop/s", flops / (nRanks * MEGA));
      break;
    case pi_flopsPerCore:
      desc = "Flop per core and sec";
      snprintf(str, sizeof(str), "%.0f mflop/s", flops / (nCores * MEGA));
      break;
    case pi_flopsPerThread:
      desc = "Flop per thread and sec";
      snprintf(str, sizeof(str), "%.0f mflop/s", flops / (nThreads * MEGA));
      break;
      
    case pi_readbw:
      desc = "Read bandwidth per rank";
      snprintf(str, sizeof(str), "%.0f MB/s", nLoadedBytes / (avgtime * MEGA * nRanks));
      break;
    case pi_writebw:
      desc = "Write bandwidth per rank";
      snprintf(str, sizeof(str), "%.0f MB/s", nStoredBytes / (avgtime * MEGA * nRanks));
      break;
      
    case pi_correct:
      desc = "Max error to reference";
      if (config->comparefunc && config->reffunc) {
        snprintf(str, sizeof(str), "%g", stat->error);
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
//    case pi_corecpi:
//      desc = "Cycles per instruction (Core)";
//     snprintf(str, sizeof(str), "%.3f cpi", nCoreCycles / nInstructions);
//      break;
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
//    case pi_l1plistlatestalls:
//      desc = "Stalls list prefetch behind";
//      snprintf(str, sizeof(str), "%.2f", nL1PLatePrefetchStalls / nCoreCycles);
//      break;
#endif
    default:
      desc = NULL;
    }

    printf("%" SCELLWIDTH "s|", str);
  }

  return desc;
}


static void print_stats(const bench_global_result_t *stats, const bench_node_result_t *nodeStats, const bench_exec_info_t *execs, size_t nConfigs) {
  for (int j = 0; j < __pi_COUNT; j += 1) {
    printf("%10s|", "");
    const char *desc = print_stat(stats, nodeStats, execs, nConfigs, (mypapi_interpretations)j);
    if (desc)
      printf(" %s\n", desc);
    else
      printf("\n");
  }
}


static int omp_threads[] = { 1, 2, 4, 8, 16, 32, 48, 64 };
static const char *omp_threads_desc[] = { "1", "2", "4", "8", "16", "32", "48", "64" };

typedef struct {
  int coord[6];
} bgcoord_t;


static void bench_threadinfo_worker(void *argptr, size_t tid, size_t nThreads) {
  int selfRank = benchSelfRank();

  if (isPrintingRank())  {
    printf("MK This is rank %d thread %llu (ompid=%d, procid=%d,coreid=%d,smtid=%d)\n", selfRank, tid, mypapi_threadid(), Kernel_ProcessorID(), Kernel_ProcessorCoreID(), Kernel_ProcessorThreadID());
  }
}

#if defined(__cplusplus)
extern "C" {
#endif


void bench_exec(size_t max_threads, const bench_exec_info_t *configs, size_t nConfigs) {
  int worldRanks = benchWorldRanks();
  int selfRank = benchSelfRank();
  int printing = isPrintingRank();
#ifdef OMP
  const int maxThreads = omp_get_max_threads();
#else
  const int maxThreads = 1;
#endif
  
  bgq_adhoc_call(&bench_threadinfo_worker, NULL);

#if 0
   Personality_t pers;
   KERNEL_CHECK(Kernel_GetPersonality(&pers, sizeof(pers)));
   
   bgcoord_t Coords;
   Coords.coord[0] = pers.Network_Config.Acoord;
   Coords.coord[1] = pers.Network_Config.Bcoord;
   Coords.coord[2] = pers.Network_Config.Ccoord;
   Coords.coord[3] = pers.Network_Config.Dcoord;
   Coords.coord[4] = pers.Network_Config.Ecoord;
   Coords.coord[5] = Kernel_MyTcoord();
    
   bgcoord_t Len;
   Len.coord[0] = pers.Network_Config.Anodes;
   Len.coord[1] = pers.Network_Config.Bnodes;
   Len.coord[2] = pers.Network_Config.Cnodes;
   Len.coord[3] = pers.Network_Config.Dnodes;
   Len.coord[4] = pers.Network_Config.Enodes;
   Len.coord[5] = Kernel_ProcessCount();
   
   uint64_t nPhys = Len.coord[0]*Len.coord[1]*Len.coord[2]*Len.coord[3]*Len.coord[4];
   uint64_t nPhys = Len.coord[0]*Len.coord[1]*Len.coord[2]*Len.coord[3]*Len.coord[4];
   
   Personality_t* rbuf = (Personality_t *)malloc(worldRanks*sizeof(Coords)); 
    MPI_Gather(&pers, sizeof(Coords), MPI_BYTE, rbuf, sizeof(Coords), MPI_BYTE, 0, MPI_COMM_WORLD);
    if (selfRank==0) {
      for (int i = 0; i <worldRanks; i+=1) {
        uint64_t physid = 0;
        for (int j = 0; j < 5; j+=1) {
          physid *= Len.coord[j];
          physid += rbuf[i].coord[j];
        }
        
        uint64_t virtid = 0;
        for (int j = 0; j < 6; j+=1) {
          virtid *= Len.coord[j];
          virtid += rbuf[i].coord[j];
        }
      }
    }
    free(rbuf);
#endif
  
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
  bench_node_result_t nodeExcerpt;
  for (int i2 = lengthof(omp_threads)-1; i2 >=0 ; i2 -= 1) {
    int threads = omp_threads[i2];
    if (max_threads>0 && max_threads > threads)
      continue;
    
    if (printing)
      printf("%-10s|", omp_threads_desc[i2]);

    bench_global_result_t *stats = (bench_global_result_t *)malloc(nConfigs * sizeof(bench_global_result_t));
    bench_node_result_t *rankStats = (bench_node_result_t *)malloc(nConfigs * sizeof(bench_node_result_t));
    
    for (int i3 = 0; i3 < nConfigs; i3 += 1) {
      const bench_exec_info_t *config = &configs[i3];
      
      bench_global_result_t *result = &stats[i3];
      bench_node_result_t *rankResult = &rankStats[i3];
      bench_global_result_ctor(result);
      bench_node_result_ctor(rankResult, selfRank);

      if (threads > 1 && config->ompmode==omp_single)
	    continue;
      if (threads > maxThreads)
        continue;
      
      bench_exec_config(result, rankResult, config, selfRank, threads);

      if (threads == 1 && i3 == 0) {
        excerpt = *result;
        nodeExcerpt = *rankResult;
      }

      if (printing) {
        char str[80] = { 0 };
        double avg = bench_statistics_avg(&result->durationAvgAvg);
        if (avg <= 0)
          snprintf(str, sizeof(str), "~ %s", (result->error > 0.001) ? "X" : "");
        else
          //snprintf(str, sizeof(str), "%.2f mlup/s%s", (double) result.lup / (result.avgtime * MEGA), (result.error > 0.001) ? "X" : "");
          snprintf(str, sizeof(str), "%.2f msecs", avg / MILLI);
        printf("%" SCELLWIDTH "s|", str);
        fflush(stdout);
      }
    }
    if (printing) {
      printf("\n");
      print_stats(stats, rankStats, configs, nConfigs);
      print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * nConfigs);
      printf("\n");
    }
    free(stats);
  }

  if (printing) {
    printf("Hardware counter excerpt (1 thread, first config):\n");
    mypapi_print_counters(&nodeExcerpt.excerpt);
    printf("\n");
  }
}

#if defined(__cplusplus)
} // extern "C"
#endif


#if defined(__cplusplus)

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



void cxxfunccaller(void *argptr, size_t tid, size_t nThreads) {
  auto arg = (bench_exec_info_cxx_t*)argptr;
  arg->func(tid, nThreads);
}


void cxxreffunccaller(void *argptr, size_t tid, size_t nThreads) {
  auto arg = (bench_exec_info_cxx_t*)argptr;
  arg->reffunc(tid, nThreads);
}


void cxxpreparefunccaller(void *argptr, size_t tid, size_t nThreads) {
  auto arg = (bench_exec_info_cxx_t*)argptr;
  arg->preparefunc(tid, nThreads);
}

double cxxcomparealler(void *argptr, size_t tid, size_t nThreads) {
  auto arg = (bench_exec_info_cxx_t*)argptr;
  return arg->comparefunc(tid, nThreads);
}


void bench_exec_cxx(size_t max_threads, const std::vector<bench_exec_info_cxx_t> &configs) {
  size_t nConfigs = configs.size();
  std::vector<bench_exec_info_t> cConfigs;
  cConfigs.resize(nConfigs);

  for (size_t i = 0; i < nConfigs; i += 1) {
    const auto &cxxinfo = configs[i];
    bench_exec_info_t &result = cConfigs[i];

    result.desc = cxxinfo.desc;
    result.nFlopsPerCall = cxxinfo.nFlopsPerCall;
    result.nLoadedBytesPerCall = cxxinfo.nLoadedBytesPerCall;
    result.nStencilsPerCall = cxxinfo.nStencilsPerCall;
    result.nStoredBytesPerCall = cxxinfo.nStoredBytesPerCall;
    result.nWorkingSet = cxxinfo.nWorkingSet;
    result.pprefetch = cxxinfo.pprefetch;
    result.prefetch = cxxinfo.prefetch;
    result.ompmode = cxxinfo.ompmode;

    result.data = (void*)&cxxinfo;
    result.func = cxxinfo.func ? cxxfunccaller : NULL;
    result.reffunc = cxxinfo.reffunc ? cxxreffunccaller : NULL;
    result.preparefunc = cxxinfo.preparefunc ? cxxpreparefunccaller : NULL;
    result.comparefunc = cxxinfo.comparefunc ? cxxcomparealler : NULL;
  }

  bench_exec(max_threads, cConfigs.data(), nConfigs);
}


#endif

