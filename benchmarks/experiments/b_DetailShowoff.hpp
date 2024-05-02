#include "experiments/benchmark.hpp"
#include "helpers/helper.hpp"
#include "workloads/string_workload.hpp"

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

using Scheduler::MetaScheduler;
using Scheduler::CPUOnlyScheduler;
using Scheduler::DPUOnlyScheduler;
using Scheduler::HEFTScheduler;
using Scheduler::GreedyScheduler;

using OptimizerInfos = Scheduler::MetaScheduler::OptimizerInfos;

using utils::perfStats;
using utils::Schedule;

using pt_t = Optimizer::OptimizerBase<float, float>::pt_t;
using valFrm_t = Optimizer::OptimizerBase<float, float>::valFrm_t;
using ptFrm_t = Optimizer::OptimizerBase<float, float>::ptFrm_t;
using ptHist_t = Optimizer::OptimizerBase<float, float>::ptHist_t;
using valHist_t = Optimizer::OptimizerBase<float, float>::valHist_t;

namespace benchmarks {
class DetailShowOff : public Benchmark {
public:
  using Benchmark::Benchmark;
  void writeCSV(const std::string &dumpPath) const noexcept override {
    std::ofstream file(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                       +"wholesome_perf.csv");
    file << "scheduler_name,timeConsume_Seconds,energyConsume_"
            "Joules,dataTransfer_MiB\n";
    for (const auto &[schedName, stats] : perfs) {
      file << schedName<< "," << std::to_string(stats.timeCost_Second) << ","
                       << std::to_string(stats.energyCost_Joule) << ","
                       << std::to_string(stats.dataMovement_MiB) << "\n";
    }

    for (const auto &[schedName, optInfos] : metaData) {
      std::uint32_t opNum = std::stoi(myArgs.at("opNum"));
      std::uint32_t pointNum = optInfos.totalValHist[0].size();
      std::uint32_t iterNum = optInfos.totalValHist.size();
      std::ofstream tphfile(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                            myArgs.at("opNum") + "_" + 
                            schedName + "_"+
                            optInfos.optimizerName+ "_" +
                            +"totalPtHist.csv");
      // headers
      for (int i = 0; i < opNum; i++) {
        tphfile << "Load" << std::to_string(i);
        if (i != opNum - 1) {
          tphfile << ",";
        } else {
          tphfile << "\n";
        }
      }
      // data
      for (const auto &pt : optInfos.totalPtFrm) {
        for (int i = 0; i < opNum; i++) {
          tphfile << std::to_string(pt[i] / 200.0 + 0.5f);
          if (i != opNum - 1) {
            tphfile << ",";
          } else {
            tphfile << "\n";
          }
        }
      }
      std::ofstream tvhfile(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                            myArgs.at("opNum") + "_" +
                            schedName + "_"+
                            optInfos.optimizerName+ "_" +
                            +"totalValHist.csv");
      // headers
      for (int i = 0; i < pointNum; i++) {
        tvhfile << "Point" << std::to_string(i);
        if (i != pointNum - 1) {
          tvhfile << ",";
        } else {
          tvhfile << "\n";
        }
      }
      // data
      for (const auto &ptvals : optInfos.totalValHist) {
        for (int i = 0; i < pointNum; i++) {
          tvhfile << std::to_string(ptvals[i]);
          if (i != pointNum - 1) {
            tvhfile << ",";
          } else {
            tvhfile << "\n";
          }
        }
      }

      std::ofstream cpffile(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                            myArgs.at("opNum") + "_" + 
                            schedName + "_"+
                            optInfos.optimizerName+ "_" +
                            +"convergePtFrm.csv");
      // headers
      for (int i = 0; i < opNum; i++) {
        cpffile << "Load" << std::to_string(i);
        if (i != opNum - 1) {
          cpffile << ",";
        } else {
          cpffile << "\n";
        }
      }
      // data
      for (const auto &pt : optInfos.convergePtFrm) {
        for (int i = 0; i < opNum; i++) {
          cpffile << std::to_string(pt[i] / 200.0 + 0.5f);
          if (i != opNum - 1) {
            cpffile << ",";
          } else {
            cpffile << "\n";
          }
        }
      }
      std::ofstream cvffile(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                            myArgs.at("opNum") + "_" + 
                            schedName + "_"+
                            optInfos.optimizerName+ "_" +
                            +"convergeValFrm.csv");
      cvffile << "global_fitness\n";
      for (int i = 0; i < iterNum; i++) {
        cvffile << std::to_string(optInfos.convergeValFrm[i]);
        cvffile << "\n";
      }
    }
    for (const auto &[schedName, schedule] : schedules) {
      std::uint32_t opNum = std::stoi(myArgs.at("opNum"));
      std::ofstream sfile(dumpPath + myArgs.at("loadSize_MiB") + "MiB_" +
                          myArgs.at("opNum") + "_"  +
                          schedName + "_"+
                          +"schedule.csv");
      for (int i = 0; i < opNum; i++) {
        sfile << "Load" << std::to_string(i);
        if (i != opNum - 1) {
          sfile << ",";
        } else {
          sfile << "\n";
        }
      }
      // data
      for (int i = 0; i < opNum; i++) {
        sfile << std::to_string(schedule.offloadRatio[i]);
        if (i != opNum - 1) {
          sfile << ",";
        } else {
          sfile << "\n";
        }
      }
    }
  }
  void exec() override {
    size_t loadSize_MiB = std::stoul(myArgs["loadSize_MiB"]);
    size_t opNum = std::stoul(myArgs["opNum"]);
    void **memPool = (void **)malloc(3);
    for (int i = 0; i < 3; i++) {
      memPool[i] = malloc(loadSize_MiB * size_t(1 << 20));
    }
    OperatorManager om;
    HeteroComputePool hcp(opNum + 2, om, memPool);
    TaskGraph tg = genInterleavedWorkload(loadSize_MiB, opNum);
    om.trainModel(tg.genRegressionTask());
    MetaScheduler msPF(0.3f, 0.7f, 100, tg, om);
    MetaScheduler msHy(0.5f, 0.5f, 100, tg, om);
    MetaScheduler msEF(0.7f, 0.3f, 100, tg, om);
    ///////// adding /////////
    HEFTScheduler    heft(tg, om);
    GreedyScheduler  greedy;
    CPUOnlyScheduler cpuOnly;
    DPUOnlyScheduler dpuOnly;
    schedules[ "HEFT"   ]= heft.schedule();
    schedules[ "Greedy" ]= greedy.schedule(tg, om);
    schedules[ "CPUOnly"]= cpuOnly.schedule(tg, om);
    schedules[ "DPUOnly"]= dpuOnly.schedule(tg, om);
    ///////// adding /////////

    schedules["MetaPB_PerfFirst"] = msPF.schedule();
    schedules["MetaPB_Hybrid"] = msHy.schedule();
    schedules["MetaPB_EnergyFirst"] = msEF.schedule();

    perfs["MetaPB_PerfFirst"] =
        hcp.execWorkload(tg, schedules.at("MetaPB_PerfFirst"), execType::DO);
    perfs["MetaPB_Hybrid"] =
        hcp.execWorkload(tg, schedules.at("MetaPB_Hybrid"), execType::DO);
    perfs["MetaPB_EnergyFirst"] =
        hcp.execWorkload(tg, schedules.at("MetaPB_EnergyFirst"), execType::DO);
    
    for(const auto& [schedName, sched] : schedules){
        std::cout << 
          "executing "<< schedName << 
          "'s schedule on \n"; 
        perfStats stat;
        for (int i = 0; i < WARMUP_REP + REP; i++) {
          perfStats thisStat = hcp.execWorkload(tg, sched, execType::DO);
          if (i >= WARMUP_REP) {
            stat.timeCost_Second += thisStat.timeCost_Second / REP;
            stat.energyCost_Joule += thisStat.energyCost_Joule / REP;
            stat.dataMovement_MiB += thisStat.dataMovement_MiB / REP;
            if(i == WARMUP_REP + REP - 1){
              hcp.outputTimingsToCSV("/output/MetaPB_Results/string_workload/"+ std::to_string(loadSize_MiB) + "MiB_Timings_" + schedName +".csv");
            }
          }

        }
        perfs[schedName] = stat;
    }

    metaData["MetaPB_PerfFirst"] = msPF.getOptInfo();
    metaData["MetaPB_Hybrid"] = msHy.getOptInfo();
    metaData["MetaPB_EnergyFirst"] = msEF.getOptInfo();

    for (int i = 0; i < 3; i++) {
      free(memPool[i]);
    }
    free((void *)memPool);
  }

private:
  std::map<std::string, OptimizerInfos> metaData;
  std::map<std::string, Schedule> schedules;
  std::map<std::string, perfStats> perfs;
};
} // namespace benchmarks
