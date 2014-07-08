#ifndef BENCH_H
#define BENCH_H

#include <vector>
#include <functional>
#include <cstdint>

enum prefetch_stream_mode_t {
  prefetch_default,
  prefetch_disable,
  prefetch_confirmed,
  prefetch_optimistic
};



struct bench_exec_info_t {
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

  bench_exec_info_t();
};


void bench_exec(const std::vector<bench_exec_info_t> &configs);



#if 0

#include "mypapi.h"
#include <functional>





namespace molly {
  
typedef enum {
	hm_nocom = 1 << 0,
	hm_nooverlap = 1 << 1,
	hm_nokamul = 1 << 2,
	hm_fixedoddness = 1 << 3, // obsolete

	hm_noprefetchexplicit = 1 << 4, // obsolete
	hm_prefetchlist = 1 << 5,
	hm_noprefetchstream = 1 << 6,

	hm_noweylsend = 1 << 7, // obsolete
	hm_nobody = 1 << 8,
	hm_nosurface = 1 << 9, // obsolete (->hm_nodistribute)

	hm_l1pnonstoprecord = 1 << 10,
	hm_experimental = 1 << 11, // obsolete

	hm_prefetchimplicitdisable = 1 << 12,
	hm_prefetchimplicitoptimistic = 2 << 12,
	hm_prefetchimplicitconfirmed = 3 << 12,

	hm_withcheck = 1 << 14,
	hm_nodistribute = 1 << 15,
	hm_nodatamove = 1 << 16,
	hm_nospi = 1 << 17,
	hm_floatprecision = 1 << 18,
	hm_forcefull = 1 << 19,
	hm_forceweyl = 1 << 20
} bgq_hmflags;
typedef uint64_t ucoord;
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
	//double flops;

	uint64_t nStencilsPerTest;
	uint64_t nFlopsPerStencil;
	
	double error;

	mypapi_counters counters;
	bgq_hmflags opts;
	double avgovhtime;
} benchstat;
typedef std::function<void(int /*k*/,  molly::bgq_hmflags /* flags */)> benchfunc_t;
extern "C" void exec_bench(const benchfunc_t &func, int nTests, uint64_t nStencilsPerTest, uint64_t nFlopsPerStencil);
//void __molly_add_flops(uint64_t flops);

  
} // namespace molly
#endif

#endif /* BENCH_H */
