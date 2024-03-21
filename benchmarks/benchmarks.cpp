#include "experiments/benchmarkManager.hpp"
using namespace benchmarks;

int main() {
  BenchmarkManager bm;
  // ------------ Hybridize OP benchmarks --------------
  bm.addBenchmark<HybridOP_Benchmark>(
    {"singleOP"},
    {
      {"loadSize_MiB", "4096"},
      {"isConsideringReduce", "false"},
    }
  );
  bm.addBenchmark<HybridOP_Benchmark>(
    {"singleOP"},
    {
      {"loadSize_MiB", "4096"},
      {"isConsideringReduce", "true"},
    }
  );
  bm.exec();
  return 0;
}