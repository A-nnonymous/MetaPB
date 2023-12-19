#ifndef OPT_RSA_SRC
#define OPT_RSA_SRC
#include "../OptimizerRSA.hpp"
namespace Optimizer {

/// @brief Extract additional information that needed by RSA
template <typename aType, typename vType>
void OptimizerRSA<aType, vType>::extraction() noexcept {
  // Iteration related argument extraction.
  currIterIdx++;
  evoSense =
      evoScaleDist(this->rng) * (1.0 - (currIterIdx / (double)this->iterNum));
}

/// @brief RSA update method.
template <typename aType, typename vType>
void OptimizerRSA<aType, vType>::exploration() noexcept {
  // Invariant: As the optimizer executed to this point, it maintains a global &
  // frame-wise optimum point/val(till now) to be used by user-defined update
  // func and the passed history of all points.
  ptFrm_t newFrm;
  newFrm.reserve(this->ptHistory.back().size());

  for (size_t ptIdx = 0; ptIdx != this->pointNum; ++ptIdx) {
    const auto &thisPt = this->ptHistory.back()[ptIdx];
    const auto &thisVal = this->valHistory.back()[ptIdx];
    const auto &thisFrm = this->ptHistory.back();
    pt_t newPt(this->dimNum, (aType)0);
    for (size_t dimIdx = 0; dimIdx != this->dimNum; ++dimIdx) {
      const auto &upperBound = this->upperLimits[dimIdx];
      const auto &lowerBound = this->lowerLimits[dimIdx];
      double propose;
      // Core algorithm of RSA
      const size_t thisChoice = choice(this->rng);
      const double thisMean =
          std::reduce(thisPt.begin(), thisPt.end()) / (double)this->dimNum;
      const double dimRange = double(upperBound - lowerBound);

      const double R =
          this->gBestPt[dimIdx] -
          (thisFrm[thisChoice][dimIdx]) / (this->gBestPt[dimIdx] + __DBL_MIN__);
      const double P =
          arg_ALPHA + (thisPt[dimIdx] - thisMean) /
                          (this->gBestPt[dimIdx] * dimRange + __DBL_MIN__);
      const double Eta = this->gBestPt[dimIdx] * P;

      if (currIterIdx <= (this->iterNum) / 4) // Stage 1
      {
        propose = this->gBestPt[dimIdx] - Eta * arg_BETA -
                  R * this->dist01(this->rng);
      } else if (currIterIdx <= ((this->iterNum) / 2) &&
                 currIterIdx > ((this->iterNum) / 4)) // Stage 2
      {
        propose = this->gBestPt[dimIdx] * thisFrm[thisChoice][dimIdx] *
                  evoSense * this->dist01(this->rng);
      } else if (currIterIdx <= (3 * (this->iterNum) / 4) &&
                 currIterIdx > ((this->iterNum) / 2)) // Stage 3
      {
        propose = this->gBestPt[dimIdx] * P * this->dist01(this->rng);
      } else // Stage 4
      {
        propose = this->gBestPt[dimIdx] - Eta * __DBL_MIN__ -
                  R * this->dist01(this->rng);
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
#endif
