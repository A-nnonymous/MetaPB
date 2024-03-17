#ifndef OP_BASE_HPP
#define OP_BASE_HPP

#define PREPROBE_WARMUP_REP 3
#define PROBE_REP 10
#define BATCH_LOWERBOUND_MB 128
#define PERF_SAMPLE_POINT 12
#define REGRESSION_TRAINING_ITER 200
#define REGRESSION_MODEL_CACHE_PATH "/tmp/MetaPB/perfModel/"
#define DPU_ENERGY_CONSTANT_PER_SEC 280.0

#define divceil(n, m) (((n)-1) / (m) + 1)
#define roundup(n, m) ((n / m) * m + m)

#include "DPU_GLOBAL.hpp"
#include "utils/ChronoTrigger.hpp"
#include "utils/Learner.hpp"
#include "utils/MetricsGather.hpp"
#include "utils/Stats.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace MetaPB {
namespace Operator {

using perfStats = utils::perfStats;
using Learner = utils::Learner;
using ChronoTrigger = utils::ChronoTrigger;
/// @brief This class is the uniformed interface of all operator.
class OperatorBase {
public:
  OperatorBase(std::unique_ptr<GLOBAL_DPU_MGR> &g_DPU_MGR)
      : allDPUs(g_DPU_MGR->dpu_set) {}
  inline virtual void execCPU(const size_t batchSize_MiB,
                              void **memPoolBffrPtrs) const noexcept = 0;
  inline virtual void execDPU(const size_t batchSize_MiB) const noexcept = 0;

  // -------------- Optimization phase utilities ----------------
  void trainModel(const size_t batchUpperBound_MiB, void **memPoolBffrPtrs,
                  const int iterMax = REGRESSION_TRAINING_ITER) noexcept;
  void dumpMetrics(const size_t batchSize_MiB, void **memPoolBffrPtrs,
                   const std::string dumpPath) noexcept;
  perfStats deducePerf(const double offloadRatio,
                       const size_t batchSize_MiB) noexcept;

  std::string getDPUBinaryPath() const noexcept;
  inline virtual const std::string get_name() const noexcept = 0;
  inline virtual constexpr int getInputTensorNum() const noexcept = 0;
  virtual inline constexpr bool checkIfIsTrainable() const noexcept = 0;
  virtual inline constexpr bool checkIfIsCPUOnly() const noexcept = 0;
  virtual inline const bool checkIfIsTrained() const noexcept {
    return isTrained;
  }

private:
  perfStats execCPUwithProbe(const size_t batchSize_MiB,
                             void **memPoolBffrPtrs) noexcept;
  perfStats execDPUwithProbe(const size_t batchSize_MiB) noexcept;

  void cacheModel(const size_t batchSize_MiB) const noexcept;
  bool loadModelCacheIfExist(const size_t batchSize_MiB) noexcept;

  // storing all datasize to taskname
  std::map<float, std::string> cpuExecJob2Name;
  std::map<float, std::string> dpuExecJob2Name;

  Learner CPUPerfLearner;
  Learner CPUEnergyLearner;
  Learner DPUPerfLearner;
  inline static bool isTrained = false;
  ChronoTrigger ct;

protected:
  dpu_set_t &allDPUs;
};

} // namespace Operator
} // namespace MetaPB
#endif
