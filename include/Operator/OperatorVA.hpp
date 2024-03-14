#ifndef OP_BASE_HPP
#define OP_BASE_HPP

#include "utils/Consensus.h"
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

/// @brief This class is the uniformed interface of all operator.
class OperatorVAOperatorBase {
public:
  /// @brief Ordered map of all arguments, mapping name to float(converted).
  // Be aware of type convertion.
  struct ArgMap {
    map<string, float> args;
    ArgMap() {
      args["CPU_Ratio"] = 1.0f;
      args["DMA_BlockSize_Log2"] = 10.0f;
      std::cout << UINT32_32ALN << std::endl;
    }
  };

  OperatorBase() = default;

  /// @brief Perform splitting and execution of exact kernel with given
  /// arguments
  /// @param ratio CPU work Ratio, set as 1.0 or 0.0 in Quantilization phase.
  /// @param args Arguments of this kernel passed by upper level caller.
  virtual void exec(const ArgMap &args) = 0;

private:
  virtual void execCPU(const ArgMap &) = 0;
  virtual void execDPU(const ArgMap &) = 0;

  static float predictUsingModel(const vector<float> &inputFeature,
                                 BoosterHandle booster) noexcept;
  /// @brief The main concern in MetaPB is time and energy.
  // Each xgboost model handle is a proxy to an trained performance regression
  // model. It can be utilized to perform prediction of performance using only
  // few arguments.
  typedef struct {
    BoosterHandle timeConsumeModelHandle;
    BoosterHandle energyConsumeModelHandle;
  } PerfModel;

  // -------------------- Static Zone ---------------------
  inline static PerfModel CPUPerfModel;
  inline static PerfModel DPUPerfModel;
  inline static const std::string OpName = "Base";

public:
  static vector<float> getPerfSignature(const ArgMap &argmap);
  // ------------------- Phase Control --------------------
  inline static bool isModelized = false; // indicate whether this operator is
                                          // tested and regression modelized.

  // -------------- Quantization phase utilities ----------------
  static void setPerfModel(const BoosterHandle tModel,
                           const BoosterHandle eModel,
                           const BoosterHandle tModelD,
                           const BoosterHandle eModelD) noexcept;
  // -------------- Optimization phase utilities ----------------
  static float predictTimeConsume(const ArgMap &proposed) noexcept;
  static float predictEnergyConsume(const ArgMap &proposed) noexcept;
};
} // namespace Operator
} // namespace MetaPB
#endif
