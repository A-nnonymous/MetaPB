#include "experiments/benchmark.hpp"
#include "helpers/helper.hpp"
#include "workloads/single_op_experiment.hpp"
#include <map>

//#define TEST_SAMPLE_POINT (PERF_SAMPLE_POINT*2)
#define TEST_SAMPLE_POINT 16

using namespace benchmarks;
using namespace MetaPB;

using Executor::execType;
using Executor::HeteroComputePool;
using Executor::TaskGraph;
using Operator::allPerfRelOPSet;
using Operator::hybridOPSet;
using Operator::OperatorManager;
using Operator::OperatorTag;
using Operator::tag2Name;
using utils::Schedule;

namespace benchmarks {
/// @brief This experiment is benchmarking deduce performance in a single OP
class singleOPDeduce : public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {
    std::ofstream dataFile(dumpPath + "deduceResult.csv");
    dataFile << "opName,dataSize_MiB,realPerfCPU,deducePerfCPU,realEnergyCPU,"
                "deduceEnergyCPU,realPerfDPU,deducePerfDPU\n";
    for (const auto &[perfTag, _] : realStats) {
      size_t dataSize = perfTag.second;
      auto opTag = perfTag.first;
      std::string opName = tag2Name.at(perfTag.first);
      dataFile << opName << "," << std::to_string(dataSize) << ","
               << std::to_string(realStats.at(perfTag).timeCost_Second) << ","
               << std::to_string(deduceStats.at(perfTag).timeCost_Second) << ","
               << std::to_string(realStats.at(perfTag).energyCost_Joule) << ","
               << std::to_string(deduceStats.at(perfTag).energyCost_Joule)
               << ",";
      if (memoryBoundOPSet.contains(opTag) ||
          computeBoundOPSet.contains(opTag)) {
        dataFile << std::to_string(realStatsDPU.at(perfTag).timeCost_Second)
                 << ","
                 << std::to_string(deduceStatsDPU.at(perfTag).timeCost_Second)
                 << "\n";
      } else {
        dataFile << "0,0\n";
      }
    }
  }

  void exec() override {
    size_t upperBound_MiB = std::stoul(myArgs["upperBound_MiB"]);
    /*
    void **memPool = (void **)malloc(3);
#pragma omp parallel for
    for (int i = 0; i < 3; i++) {
      memPool[i] = malloc(upperBound_MiB* size_t(1 << 20));
    }
    */
    ///////////// debugging ////////////
    size_t bytes = upperBound_MiB * size_t(1 << 20);
    void *src1 = (void *)malloc(bytes);
    void *src2 = (void *)malloc(bytes);
    void *dst1 = (void *)malloc(bytes);
    void *memPool[3] = {src1, src2, dst1};
    ///////////// debugging ////////////

    OperatorManager om;
    om.trainAll(upperBound_MiB, 100);

    std::vector<size_t> testSizes(TEST_SAMPLE_POINT);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(BATCH_LOWERBOUND_MB,
                                               upperBound_MiB);
    for (auto &elem : testSizes) {
      elem = dist(gen);
    }
    std::sort(testSizes.begin(), testSizes.end());
    for (const auto &opSet : allPerfRelOPSet) {
      for (const auto &opTag : opSet) {
        for (const size_t batchSize_MiB : testSizes) {
          std::cout << "testing " << batchSize_MiB << "\n";
          // This stat is already been averaged by probing function
          realStats[{opTag, batchSize_MiB}] =
              om.execCPUwithProbe(opTag, batchSize_MiB, memPool);

          deduceStats[{opTag, batchSize_MiB}] =
              om.deducePerfCPU(opTag, batchSize_MiB);
          // not CPUOnly
          if (memoryBoundOPSet.contains(opTag) ||
              computeBoundOPSet.contains(opTag)) {
            realStatsDPU[{opTag, batchSize_MiB}] =
                om.execDPUwithProbe(opTag, batchSize_MiB);

            deduceStatsDPU[{opTag, batchSize_MiB}] =
                om.deducePerfDPU(opTag, batchSize_MiB);
          }
        }
      }
    }
    free(dst1);
    free(src2);
    free(src1);
  }

private:
  std::map<std::pair<OperatorTag, size_t>, perfStats> realStats;
  std::map<std::pair<OperatorTag, size_t>, perfStats> deduceStats;
  std::map<std::pair<OperatorTag, size_t>, perfStats> realStatsDPU;
  std::map<std::pair<OperatorTag, size_t>, perfStats> deduceStatsDPU;
};
} // namespace benchmarks
