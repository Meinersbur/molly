#define __MOLLYRT
#include "bench.h"
#include "bgq_dispatch.h"

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
