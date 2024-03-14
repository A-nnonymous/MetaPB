#ifndef OP_BASE_HPP
#define OP_BASE_HPP

#include "utils/Consensus.h"
#include "Executor/TaskGraph.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <xgboost/c_api.h>
#define DPU_ENERGY_CONSTANT_PER_NS 280.0 / 1000000000
#define safe_xgboost(call)                                                     \
  {                                                                            \
    int err = (call);                                                          \
    if (err != 0) {                                                            \
      std::cerr << __FILE__ << ":" << __LINE__ << " error in " << #call << ":" \
                << XGBGetLastError();                                          \
      exit(1);                                                                 \
    }                                                                          \
  }

using std::map;
using std::string;
using std::vector;
namespace MetaPB {
namespace Operator {
  using Executor::perfStats;

/// @brief This class is the uniformed interface of all operator.
class OperatorBase {
public:

  OperatorBase() = default;

  virtual void execCPU(const size_t batchSize) = 0;
  virtual void execDPU(const size_t batchSize) = 0;
  // -------------- Optimization phase utilities ----------------
  void trainModel(const size_t batchUpperBound);
  perfStats deducePerf(const double offloadRatio, const size_t batchSize);

private:
  // -------------------- Static Zone ---------------------
  inline static const std::string OpName = "Base";
};
} // namespace Operator
} // namespace MetaPB
#endif
