#ifndef BENCH_H
#define BENCH_H

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t

#if defined(__cplusplus)
extern "C" {
#endif

  enum prefetch_stream_mode_t {
    prefetch_default,
    prefetch_disable,
    prefetch_confirmed,
    prefetch_optimistic
  };

  typedef void(*bench_exec_func_t)(void*, size_t, size_t);
  typedef double(*bench_exec_comparefunc_t)(void*, size_t, size_t);

  struct bench_exec_info_t {
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
  };

  void bench_exec(const bench_exec_info_t *configs, size_t nConfigs);

#if defined(__cplusplus)
}
#endif


#if defined(__cplusplus)
#include <vector>
#include <functional>

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

  bench_exec_info_cxx_t() : desc(NULL), nStencilsPerCall(0), nFlopsPerCall(0), nLoadedBytesPerCall(0), nStoredBytesPerCall(0), nWorkingSet(0), prefetch(prefetch_default) , pprefetch(false) {}

  //bench_exec_info_cxx_t(const bench_exec_info_t &cConfig);
};

void bench_exec_cxx(const std::vector<bench_exec_info_cxx_t> &configs);

template<size_t N>
static inline void bench_exec_cxx(const bench_exec_info_cxx_t (&configs)[N]) {
  std::vector<bench_exec_info_cxx_t> vec(configs, configs+N);
  bench_exec(vec);
}

#endif


#endif /* BENCH_H */
