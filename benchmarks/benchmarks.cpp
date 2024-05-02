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
  bm.addBenchmark<singleOPDeduce>({"regression"},
                                  {
                                    {"upperBound_MiB", "4096"}
                                  });
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
                                  */
  /*
  bm.addBenchmark<Graph_benchmark>(
      {"All_Graphloads"},
      {
        {"loadSize_MiB", "4096"}
      });
      */
  bm.addBenchmark<DetailShowOff>({"string_workload"},
                                 {
                                     {"loadSize_MiB", "4096"},
                                     {"opNum", "6"},
                                 });
  
  bm.exec();
  return 0;
}
