#include "experiments/benchmarkManager.hpp"
using namespace benchmarks;

int main() {
  BenchmarkManager bm;
  // ------------ Hybridize OP benchmarks --------------

  /*
  bm.addBenchmark<DetailShowOff>({"string_workload"},
                                 {
                                     {"loadSize_MiB", "2048"},
                                     {"opNum", "6"},
                                 });
  bm.addBenchmark<DetailShowOff>({"string_workload"},
                                 {
                                     {"loadSize_MiB", "4096"},
                                     {"opNum", "6"},
                                 });
                                 */
  /*

  bm.addBenchmark<HybridOP_Benchmark>(
    {"singleOP"},
    {
      {"loadSize_MiB", "4096"},
      {"isConsideringReduce", "true"},
    }
  );
  bm.addBenchmark<HybridOP_Benchmark>(
    {"singleOP"},
    {
      {"loadSize_MiB", "4096"},
      {"isConsideringReduce", "false"},
    }
  );
  bm.addBenchmark<singleOPDeduce>({"regression"},
                                  {
                                    {"upperBound_MiB", "1028"}
                                  });
  bm.addBenchmark<Graph_benchmark>({"All_Graphloads"},
                                   {{"loadSize_MiB", "4096"}});
  bm.addBenchmark<Graph_benchmark>({"All_Graphloads"},
                                   {{"loadSize_MiB", "8192"}});
  */
  bm.addBenchmark<Graph_benchmark>({"All_Graphloads"},
                                   {{"loadSize_MiB", "1028"}});
  bm.exec();
  return 0;
}
