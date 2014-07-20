
#include <bench.h>
#include <mpi.h>
#include <cstdlib> // EXIT_SUCCESS


// Only to satisfy MollyRT
extern "C" void __molly_generated_init() {
}
extern "C" int __molly_orig_main(int argc, char *argv[]) {
  return EXIT_SUCCESS;
}
extern "C" void __molly_generated_release() {
}


int main(int argc, char *argv[]) {
  std::vector<bench_exec_info_cxx_t> configs;

  {
    configs.emplace_back();
    auto &benchinfo = configs.back();
    benchinfo.desc = "1 sec";
    benchinfo.func = [](size_t tid, size_t nThreads) {
  auto start = MPI_Wtime();
  while (true) {
    auto now = MPI_Wtime();
    auto duration = now - start;
    if (duration >= 1)
      break;
  }
    };
    benchinfo.nStencilsPerCall = 0;
    benchinfo.nFlopsPerCall =  0;
    benchinfo.nStoredBytesPerCall = 0;
    benchinfo.nLoadedBytesPerCall = 0;
    benchinfo.nWorkingSet = 0;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
  }

    {
    configs.emplace_back();
    auto &benchinfo = configs.back();
    benchinfo.desc = "donothing";
    benchinfo.func = [](size_t tid, size_t nThreads) {
    };
    benchinfo.nStencilsPerCall = 0;
    benchinfo.nFlopsPerCall =  0;
    benchinfo.nStoredBytesPerCall = 0;
    benchinfo.nLoadedBytesPerCall = 0;
    benchinfo.nWorkingSet = 0;
    benchinfo.prefetch = prefetch_confirmed;
    benchinfo.pprefetch = false;
    benchinfo.ompmode = omp_single;
  }
  
  MPI_Init(&argc, &argv);
  bench_exec_cxx(1, configs);
  MPI_Finalize();

  return EXIT_SUCCESS;
}
