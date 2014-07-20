#include "bgq_dispatch.h"

#if defined(__cplusplus)
#include <cstdlib> // abort()
#else
#include <stdlib.h> // abort()
#endif

bool g_bgq_dispatch_inparallel =false;
int g_bgq_dispatch_threads;
bool g_bgq_dispatch_l1p = false;

#ifndef NDEBUG
#include <mpi.h>
#include <stdio.h>
#define master_print(...)              \
  do { \
	  int myrank; \
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank); \
  if (myrank == 0)  \
		if (omp_get_num_threads() == 0)  \
			fprintf(stderr, __VA_ARGS__); \
	} while (0)
#define root_print(...)             \
  do { \
	  int myrank; \
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank); \
  if (myrank == 0)  \
			fprintf(stderr, __VA_ARGS__); \
	} while (0)
#else
#define master_print(...)
#define root_print(...)
#endif

typedef struct {
	bgq_mainlike_func func;
	int argc;
	char **argv;
} bgq_dispatch_mainlike_args;

int bgq_dispatch_callMainlike(void *args_untyped) {
	bgq_dispatch_mainlike_args *args = (bgq_dispatch_mainlike_args *)args_untyped;
	bgq_mainlike_func func = args->func;

	return (*func)(args->argc, args->argv);
}

int bgq_parallel_mainlike(bgq_mainlike_func func, int argc, char *argv[]) {
	bgq_dispatch_mainlike_args args = { func, argc, argv };
	return bgq_parallel(&bgq_dispatch_callMainlike, &args);
}



#ifndef OMP

// Dummy implementation using just one thread
//TODO: Impl using pthreads?

int bgq_parallel(bgq_master_func master_func, void *master_arg) {
	g_bgq_dispatch_threads = 1;
	g_bgq_dispatch_inparallel = true;

	int master_result = master_func(master_arg);
	g_bgq_dispatch_inparallel = false;
	g_bgq_dispatch_threads = 0;
	return master_result;
}

void bgq_master_call(bgq_worker_func worker_func, void *arg) {
	(*worker_func)(arg, 0, 1);
}

void bgq_master_sync() {
	// Just 1 thread, no sync required
}

void bgq_master_memzero(void *ptr, size_t size) {
	memset(ptr, 0, size);
}

void bgq_master_memcpy(void *ptrDst, void *ptrSrc, size_t size) {
	memcpy(ptrDst, ptrSrc, size);
}


void bgq_set_threadcount(size_t threads) {
  if (threads != 1) {
    printf("Multiple threads in non-OpenMP build\n"); 
    abort();
  }
}

void bgq_adhoc_call(bgq_worker_func func, void *arg) {  
    g_bgq_dispatch_threads = 1;
    g_bgq_dispatch_inparallel = true;
    
    (*func)(arg, 0, 1);
    
    g_bgq_dispatch_inparallel = false;
    g_bgq_dispatch_threads = 0;
}

#else


#ifdef BGQ
#include <l2/barrier.h>
//#include <wu/wait.h>
#include <upci/upc_atomic.h>
//#include <hwi/include/bqc/A2_inlines.h>
//#include <time.h> // nanosleep() system call
#endif

#include <omp.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifdef XLC
#include <l1p/pprefetch.h>
#endif

#ifdef OMP
#include <omp.h>
#endif

//static L2_Barrier_t barrier;

#ifndef NDEBUG
static char space[64*25+1]= {' '};
#endif

static volatile bgq_worker_func g_bgq_dispatch_func;
static void * volatile g_bgq_dispatch_arg;
static volatile bool g_bgq_dispatch_terminate;
//static volatile bool g_bgq_dispatch_sync; // obsolete
#ifndef NDEBUG
static volatile size_t g_bgq_dispatch_seq;
#endif

static bool g_bgq_dispatch_pendingsync; // Accessed by master only; control signal for master_func in case it calls bgq_master_sync() more than once. In this case any call except the first does nothing



typedef struct {
	bgq_worker_func func;
	void *funcargs;
} bgq_master_adhoc_parms;

