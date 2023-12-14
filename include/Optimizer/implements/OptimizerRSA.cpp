#ifndef OPT_RSA_SRC
#define OPT_RSA_SRC
#include "../OptimizerRSA.hpp"
namespace Optimizer {

/// @brief Particle swarm update method.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
typename OptimizerRSA<aType, vType>::ptFrm_t
OptimizerRSA<aType, vType>::updateFunc() {
  ptFrm_t result;
  return result;
}

} // namespace Optimizer
#endif
