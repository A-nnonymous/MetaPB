#include "experiments/benchmark.hpp"
#include "helpers/helper.hpp"
#include "workloads/single_op_experiment.hpp"
#include <map>

#define TEST_SAMPLE_POINT (PERF_SAMPLE_POINT*2)

using namespace benchmarks;
using namespace MetaPB;

using Executor::execType;
using Executor::HeteroComputePool;
using Executor::TaskGraph;
using Operator::hybridOPSet;
using Operator::OperatorManager;
using Operator::OperatorTag;
using Operator::tag2Name;
using utils::Schedule;

namespace benchmarks {
/// @brief This experiment is benchmarking deduce performance in a single OP
class singleOPDeduce: public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {
    std::ofstream dataFile(dumpPath + "deduceResult.csv");
    dataFile << "dataSize_MiB,real_perf,deduce_perf\n";
    for(const auto&[dataSize, _] : realStats){
      dataFile << std::to_string(dataSize) << "," 
               << std::to_string(realStats.at(dataSize).timeCost_Second) << ","
               << std::to_string(deduceStats.at(dataSize).timeCost_Second) << "\n";
    }
  }

  void exec() override {
    int warmup = 2, rep = 5;
    size_t upperBound_MiB = std::stoul(myArgs["upperBound_MiB"]);
    void **memPool = (void **)malloc(3);

#pragma omp parallel for
    for (int i = 0; i < 3; i++) {
      memPool[i] = malloc(upperBound_MiB* size_t(1 << 20));
    }

    OperatorManager om;
    om.trainModel({{OperatorTag::MAC, upperBound_MiB}});

    size_t step_MiB =
        (upperBound_MiB - BATCH_LOWERBOUND_MB) / TEST_SAMPLE_POINT;

    for (size_t batchSize_MiB = BATCH_LOWERBOUND_MB; batchSize_MiB <= upperBound_MiB; batchSize_MiB += step_MiB) {
        std::cout << "testing "<< batchSize_MiB << "\n";
        // This stat is already been averaged by probing function
        realStats[batchSize_MiB]=  om.execCPUwithProbe(OperatorTag::MAC, batchSize_MiB,memPool);

        deduceStats[batchSize_MiB] = om.deducePerfCPU(OperatorTag::MAC, batchSize_MiB);
    }
  }

private:
  std::map<size_t, perfStats> realStats;
  std::map<size_t, perfStats> deduceStats;
};
} // namespace benchmarks