static int bgq_master_adhoc(void *arg_untyped) {
	bgq_master_adhoc_parms *args = arg_untyped;
	bgq_worker_func func = args->func;
	void *funcargs = args->funcargs;

	bgq_master_call(func, funcargs);
	return 0;
}

void bgq_adhoc_call(bgq_worker_func func, void *arg) {  
  assert(func);
  bgq_master_adhoc_parms work = { func, arg };
  bgq_parallel(&bgq_master_adhoc, &work);
}


#ifdef BGQ
static bool g_bgq_dispatch_barrier_initialized = false;
static L2_Barrier_t g_bgq_dispatch_barrier = L2_BARRIER_INITIALIZER;
#endif

static inline void bgq_thread_barrier() {
#ifdef BGQ
	uint64_t savpri = Set_ThreadPriority_Low(); // Lower thread priority, so if busy waiting is used, do not impact other threads on core; TODO: May also use wakeup machinsm
	L2_Barrier(&g_bgq_dispatch_barrier, g_bgq_dispatch_threads);
	Restore_ThreadPriority(savpri);
#else
#pragma omp barrier
#endif
}


void bgq_set_threadcount(size_t threads) {
  assert(!g_bgq_dispatch_inparallel);
  omp_set_num_threads(threads);
}




//static size_t count = 0;
//#pragma omp threadvar(count)

static void bgq_worker() {
	//assert(omp_in_parallel() && "Call this inside #pragma omp parallel");
	assert(g_bgq_dispatch_threads == omp_get_num_threads());
	size_t threads = g_bgq_dispatch_threads;
	size_t tid = omp_get_thread_num(); // Or Kernel_ProcessorID()


	//assert((tid != 0) && "This function is for non-master threads only");
	//size_t count = 0;
	while (true) {
		// Wait until every thread did its work
		// This doesn't need to be a barrier, waiting for submission of some work from the master is ok too
		// TODO: Hope OpenMP has a good implementation without busy waiting; if not, do some own work

#ifndef BGQ
		if (tid!=0) {
			// Guarantee that work is finished
			//TODO: can we implement this without this second barrier?
			// Required to ensure consistency of g_bgq_dispatch_sync, g_bgq_dispatch_terminate, g_bgq_dispatch_func
			bgq_thread_barrier(); // the sync barrier
		}
#endif


		// Master thread may write shared variables before this barrier
		bgq_thread_barrier();
#ifndef BGQ
		mbar();
#else
#pragma omp flush
#endif
		// Worker threads read common variables after this barrier
		if (tid==0) {
			assert(!g_bgq_dispatch_pendingsync);
			g_bgq_dispatch_pendingsync = true;
		}

		//count += 1;
		// All threads should be here at the same time, including the master thread, which has issued some work, namely, calling a function

		//if (g_bgq_dispatch_sync) {
			// This was meant just for synchronization between the threads, which already has been done
			//printf("%*sSYNC: tid=%u seq=%u\n", (int)tid*20, "", (int)tid, (int)g_bgq_dispatch_seq);
		//} else 
		  if (g_bgq_dispatch_terminate) {
			// Exit program, or at least, the parallel section
			//printf("%*sTERM: tid=%u seq=%u\n", (int)tid*20, "",(int)tid, (int)g_bgq_dispatch_seq);
			return;
		} else {
			//root_print("%*sCALL: tid=%u seq=%u func=%p\n", (int)tid*20, "",(int)tid, (int)g_bgq_dispatch_seq, g_bgq_dispatch_func);
			assert(g_bgq_dispatch_func);
			void *arg = g_bgq_dispatch_arg;
#ifdef BGQ
			if (g_bgq_dispatch_l1p) {
				L1P_PatternResume();
				g_bgq_dispatch_func(arg, tid, threads);
				L1P_PatternPause();
			} else {
#endif
				g_bgq_dispatch_func(arg, tid, threads); //TODO: Shuffle tid to loadbalance work?
#ifdef BGQ
			}
#endif
		}

		if (tid==0) {
			// Let master thread continue the program
			// Hint: master thread must call bgq_thread_barrier() sometime to release the workers from the following barrier
			return;
		}
		// All others, wait for the next command
	}
}




