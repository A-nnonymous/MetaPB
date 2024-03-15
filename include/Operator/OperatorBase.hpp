#ifndef OP_BASE_HPP
#define OP_BASE_HPP

#define DPU_ENERGY_CONSTANT_PER_NS 280.0 / 1000000000

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "Executor/TaskGraph.hpp"
#include "utils/Learner.hpp"

using perfStats = MetaPB::Executor::perfStats;
using Learner = MetaPB::utils::Learner;

namespace MetaPB {
namespace Operator {
/// @brief This class is the uniformed interface of all operator.
class OperatorBase {
public:
  OperatorBase() = default;

  virtual void execCPU(const size_t batchSize) = 0;
  virtual void execDPU(const size_t batchSize) = 0;
  // -------------- Optimization phase utilities ----------------
  void trainModel(const size_t batchUpperBound);
  /// @brief About CPU, DPU, hybrid metrics in csv file
  /// @param dumpPath Path where the csv file dumped.
  void dumpMetrics(const std::string dumpPath);
  perfStats deducePerf(const double offloadRatio, const size_t batchSize);

private:
  perfStats execCPUwithProbe(const size_t batchSize);
  perfStats execDPUwithProbe(const size_t batchSize);
  Learner CPUPerfLearner;
  Learner CPUEnergyLearner;
  Learner DPUPerfLearner;
  inline static const std::string OpName = "Base";
};

} // namespace Operator
} // namespace MetaPB
#endif
