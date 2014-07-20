
#include <bench.h>
#include <mpi.h>
#include <cstdlib> // EXIT_SUCCESS

void benchme(size_t tid, size_t nThreads) {
  auto start = MPI_Wtime();
  while (true) {
    auto now = MPI_Wtime();
    auto duration = now - start;
    if (duration >= 1)
      break;
  }
}

void donothing(size_t tid, size_t nThreads) {
}


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
    bench_exec_info_cxx_t benchinfo;
    benchinfo.desc = "nothing";
    benchinfo.func = donothing;
    configs.push_back(benchinfo);
  }

  {
    bench_exec_info_cxx_t benchinfo;
    benchinfo.desc = "benchme";
    benchinfo.func = benchme;
    configs.push_back(benchinfo);
  }

  MPI_Init(&argc, &argv);
  bench_exec_cxx(configs);
  MPI_Finalize();

  return EXIT_SUCCESS;
}