int bgq_parallel(bgq_master_func master_func, void *master_arg) {
#ifndef NDEBUG
	for (int i = 0; i < 64*25; i+=1)
		space[64*25] = ' ';
	space[64*25] = '\0';
#endif
	assert(!g_bgq_dispatch_inparallel);
	assert(!omp_in_parallel() && "This starts the parallel section, do not call it within one");
	g_bgq_dispatch_func = NULL;
	g_bgq_dispatch_arg = NULL;
	g_bgq_dispatch_terminate = false;
	//g_bgq_dispatch_sync = false;
#ifndef NDEBUG
	g_bgq_dispatch_seq = 0;
#endif
#ifdef BGQ
	if (!g_bgq_dispatch_barrier_initialized) {
		Kernel_L2AtomicsAllocate(&g_bgq_dispatch_barrier, sizeof(g_bgq_dispatch_barrier));
		g_bgq_dispatch_barrier_initialized = true;
	}
#endif
	g_bgq_dispatch_threads = omp_get_max_threads();
#ifdef OMP
	//omp_num_threads = 1/*omp_get_num_threads()*/; // For legacy linalg (it depends on whether nested parallelism is enabled)
#endif
	g_bgq_dispatch_inparallel = true;

	int master_result = 0;
	// We use OpenMP only to start the threads
	// Overhead of using OpenMP is too large
#pragma omp parallel
	{   //*((char*)NULL)=0;
		size_t tid = omp_get_thread_num(); // Or Kernel_ProcessorID()
#ifndef NDEBUG
		if (tid == 0) {
			int threads = omp_get_num_threads();
			//root_print("Entered parallel control with %d threads, tid=%d=%d\n", threads, Kernel_ProcessorID(),tid);
		}
#endif

		// Start workers
		if (tid != 0) {
			bgq_worker();
		}

		// Start control program in master
		if (tid == 0) {
			g_bgq_dispatch_pendingsync = true;
			
			master_result = master_func(master_arg);

			bgq_master_sync();
			// After program finishes, set flag so worker threads can terminate
			g_bgq_dispatch_func = NULL;
			g_bgq_dispatch_arg = NULL;
			g_bgq_dispatch_terminate = true;
			//g_bgq_dispatch_sync = false;
#ifdef BGQ
			mbar();
#else
#pragma omp flush
#endif

			// Wakeup workers to terminate
			bgq_worker();
		}

		//printf("%*sEXIT: tid=%u\n", (int)tid*25, "",(int)tid);
	}
	g_bgq_dispatch_inparallel = false;
	g_bgq_dispatch_threads = 0;
#ifdef OMP
	//omp_num_threads = omp_get_max_threads();
#endif
#ifndef NDEBUG
	//root_print("EXIT bgq_parallel ##############################################################\n");
#endif
	return master_result;
}



void bgq_master_call(bgq_worker_func func, void *arg) {
	assert(omp_get_thread_num()==0);
	assert(func);

	if (g_bgq_dispatch_inparallel) {
		bgq_master_sync();

#ifndef NDEBUG
		g_bgq_dispatch_seq += 1;
		//root_print("MASTER CALL seq=%d func=%p,--------------------------------------------------------\n", (int)g_bgq_dispatch_seq, func);
#endif
		g_bgq_dispatch_func = func;
		g_bgq_dispatch_arg = arg;
		g_bgq_dispatch_terminate = false;
		//g_bgq_dispatch_sync = false;
#ifdef BGQ
		mbar();
#else
#pragma omp flush
#endif
		// Join the worker force
		assert(g_bgq_dispatch_func);
		bgq_worker();
	} else {
		// Not in a parallel section. Two possibilities:
		// 1. Execute sequentially
		// 2. Start and end a parallel section just for calling this func

		if (omp_in_parallel()) {
			// We are in another OpenMP construct, therefore call sequentially
			(*func)(arg, 0, 1);
		} else {
		    bgq_adhoc_call(func, arg);
		}
	}
}


