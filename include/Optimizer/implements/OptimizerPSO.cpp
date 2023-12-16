#ifndef OPT_PSO_SRC
#define OPT_PSO_SRC
#include "../OptimizerPSO.hpp"
namespace Optimizer {

/// @brief Particle swarm update method with handling of generic types.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
typename OptimizerPSO<aType, vType>::ptFrm_t
OptimizerPSO<aType, vType>::updateFunc() {
  // Invariant: As the optimizer executed to this point, it maintains a global &
  // frame-wise maximum and minimum point/val to be used by user-defined update 
  // func and the passed history of all points, include all those special points.
  ptFrm_t result;
  std::mt19937_64 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist01(0, 1);
  result.reserve(this->ptHistory.back().size());

  for(size_t ptIdx = 0; ptIdx != this->pointNum; ++ptIdx){
    pt_t newPt;
    for(size_t dimIdx = 0; dimIdx != this->dimNum; ++dimIdx){
      double r1 = dist01(rng);
      double r2 = dist01(rng);
      // Core algorithm of PSO
      /*
      // Handle rounding and limiting of integer arguments.
      if constexpr (std::is_integral<aType>::value) {
        newPt[dimIdx] = std::lround(proposedDim);
      } else if constexpr (std::is_floating_point<aType>::value) {
        newPt[dimIdx] = (aType)(proposedDim);
      }
      */
    }
    result.emplace_back(std::move(newPt));
  } 
  return result;
}

} // namespace Optimizer
#endif
