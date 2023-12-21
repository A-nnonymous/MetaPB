#ifndef OPT_NAIVE_SRC
#define OPT_NAIVE_SRC
#include "../OptimizerNaive.hpp"

namespace MetaPB{
namespace Optimizer {

template <typename aType, typename vType>
void OptimizerNaive<aType, vType>::extraction() noexcept {}

/// @brief Simple naive update method, keep all points unchanged.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
void OptimizerNaive<aType, vType>::exploration() noexcept {
  ptFrm_t result;
  size_t iterRemains = (this->iterNum - this->iterCounter - 1);
  double progress = ((double)this->iterCounter / (double)this->iterNum);
  double progRemain = 1.0 - progress;
  std::mt19937 rng(std::random_device{}());
  // double noiseProb = (progress > 0.9) ? 0.0 : 0.9 - progress;
  double noiseProb = 0.9 * pow(progRemain, 2);
  std::bernoulli_distribution noiseDist(noiseProb);
  std::uniform_real_distribution<double> ratioDist(pow(progRemain, 2), 1.0);
  result.reserve(this->pointNum);
  for (const auto &point : this->ptHistory.back()) {
    pt_t newPoint(point.size());
    bool isNoise = noiseDist(rng);
    for (size_t dim = 0; dim != point.size(); ++dim) {
      aType diff = optima[dim] - point[dim];
      aType velocity = (isNoise ? ceil(-diff * ratioDist(rng))
                                : ceil(diff * ratioDist(rng)));
      aType ration = (diff / iterRemains) * 20;
      if (velocity > ration)
        velocity = isNoise ? -ration : ration;
      // bool isNoise = false;
      newPoint[dim] = point[dim] + velocity;
      if (newPoint[dim] > this->upperLimits[dim])
        newPoint[dim] = this->upperLimits[dim];
      if (newPoint[dim] < this->lowerLimits[dim])
        newPoint[dim] = this->lowerLimits[dim];
    }
    result.emplace_back(std::move(newPoint));
  }
  this->ptHistory.emplace_back(std::move(result));
}

} // namespace Optimizer
} // namespace MetaPB
#endif
