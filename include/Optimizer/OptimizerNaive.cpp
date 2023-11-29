#ifndef OPT_NAIVE_SRC
#define OPT_NAIVE_SRC
#include "OptimizerNaive.hpp"
namespace Optimizer {
template <typename aType, typename vType>
typename OptimizerNaive<aType, vType>::ptFrm_t
OptimizerNaive<aType, vType>::updateFunc(
    const typename OptimizerNaive<aType, vType>::ptFrm_t &points,
    const typename OptimizerNaive<aType, vType>::valFrm_t &pVals) {
  return points;
}

template <typename aType, typename vType>
typename OptimizerNaive<aType, vType>::valFrm_t
OptimizerNaive<aType, vType>::evaluateFunc(
    const OptimizerNaive<aType, vType>::ptFrm_t &points) {
  vector<double> result;
  result.reserve(points.size());
  for (auto &&point : points) {
    double sum = 0.0f;
    for (auto &&dim : point) {
      sum += dim;
    }
    result.emplace_back(sum);
  }
  return result;
}
} // namespace Optimizer
#endif