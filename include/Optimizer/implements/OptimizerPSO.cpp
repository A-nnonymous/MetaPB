#ifndef OPT_PSO_SRC
#define OPT_PSO_SRC
#include "../OptimizerPSO.hpp"
namespace Optimizer {

template <typename aType, typename vType>
OptimizerPSO<aType, vType>::OptimizerPSO(
    const double vMax, const double omega, const double dt, const double ego,
    const pt_t lowerLimits, const pt_t upperLimits, const size_t dimNum,
    const size_t pointNum, const size_t iterNum,
    const function<valFrm_t(const ptFrm_t &)> &evalFunc)
    : OptimizerBase<aType, vType>(lowerLimits, upperLimits, dimNum, pointNum,
                                  iterNum, evalFunc),
      vMax(vMax), omega(omega), dt(dt), ego(ego) {
  pBestPts.resize(pointNum);
  pBestVals.resize(pointNum, std::numeric_limits<vType>::max());
  velocities.resize(pointNum, vector<double>(dimNum));

  // Random initialize velocities.
  std::uniform_real_distribution<double> distV(-vMax, vMax);
  for (size_t ptIdx = 0; ptIdx != pointNum; ptIdx++) {
    for (size_t dimIdx = 0; dimIdx != dimNum; dimIdx++) {
      velocities[ptIdx][dimIdx] =
          distV(this->rng) * (upperLimits[dimIdx] - lowerLimits[dimIdx]);
    }
  }
}

/// @brief Extract additional information that needed by PSO
template <typename aType, typename vType>
void OptimizerPSO<aType, vType>::extraction() noexcept {
  const auto &lastValFrm = this->valHistory.back();
  const auto &lastPtFrm = this->ptHistory.back();
  for (size_t ptIdx = 0; ptIdx != lastPtFrm.size(); ++ptIdx) {
    if (lastValFrm[ptIdx] < pBestVals[ptIdx]) {
      pBestVals[ptIdx] = lastValFrm[ptIdx];
      pBestPts[ptIdx] = lastPtFrm[ptIdx];
    }
  }
}
/// @brief Particle swarm update method with handling of generic types.
/// @return New points frame.
template <typename aType, typename vType>
void OptimizerPSO<aType, vType>::exploration() noexcept {
  // Invariant: As the optimizer executed to this point, it maintains a global &
  // frame-wise optimum point/val(till now) to be used by user-defined update
  // func and the passed history of all points.
  ptFrm_t newFrm;
  newFrm.reserve(this->ptHistory.back().size());

  for (size_t ptIdx = 0; ptIdx != this->pointNum; ++ptIdx) {
    const auto &myPt = this->ptHistory.back()[ptIdx];
    auto &myVelocity = velocities[ptIdx];
    pt_t newPt = myPt;
    for (size_t dimIdx = 0; dimIdx != this->dimNum; ++dimIdx) {
      // Core algorithm of PSO
      const double r1 = this->dist01(this->rng);
      const double r2 = this->dist01(this->rng);
      const auto upperBound = this->upperLimits[dimIdx];
      const auto lowerBound = this->lowerLimits[dimIdx];
      const double vUpper = vMax * (upperBound - lowerBound);
      const double vLower = -vUpper;
      myVelocity[dimIdx] =
          omega * myVelocity[dimIdx] +
          (r1 * ego) * (pBestPts[ptIdx][dimIdx] - myPt[dimIdx]) +
          (r2 / ego) * (this->gBestPt[dimIdx] - myPt[dimIdx]);
      myVelocity[dimIdx] =
          myVelocity[dimIdx] > vUpper ? vUpper : myVelocity[dimIdx];
      myVelocity[dimIdx] =
          myVelocity[dimIdx] < vLower ? vLower : myVelocity[dimIdx];
      const double dx = myVelocity[dimIdx] * dt;

      // Handline datatype adaptation.
      if constexpr (std::is_integral<aType>::value) {
        newPt[dimIdx] += static_cast<aType>(std::lround(dx));
      } else if constexpr (std::is_floating_point<aType>::value) {
        newPt[dimIdx] += static_cast<aType>(dx);
      }
      newPt[dimIdx] = newPt[dimIdx] > upperBound ? upperBound : newPt[dimIdx];
      newPt[dimIdx] = newPt[dimIdx] < lowerBound ? lowerBound : newPt[dimIdx];
    }
    newFrm.emplace_back(std::move(newPt));
  }
  // Maintain the ptHistory.
  this->ptHistory.emplace_back(std::move(newFrm));
}

} // namespace Optimizer
#endif
