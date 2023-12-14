#ifndef OPT_AOA_SRC
#define OPT_AOA_SRC
#include "../OptimizerAOA.hpp"
namespace Optimizer {

/// @brief Particle swarm update method.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
typename OptimizerAOA<aType, vType>::ptFrm_t
OptimizerAOA<aType, vType>::updateFunc() {
  ptFrm_t result;
  return result;
}

} // namespace Optimizer
#endif
