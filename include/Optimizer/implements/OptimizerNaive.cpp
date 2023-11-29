#ifndef OPT_NAIVE_SRC
#define OPT_NAIVE_SRC
#include "../OptimizerNaive.hpp"
namespace Optimizer {

/// @brief Simple naive update method, keep all points unchanged.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
typename OptimizerNaive<aType, vType>::ptFrm_t
OptimizerNaive<aType, vType>::updateFunc(
    const typename OptimizerNaive<aType, vType>::ptFrm_t &points,
    const typename OptimizerNaive<aType, vType>::valFrm_t &pVals) {
  return points;
}

} // namespace Optimizer
#endif