void bgq_master_sync() {
	assert(omp_get_thread_num()==0 && "Call only by the master");

	if (g_bgq_dispatch_inparallel) {
		if (g_bgq_dispatch_pendingsync) {
#ifndef BGQ
			bgq_thread_barrier();
#else
			// One-barrier solution with atomic L2 operations
			uint64_t savpri = Set_ThreadPriority_Low(); // Lower thread priority, so if busy waiting is used, do not impact other threads on core
			uint64_t start = g_bgq_dispatch_barrier.start;
			uint64_t target = start + g_bgq_dispatch_threads - 1; // Number of workers
			// Wait until all the workers reached the barrier
			// Note the value of start cannot change here because the master threads also needs to join the barrier before start is increased
			while (g_bgq_dispatch_barrier.count < target) {
				// Busy waiting loop
			}
			// Here we know that all workers are busy waiting for the barrier
			// Hence, we can safely write globals without race conditions when workers pick up the overwritten values
			Restore_ThreadPriority(savpri);
#endif
#ifndef NDEBUG
			//fflush(stderr);
			//root_print("MASTER SYNC seq=%d--------------------------------------------------------\n", (int)g_bgq_dispatch_seq);
			//fflush(stderr);
#endif
			g_bgq_dispatch_pendingsync = false; // Every call from here on is going to be ignored
			return;
		} else {
			// Threads already at sync barrier
			return;
		}
	} else {
		// No sync necessary
	}
}


typedef struct {
	void *ptr;
	size_t size;
} bgq_memzero_work;

#define BGQ_ALIGNMENT_L1 64

static inline size_t min_sizet(size_t a, size_t b) {
  return (a <= b) ? a : b;
}

void bgq_memzero_worker(void *args_untyped, size_t tid, size_t threads) {
	bgq_memzero_work *args = args_untyped;
	char *ptr = args->ptr;
	size_t size = args->size;

	const size_t workload = size;
	const size_t threadload = (workload+threads-1)/threads;
	const size_t begin = tid*threadload;
	const size_t end = min_sizet(workload, begin+threadload);

	char *beginLine = (ptr + begin);
	beginLine = (void*)((uintptr_t)beginLine & ~(BGQ_ALIGNMENT_L1-1));

	char *endLine  = (ptr + end);
	endLine = (void*)((uintptr_t)endLine & ~(BGQ_ALIGNMENT_L1-1));

	// Special cases
	//if (tid == 0)
	//	beginLine = ptr; /* rule redundant */
	if (tid == g_bgq_dispatch_threads-1)
		endLine = (ptr + size);

	if (beginLine < ptr)
		beginLine = ptr;
	if (beginLine >= endLine)
		return;

	assert(beginLine >= ptr);
	assert(endLine <= ptr + size);
	assert(endLine >= beginLine);

	size_t threadsize = (endLine-beginLine);
	memset(beginLine, 0x00, threadsize);
}

void bgq_master_memzero(void *ptr, size_t size) {
	if (size <= (1<<14)/*By empirical testing*/) {
		memset(ptr, 0x00, size);
	} else {
		bgq_master_sync();
		static bgq_memzero_work work;
		work.ptr = ptr;
		work.size = size;
		bgq_master_call(&bgq_memzero_worker, &work);
	}
}


typedef struct {
	void *ptrDst;
	void *ptrSrc;
	size_t size;
} bgq_memcopy_work;

void bgq_memcpy_worker(void *args_untyped, size_t tid, size_t threads) {
	bgq_memcopy_work *args = args_untyped;
	char *ptrDst = args->ptrDst;
	char *ptrSrc = args->ptrSrc;
	size_t size = args->size;

	const size_t workload = size;
	const size_t threadload = (workload+threads-1)/threads;
	const size_t begin = tid*threadload;
	const size_t end = min_sizet(workload, begin+threadload);

	if (begin<end) {
		size_t count = end-begin;
		memcpy(ptrDst+begin, ptrSrc+begin, count);
	}
}

void bgq_master_memcpy(void *ptrDst, void *ptrSrc, size_t size) {
	if (size <= (1<<14)) {
		memcpy(ptrDst, ptrSrc, size);
	} else {
		bgq_master_sync();
		static bgq_memcopy_work work;
		work.ptrDst = ptrDst;
		work.ptrSrc = ptrSrc;
		work.size = size;
		bgq_master_call(&bgq_memcpy_worker, &work);
	}
}



#endif
