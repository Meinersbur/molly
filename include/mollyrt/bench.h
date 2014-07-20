#ifndef BENCH_H
#define BENCH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif



#if defined(__cplusplus)
#include <cstdint> // uint64_t
#include <cstddef> // size_t
#include <cstdbool>

#include <vector>
#include <functional>
#else
#include <stdint.h> // uint64_t
#include <stddef.h> // size_t
#include <stdbool.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

  typedef enum {
    prefetch_default,
    prefetch_disable,
    prefetch_confirmed,
    prefetch_optimistic
  } prefetch_stream_mode_t;
  
  typedef enum {
    omp_single,   // No multithreading, tid=0, nThreads=1
    omp_plain,    // Client creates and releases threads itself (pthread_create or #pragma omp parallel [for]); called with tid=0
    omp_parallel, // Client expects #pragma omp parallel environment, and uses #pragma omp for; called with tid=0
    omp_dispatch  // Client is called on every thread
  } omp_mode_t;
  
  typedef void(*bench_exec_func_t)(void*, size_t, size_t);
  typedef double(*bench_exec_comparefunc_t)(void*, size_t, size_t);

  typedef struct {
    const char *desc;

    void *data;
    bench_exec_func_t preparefunc;
    bench_exec_func_t func;
    bench_exec_func_t reffunc;
    bench_exec_comparefunc_t comparefunc;
    uint64_t nStencilsPerCall;
    uint64_t nFlopsPerCall;
    uint64_t nLoadedBytesPerCall;
    uint64_t nStoredBytesPerCall;
    uint64_t nWorkingSet;

    prefetch_stream_mode_t prefetch;
    bool pprefetch;
    omp_mode_t ompmode;
  } bench_exec_info_t;

  void bench_exec(size_t max_threads, const bench_exec_info_t *configs, size_t nConfigs);

#if defined(__cplusplus)
}
#endif


#if defined(__cplusplus)
struct bench_exec_info_cxx_t {
  const char *desc;
  std::function<void(size_t/*thread idx*/, size_t /*tot threads*/)> preparefunc;
  std::function<void(size_t/*thread idx*/, size_t /*tot threads*/)> func;
  std::function<void(size_t/*thread idx*/, size_t /*tot threads*/)> reffunc;
  std::function<double(size_t/*thread idx*/, size_t /*tot threads*/)> comparefunc;
  uint64_t nStencilsPerCall;
  uint64_t nFlopsPerCall;
  uint64_t nLoadedBytesPerCall;
  uint64_t nStoredBytesPerCall;
  uint64_t nWorkingSet;

  prefetch_stream_mode_t prefetch;
  bool pprefetch;
  omp_mode_t ompmode;

  bench_exec_info_cxx_t() : desc(NULL), nStencilsPerCall(0), nFlopsPerCall(0), nLoadedBytesPerCall(0), nStoredBytesPerCall(0), nWorkingSet(0), prefetch(prefetch_default) , pprefetch(false), ompmode(omp_single) {}
  //bench_exec_info_cxx_t(bench_exec_info_cxx_t &&obj) {} 
  //const bench_exec_info_cxx_t &operator=(bench_exec_info_cxx_t &&obj) {}
};

void bench_exec_cxx(size_t max_threads, const std::vector<bench_exec_info_cxx_t> &configs);

template<size_t N>
static inline void bench_exec_cxx(size_t max_threads, const bench_exec_info_cxx_t (&configs)[N]) {
  std::vector<bench_exec_info_cxx_t> vec(configs, configs+N);
  bench_exec_cxx(max_threads, vec);
}
#endif // __cplusplus


#endif /* BENCH_H */
