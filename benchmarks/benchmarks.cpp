#include "experiments/benchmarkManager.hpp"
using namespace benchmarks;

int main() {
  BenchmarkManager bm;
  // ------------ Hybridize OP benchmarks --------------

  /*
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
  bm.addBenchmark<Graph_benchmark>(
      {"All_Graphloads"},
      {
        {"loadSize_MiB", "4096"}
      });
  */
  bm.addBenchmark<DetailShowOff>({"string_workload"},
                                 {
                                     {"loadSize_MiB", "4096"},
                                     {"opNum", "10"},
                                 });
  bm.exec();
  return 0;
}
