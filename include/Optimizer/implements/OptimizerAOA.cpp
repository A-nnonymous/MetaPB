#ifndef OPT_AOA_SRC
#define OPT_AOA_SRC
#include "../OptimizerAOA.hpp"

namespace MetaPB {
namespace Optimizer {

/// @brief Extract additional information that needed by AOA
template <typename aType, typename vType>
void OptimizerAOA<aType, vType>::extraction() noexcept {
  // Iteration related argument extraction.
  currIterIdx++;
  arg_w = (-2 / M_PI) * (atan(currIterIdx)) + 1 +
          0.5 * exp(-(double)currIterIdx / 5);
  arg_MOP = 1 - (pow(currIterIdx, (1 / arg_ALPHA)) /
                 pow(this->iterNum, (1 / arg_ALPHA)));

  // Additional worst point extraction.
  const auto &lastValFrm = this->valHistory.back();
  const auto &lastPtFrm = this->ptHistory.back();
  for (size_t ptIdx = 0; ptIdx != lastPtFrm.size(); ++ptIdx) {
    if (lastValFrm[ptIdx] > gWorstVal) {
      gWorstVal = lastValFrm[ptIdx];
      gWorstPt = lastPtFrm[ptIdx];
    }
  }
}

/// @brief AOA update method.
/// @return New points frame.
template <typename aType, typename vType>
void OptimizerAOA<aType, vType>::exploration() noexcept {
  // Invariant: As the optimizer executed to this point, it maintains a global &
  // frame-wise optimum point/val(till now) to be used by user-defined update
  // func and the passed history of all points.
  ptFrm_t newFrm;
  newFrm.reserve(this->ptHistory.back().size());

  for (size_t ptIdx = 0; ptIdx != this->pointNum; ++ptIdx) {
    const auto &thisPt = this->ptHistory.back()[ptIdx];
    const auto &thisVal = this->valHistory.back()[ptIdx];
    //const double MOA =
    //   1 - pow((gWorstVal - thisVal) / (this->gBestVal - gWorstVal), arg_k);
    const double MOA = 0.2 + currIterIdx*(0.8/this->iterNum);

    pt_t newPt(this->dimNum, (aType)0);
    for (size_t dimIdx = 0; dimIdx != this->dimNum; ++dimIdx) {
      const auto &upperBound = this->upperLimits[dimIdx];
      const auto &lowerBound = this->lowerLimits[dimIdx];
      // Core algorithm of AOA
      const double r1 = this->dist01(this->rng);
      const double r2 = this->dist01(this->rng);
      const double r3 = this->dist01(this->rng);
      double propose = 1.0f;
      if (r1 < MOA) {
        if (r2 > 0.5) {
          propose = this->gBestPt[dimIdx] / (arg_MOP + __DBL_MIN__) *
                    ((upperBound - lowerBound) * arg_Mu + lowerBound);
        } else {
          propose = this->gBestPt[dimIdx] * arg_MOP *
                    ((upperBound - lowerBound) * arg_Mu + lowerBound);
        }
      } else {
        if (r3 > 0.5) {
          propose = this->gBestPt[dimIdx] -
                    arg_MOP * ((upperBound - lowerBound) * arg_Mu + lowerBound);
          /*
          newPt[dimIdx] = _w * _myPosition[dimIdx] +
                          arg_MOP * sin(2 * M_PI * dist(_rng)) *
                          fabs(2 * dist(_rng) * this->gBestPt[dimIdx] -
          _myPosition[dimIdx]);
          */
        } else {
          propose = this->gBestPt[dimIdx] +
                    arg_MOP * ((upperBound - lowerBound) * arg_Mu + lowerBound);
          /*
          newPt[dimIdx] = _w * _myPosition[dimIdx] +
                          arg_MOP * cos(2 * M_PI * dist(_rng)) *
                          fabs(2 * dist(_rng) * this->gBestPt[dimIdx] -
          _myPosition[dimIdx]);
          */
        }
      }

      // Handling datatype adaptation.
      if constexpr (std::is_integral<aType>::value) {
        newPt[dimIdx] = static_cast<aType>(std::lround(propose));
      } else if constexpr (std::is_floating_point<aType>::value) {
        newPt[dimIdx] = static_cast<aType>(propose);
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
} // namespace MetaPB
#endif
