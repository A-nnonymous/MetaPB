#ifndef OP_BASE_HPP
#define OP_BASE_HPP

#define PREPROBE_WARMUP_REP 3
#define PROBE_REP 10
#define BATCH_LOWERBOUND_MB 128
#define PERF_SAMPLE_POINT 8
#define REGRESSION_TRAINING_ITER 200
#define DPU_ENERGY_CONSTANT_PER_SEC 280.0

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Executor/TaskGraph.hpp"
#include "utils/Learner.hpp"
#include "utils/ChronoTrigger.hpp"


namespace MetaPB {
namespace Operator {

using perfStats = utils::perfStats;
using Learner = utils::Learner;
using ChronoTrigger = utils::ChronoTrigger;
/// @brief This class is the uniformed interface of all operator.
class OperatorBase {
public:
  OperatorBase() = default;

  virtual void execCPU(const size_t batchSize_MiB) = 0;
  virtual void execDPU(const size_t batchSize_MiB) = 0;
  // -------------- Optimization phase utilities ----------------
  void trainModel(const size_t batchUpperBound_MiB) noexcept;
  void dumpMetrics(const size_t batchSize_MiB, const std::string dumpPath)noexcept;
  perfStats deducePerf(const double offloadRatio, const size_t batchSize_MiB)noexcept;

  virtual const std::string get_name() = 0;
private:
  perfStats execCPUwithProbe(const size_t batchSize_MiB) noexcept;
  perfStats execDPUwithProbe(const size_t batchSize_MiB) noexcept;

  // storing all datasize to taskname
  std::map<float, std::string> cpuExecJob2Name;
  std::map<float, std::string> dpuExecJob2Name;

  Learner CPUPerfLearner;
  Learner CPUEnergyLearner;
  Learner DPUPerfLearner;

  ChronoTrigger ct;
};

} // namespace Operator
} // namespace MetaPB
#endif
