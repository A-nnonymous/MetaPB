#ifndef OPT_BASE_SRC
#define OPT_BASE_SRC
#include "../OptimizerBase.hpp"

namespace MetaPB {
namespace Optimizer {

template <typename aType, typename vType>
OptimizerBase<aType, vType>::OptimizerBase(
    const pt_t lowerLimits, const pt_t upperLimits, const size_t dimNum,
    const size_t pointNum, const size_t iterNum,
    const function<valFrm_t(const ptFrm_t &)> &evalFunc)
    : lowerLimits(lowerLimits), upperLimits(upperLimits), dimNum(dimNum),
      pointNum(pointNum), iterNum(iterNum), evaluateFunc(evalFunc) {
  // Reserve place for further emplace_back
#ifndef NDEBUG
  assert(pointNum >= 1 && "Illegal pointNum");
  assert(iterNum >= 1 && "Illegal iterNum");
#endif
  ptHistory.reserve(iterNum);
  valHistory.reserve(iterNum);
  gBestPtHist.reserve(iterNum);
  gBestValHist.reserve(iterNum);
  // Random initialize point position of first frame
  vector<pt_t> firstFrm;
  std::mt19937 rng(std::random_device{}());
  firstFrm.reserve(pointNum);
  for (size_t agtIdx = 0; agtIdx < pointNum; ++agtIdx) {
    pt_t point(dimNum, (aType)0);
    for (size_t dimIdx = 0; dimIdx < dimNum; ++dimIdx) {
      // Compile time type inference and random data initialize.
      if constexpr (std::is_integral<aType>::value) {
        std::uniform_int_distribution<aType> dist(lowerLimits[dimIdx],
                                                  upperLimits[dimIdx]);
        point[dimIdx] = dist(rng);
      } else if constexpr (std::is_floating_point<aType>::value) {
        std::uniform_real_distribution<aType> dist(lowerLimits[dimIdx],
                                                   upperLimits[dimIdx]);
        point[dimIdx] = dist(rng);
      }
    }
    firstFrm.emplace_back(std::move(point));
  }
  ptHistory.emplace_back(std::move(firstFrm));
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::exec() noexcept {
  //#ifndef NDEBUG
  auto start = std::chrono::high_resolution_clock::now();
  //#endif
  // Invariant: All point frame till here is valid to evaluate.
  for (size_t frmIdx = 0; frmIdx < iterNum; ++frmIdx) {
    exploitation();
    convergeLogging();
    extraction();
    if (frmIdx != iterNum - 1) [[likely]]
      exploration();
  }
  //#ifndef NDEBUG
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  double duration_ms = duration.count() / 1000000.0;
  std::cout << "\nWith time consumed " << duration_ms << " ms" << std::endl;
  debug_check();
  //#endif
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::exploitation() noexcept {
  valFrm_t result(pointNum, (vType)0);
  ptFrm_t newPoints;
  vector<size_t> newPtIdx;
  newPoints.reserve(pointNum); // pessimistic reservation.
  for (size_t ptIdx = 0; ptIdx != pointNum; ++ptIdx) {
    auto pt = ptHistory.back()[ptIdx];
    if (pt2Val.contains(pt)) {
      result[ptIdx] = pt2Val[pt];
    } else {
      newPoints.emplace_back(pt);
      newPtIdx.emplace_back(ptIdx);
    }
  }
  valFrm_t newResults = evaluateFunc(newPoints);
  for (size_t newIdx = 0; newIdx != newResults.size(); ++newIdx) {
    result[newPtIdx[newIdx]] = newResults[newIdx];
    pt2Val[newPoints[newIdx]] = newResults[newIdx];
  }
  valHistory.emplace_back(std::move(result));
}
template <typename aType, typename vType>
void OptimizerBase<aType, vType>::convergeLogging() noexcept {
  vType fBestVal = std::numeric_limits<vType>::max();
  size_t fBestIdx = 0;
  auto &lastFrmVal = valHistory.back();
  for (size_t valIdx = 0; valIdx < lastFrmVal.size(); valIdx++) {
    if (lastFrmVal[valIdx] < lastFrmVal[fBestIdx])
      fBestIdx = valIdx;
  }
  fBestVal = lastFrmVal[fBestIdx];
  if (fBestVal < gBestVal) {
    gBestVal = fBestVal;
    gBestPt = ptHistory.back()[fBestIdx];
  }
  gBestPtHist.emplace_back(gBestPt);
  gBestValHist.emplace_back(gBestVal);
}

template <typename aType, typename vType>
typename OptimizerBase<aType, vType>::ptFrm_t
OptimizerBase<aType, vType>::getCumulatePointsHistory() const noexcept {
  ptFrm_t result;
  result.reserve(pointNum * iterNum);
  for (const auto &kv : pt2Val) {
    result.emplace_back(kv.first);
  }
  return result;
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::debug_check() const noexcept {
  // DEPRECATED
}

} // namespace Optimizer
} // namespace MetaPB
#endif
