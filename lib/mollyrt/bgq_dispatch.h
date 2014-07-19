/*
 * bgq_dispatch.h
 *
 *  Created on: Oct 15, 2012
 *      Author: meinersbur
 */

#ifndef BGQ_DISPATCH_H_
#define BGQ_DISPATCH_H_

#include <string.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*bgq_worker_func)(void *arg, size_t tid, size_t threads);
typedef int (*bgq_master_func)(void *arg);
typedef int (*bgq_mainlike_func)(int argc, char *argv[]);

extern bool g_bgq_dispatch_inparallel;
extern int g_bgq_dispatch_threads;
extern bool g_bgq_dispatch_l1p;
void bgq_set_threadcount(size_t threads);
int bgq_parallel(bgq_master_func master_func, void *master_arg);
int bgq_parallel_mainlike(bgq_mainlike_func func, int argc, char *argv[]);
void bgq_adhoc_call(bgq_worker_func func, void *arg);  // Create temporary bgq_parallel environment to call the worker functions in it

void bgq_master_call(bgq_worker_func worker_func, void *arg);
void bgq_master_sync();

void bgq_master_memzero(void *ptr, size_t size);
void bgq_master_memcpy(void *ptrDst, void *ptrSrc, size_t size);

#if defined(__cplusplus)
} // extern "C" 
#endif

#endif /* BGQ_DISPATCH_H_ */
