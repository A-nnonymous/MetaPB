#include "Operators/OperatorBase.hpp"

namespace MetaPB {
namespace Operator {

/// @brief Provide performance related argument vector as feature map.
// This function might be used in both quantization phase and stratGen phase.
/// @return Float type feature map of performance related arguments.
vector<float> OperatorBase::getPerfSignature(const ArgMap &argmap) {
  vector<float> result;
  for (const auto &[k, v] : argmap.args) {
    result.emplace_back(v);
  }
  return result;
}

/// @brief Set the performance model to this operator
/// @param tModel Time consume model of CPU kernel
/// @param eModel Energy consume model of CPU kernel
/// @param tModelD Time consume model of DPU kernel
/// @param eModelD Energy consume model of DPU kernel(using Time)
void OperatorBase::setPerfModel(const BoosterHandle tModel,
                                const BoosterHandle eModel,
                                const BoosterHandle tModelD,
                                const BoosterHandle eModelD) noexcept {
  CPUPerfModel.timeConsumeModelHandle = tModel;
  CPUPerfModel.energyConsumeModelHandle = eModel;
  DPUPerfModel.timeConsumeModelHandle = tModelD;
  DPUPerfModel.energyConsumeModelHandle = eModelD;
}

/// @brief Using pre-trained regression model to predict consume metrics
/// @param inputFeature Arguments that passed to operator
/// @param booster xgBoost regression model
/// @return Predicted consume.
float OperatorBase::predictUsingModel(const vector<float> &inputFeature,
                                      BoosterHandle booster) noexcept {
  DMatrixHandle proposedFeature;
  safe_xgboost(XGDMatrixCreateFromMat(inputFeature.data(), 1,
                                      inputFeature.size(), std::nan(""),
                                      &proposedFeature));
  char const config[] = "{\"training\": false, \"type\": 0, "
                        "\"iteration_begin\": 0, \"iteration_end\": 0, "
                        "\"strict_shape\": false}";
  /* Shape of output prediction */
  uint64_t const *out_shape;
  /* Dimension of output prediction */
  uint64_t out_dim;
  /* Pointer to a thread local contigious array, assigned in prediction
   * function. */
  float const *out_result = NULL;
  safe_xgboost(XGBoosterPredictFromDMatrix(booster, proposedFeature, config,
                                           &out_shape, &out_dim, &out_result));
  return out_result[0];
}

/// @brief Predict the whole time consume, assume that the CPU-DPU is working in
/// parallel
/// @param proposed Metaheuristic proposed argument set
/// @return Predicted time consume.
float OperatorBase::predictTimeConsume(const ArgMap &proposed) noexcept {
  auto pvec = getPerfSignature(proposed);
  return std::max(predictUsingModel(pvec, CPUPerfModel.timeConsumeModelHandle),
                  predictUsingModel(pvec, DPUPerfModel.timeConsumeModelHandle));
}

/// @brief Predict the whole energy cost, assume that DPU is always at TDP 280W
/// @param proposed Metaheuristic proposed argument set
/// @return Predicted time consume.
float OperatorBase::predictEnergyConsume(const ArgMap &proposed) noexcept {
  auto pvec = getPerfSignature(proposed);
  return predictUsingModel(pvec, CPUPerfModel.energyConsumeModelHandle) +
         POWER_CONSTANT_NS *
             predictUsingModel(pvec, DPUPerfModel.timeConsumeModelHandle);
}

} // namespace Operator
} // namespace MetaPB