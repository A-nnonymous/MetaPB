#include "experiments/benchmark.hpp"
#include "helpers/helper.hpp"
#include "workloads/single_op_experiment.hpp"

#include <map>
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
class HybridOP_Benchmark : public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {
    std::ofstream file(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                       (myArgs.at("isConsideringReduce") == "true"
                            ? "w_reduce_"
                            : "wo_reduce_") +
                       "perf.csv");
    file << "OperatorName,offloadRatio,timeConsume_Seconds,energyConsume_"
            "Joules,dataTransfer_MiB\n";
    for (const auto &[perfPair, stats] : result) {
      file << tag2Name.at(perfPair.first) << ","
           << std::to_string(perfPair.second) << ","
           << std::to_string(stats.timeCost_Second) << ","
           << std::to_string(stats.energyCost_Joule) << ","
           << std::to_string(stats.dataMovement_MiB) << "\n";
    }
  }
  void exec() override {
    size_t loadSize_MiB = std::stoul(myArgs["loadSize_MiB"]);
    void **memPool = (void **)malloc(3);

#pragma omp parallel for
    for (int i = 0; i < 3; i++) {
      memPool[i] = malloc(loadSize_MiB * size_t(1 << 20));
    }
    OperatorManager om;
    om.instantiateAll();
    HeteroComputePool hcp(om, memPool);
    bool isConsideringReduce =
        myArgs["isConsideringReduce"] == "true" ? true : false;

    for (const auto &opSet : hybridOPSet) {
      for (const auto &opTag : opSet) {
        /*
        auto opTag = OperatorTag::MAC;
        Schedule sched{false, {0,1,2}, {0.5,0.5, 0.0f}};
        TaskGraph tg = genSingleOp_w_reduce(opTag, loadSize_MiB);
        hcp.execWorkload(tg,sched,execType::DO);
        */
        std::cout << "Hybridizing Operator " << tag2Name.at(opTag) << std::endl;
        for (int i = 0; i <= 10; i++) {
          float offloadRatio = i / 10.0f;
          if (isConsideringReduce) {
            Schedule sched{false, {0,1,2}, {offloadRatio,offloadRatio,0.0f}};
            TaskGraph tg = genSingleOp_w_reduce(opTag, loadSize_MiB);
            tg.printGraph("./");
            perfStats stat{0, 0, 0};
            for (int i = 0; i < WARMUP_REP + REP; i++) {
              perfStats thisStat = hcp.execWorkload(tg, sched, execType::DO);
              if (i >= WARMUP_REP) {
                stat.timeCost_Second += thisStat.timeCost_Second / REP;
                stat.energyCost_Joule += thisStat.energyCost_Joule / REP;
                stat.dataMovement_MiB += thisStat.dataMovement_MiB / REP;
              }
            }
            result[{opTag, offloadRatio}] = stat;
          } else {
            Schedule sched{false, {0,1,2}, {offloadRatio,offloadRatio, offloadRatio}};
            TaskGraph tg = genSingleOp_wo_reduce(opTag, loadSize_MiB);
            tg.printGraph("./");
            perfStats stat{0, 0, 0};
            for (int i = 0; i < WARMUP_REP + REP; i++) {
              perfStats thisStat = hcp.execWorkload(tg, sched, execType::DO);
              if (i >= WARMUP_REP) {
                stat.timeCost_Second += thisStat.timeCost_Second / REP;
                stat.energyCost_Joule += thisStat.energyCost_Joule / REP;
                stat.dataMovement_MiB += thisStat.dataMovement_MiB / REP;
              }
            }
            result[{opTag, offloadRatio}] = stat;
          }
        }
      }
    }
    for (int i = 0; i < 3; i++) {
      free(memPool[i]);
    }
    free((void *)memPool);
  }

private:
  std::map<std::pair<OperatorTag, float>, perfStats> result;
};
} // namespace benchmarks
