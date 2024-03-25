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
using Scheduler::HEFTScheduler;
using Scheduler::MetaScheduler;
using Scheduler::CPUOnlyScheduler;
using Scheduler::DPUOnlyScheduler;
using Scheduler::GreedyScheduler;

namespace benchmarks {
class Graph_benchmark : public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {
    std::ofstream file(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                       +"wholesome_perf.csv");
    file << "workload_name,scheduler_name,timeConsume_Seconds,energyConsume_"
            "Joules,dataTransfer_MiB\n";
    for (const auto &[expTag, stats] : testResults) {
      const auto & [loadName, schedName] = expTag;
      file << loadName << "," <<
          schedName<< "," << std::to_string(stats.timeCost_Second) << ","
                       << std::to_string(stats.energyCost_Joule) << ","
                       << std::to_string(stats.dataMovement_MiB) << "\n";
    }
  }
  void exec() override {
    size_t loadSize_MiB = std::stoul(myArgs["loadSize_MiB"]);
    for(int i = 20; i <= 80; i+=30){
      graphLoads[("H-" + std::to_string(i))] = randomizeHO(i/100.0f, loadSize_MiB);
      graphLoads[("M-" + std::to_string(i))] = randomizeMO(i/100.0f, loadSize_MiB);
    }
    for(int i = 4; i <=8; i*=2){
      graphLoads["FFT-" + std::to_string(i)] = genFFT(i, loadSize_MiB);
      graphLoads["GEA-" + std::to_string(i)] = genGEA(i, loadSize_MiB);
    }

    void **memPool = (void **)malloc(3);
    memPool[0] = malloc(72 * size_t(1<<30));
    memPool[1] = memPool[0] + 1 * size_t(1<<30);
    memPool[2] = memPool[1] + 1 * size_t(1<<30);
    OperatorManager om;
    for(auto& [_, tg]: graphLoads){
      om.trainModel(tg.genRegressionTask());
    }
    HeteroComputePool hcp(200, om, memPool);
    for(const auto& [loadName, loadGraph] : graphLoads){
      HEFTScheduler    heft(loadGraph, om);
      MetaScheduler    metaPF(0.6f, 0.4f, 100, loadGraph, om);
      MetaScheduler    metaHY(0.5f, 0.5f, 100, loadGraph, om);
      MetaScheduler    metaEF(0.4f, 0.6f, 100, loadGraph, om);
      GreedyScheduler  greedy;
      CPUOnlyScheduler cpuOnly;
      DPUOnlyScheduler dpuOnly;

      std::map<std::string, Schedule> scheduleResult = {
        { "HEFT", heft.schedule()},
        { "MetaPB_PerfFirst", metaPF.schedule()},
        { "MetaPB_Hybrid", metaHY.schedule()},
        { "MetaPB_EnergyFirst",metaEF.schedule() },
        { "Greedy", greedy.schedule(loadGraph, om)},
        { "CPUOnly", cpuOnly.schedule(loadGraph, om)},
        { "DPUOnly", dpuOnly.schedule(loadGraph, om)}
      };

      for(const auto& [schedName, sched] : scheduleResult){
        std::cout << 
          "executing "<< schedName << 
          "'s schedule on "<< loadName << "\n"; 
        perfStats stat;
        for (int i = 0; i < WARMUP_REP + REP; i++) {
          perfStats thisStat = hcp.execWorkload(loadGraph, sched, execType::DO);
          if (i >= WARMUP_REP) {
            stat.timeCost_Second += thisStat.timeCost_Second / REP;
            stat.energyCost_Joule += thisStat.energyCost_Joule / REP;
            stat.dataMovement_MiB += thisStat.dataMovement_MiB / REP;
            if(i == WARMUP_REP + REP - 1){
              hcp.outputTimingsToCSV("/output/MetaPB_Results/All_Graphloads/Timings_" + loadName + "_" + schedName + ".csv");
            }
          }
        }
        testResults[{loadName, schedName}] = stat;
      }
    }

    free(memPool[0]);
    free((void *)memPool);
  }

private:
  std::map<std::string, TaskGraph> graphLoads;
  std::map<std::pair<std::string, std::string>, perfStats> testResults;
};
} // namespace benchmarks
