#ifndef OPT_PSO_SRC
#define OPT_PSO_SRC
#include "../OptimizerPSO.hpp"
namespace Optimizer {


template <typename aType, typename vType>
OptimizerPSO<aType, vType>::OptimizerPSO(const double vMax, const double omega,
               const double dt, const double ego,
               const pt_t lowerLimits, const pt_t upperLimits,
               const size_t dimNum, const size_t pointNum, const size_t iterNum,
               const function<valFrm_t(const ptFrm_t &)> &evalFunc)
               : OptimizerBase<aType, vType>(lowerLimits,upperLimits,dimNum, pointNum,
                                             iterNum, evalFunc),
                 vMax(vMax), omega(omega), dt(dt), ego(ego) {
  pBestPts.resize(pointNum);
  pBestVals.resize(pointNum, std::numeric_limits<vType>::max());
  velocities.resize(pointNum, vector<double>(dimNum));

  // Random initialize velocities.
  std::mt19937_64 rng(std::random_device{}());
  std::uniform_real_distribution<double> distV(-vMax, vMax);
  for(size_t ptIdx = 0; ptIdx != pointNum; ptIdx++){
    for(size_t dimIdx = 0; dimIdx != dimNum; dimIdx++){
      velocities[ptIdx][dimIdx] = distV(rng) * (upperLimits[dimIdx] - lowerLimits[dimIdx]);
    }
  }

}
template <typename aType, typename vType>
void OptimizerPSO<aType, vType>::extraction() noexcept {
  valFrm_t& lastValFrm = this->valHistory.back();
  ptFrm_t& lastPtFrm = this->ptHistory.back();
  for(size_t ptIdx = 0; ptIdx != lastPtFrm.size(); ++ptIdx){
    if(lastValFrm[ptIdx] < pBestVals[ptIdx]){
      pBestVals[ptIdx] = lastValFrm[ptIdx];
      pBestPts[ptIdx] = lastPtFrm[ptIdx];
    }
  }
}
/// @brief Particle swarm update method with handling of generic types.
/// @tparam aType Points' arguments datatype
/// @tparam vType Points' value datatype
/// @param points Target points frame.
/// @param pVals Evaluated values of target points frame.
/// @return New points frame.
template <typename aType, typename vType>
void OptimizerPSO<aType, vType>::exploration() noexcept {
  // Invariant: As the optimizer executed to this point, it maintains a global &
  // frame-wise maximum and minimum point/val to be used by user-defined update 
  // func and the passed history of all points, include all those special points.
  ptFrm_t newFrm;
  newFrm.reserve(this->ptHistory.back().size());
  std::mt19937_64 rng(std::random_device{}());
  std::uniform_real_distribution<double> dist01(0, 1);

  for(size_t ptIdx = 0; ptIdx != this->pointNum; ++ptIdx){
    auto& myVelocity = velocities[ptIdx];
    const auto& myPt = this->ptHistory.back()[ptIdx];
    pt_t newPt = myPt;
    for(size_t dimIdx = 0; dimIdx != this->dimNum; ++dimIdx){
      // Core algorithm of PSO
      double r1 = dist01(rng);
      double r2 = dist01(rng);
      auto upperBound = this->upperLimits[dimIdx];
      auto lowerBound = this->lowerLimits[dimIdx];
      double vUpper = vMax*(upperBound - lowerBound);
      double vLower = - vUpper;
      myVelocity[dimIdx] = omega * myVelocity[dimIdx] +
                           (r1 * ego) * (pBestPts[ptIdx][dimIdx] - myPt[dimIdx]) +
                           (r2 / ego) * (this->gBestPt[dimIdx] - myPt[dimIdx]);
      myVelocity[dimIdx] = myVelocity[dimIdx] > vUpper? vUpper : myVelocity[dimIdx];
      myVelocity[dimIdx] = myVelocity[dimIdx] < vLower? vLower : myVelocity[dimIdx];
      double dx = myVelocity[dimIdx] * dt;
      // Handle rounding and limiting of integer arguments.
      if constexpr (std::is_integral<aType>::value) {
        newPt[dimIdx] += std::lround(dx);
      } else if constexpr (std::is_floating_point<aType>::value) {
        newPt[dimIdx] += (aType)(dx);
      }
      newPt[dimIdx] = newPt[dimIdx] > upperBound? upperBound : newPt[dimIdx];
      newPt[dimIdx] = newPt[dimIdx] < lowerBound? lowerBound : newPt[dimIdx];
    }
    newFrm.emplace_back(std::move(newPt));
  } 
  // Maintain the ptHistory.
  this->ptHistory.emplace_back(std::move(newFrm));
}

} // namespace Optimizer
#endif
