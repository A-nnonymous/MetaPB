#ifndef OPT_BASE_SRC
#define OPT_BASE_SRC
#include "../OptimizerBase.hpp"
namespace Optimizer {

template <typename aType, typename vType>
OptimizerBase<aType, vType>::OptimizerBase(
    const pt_t lowerLimits, const pt_t upperLimits, const size_t dimNum,
    const size_t pointNum, const size_t iterNum,
    const function<valFrm_t(const ptFrm_t &)> &evalFunc)
    : lowerLimits(lowerLimits), upperLimits(upperLimits), dimNum(dimNum),
      pointNum(pointNum), iterNum(iterNum), evaluateFunc(evalFunc) {
  ptHistory.reserve(iterNum);
  valHistory.reserve(iterNum);
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::exec() noexcept {
  randStart();
  for (size_t frmIdx = 0; frmIdx < iterNum; ++frmIdx) {
    // Exploitation using provided value function.
    valFrm_t frmVal = evaluateFunc(ptHistory[frmIdx]);
    valHistory.emplace_back(std::move(frmVal));
    // Exploration using current frame and points' values.
    if (frmIdx != iterNum - 1) [[likely]] {
      ptFrm_t nextFrm = updateFunc(ptHistory[frmIdx], valHistory[frmIdx]);
      ptHistory.emplace_back(std::move(nextFrm));
    }
  }
#ifndef NDEBUG
  debug_check();
#endif
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::randStart() noexcept {
  vector<pt_t> firstFrm;
  std::mt19937 rng(std::random_device{}());
  firstFrm.reserve(pointNum);
  for (size_t agtIdx = 0; agtIdx < pointNum; ++agtIdx) {
    pt_t agent(dimNum, (aType)0);
    for (size_t dimIdx = 0; dimIdx < dimNum; ++dimIdx) {
      // Compile time type inference and random data initialize.
      if constexpr (std::is_integral<aType>::value) {
        std::uniform_int_distribution<aType> dist(lowerLimits[dimIdx],
                                                  upperLimits[dimIdx]);
        agent[dimIdx] = dist(rng);
      } else if constexpr (std::is_floating_point<aType>::value) {
        std::uniform_real_distribution<aType> dist(lowerLimits[dimIdx],
                                                   upperLimits[dimIdx]);
        agent[dimIdx] = dist(rng);
      }
    }
    firstFrm.emplace_back(std::move(agent));
  }
  ptHistory.emplace_back(std::move(firstFrm));
}

template <typename aType, typename vType>
typename OptimizerBase<aType, vType>::ptFrm_t
OptimizerBase<aType, vType>::getCumulatePointsHistory() const {
  ptFrm_t result;
  result.reserve(pointNum * iterNum);
  for (const auto &frm : ptHistory) {
    for (const auto &pt : frm) {
      result.emplace_back(pt);
    }
  }
  return result;
}

template <typename aType, typename vType>
void OptimizerBase<aType, vType>::debug_check() noexcept {
  std::cout << ptHistory.size() << " " << valHistory.size() << std::endl;
  int nPoint = 0;
  std::cout << "All point in frame0\n";
  for (auto &&point : ptHistory[0]) {
    nPoint++;
    std::cout << "Point " << nPoint << " {";
    for (auto &&dim : point) {
      std::cout << dim << ", ";
    }
    std::cout << "} \n";
  }
  nPoint = 0;
  std::cout << "All point in frame3\n";
  for (auto &&point : ptHistory[3]) {
    nPoint++;
    std::cout << "Point " << nPoint << " {";
    for (auto &&dim : point) {
      std::cout << dim << ", ";
    }
    std::cout << "} \n";
  }
  nPoint = 0;
  std::cout << "Squashed point history:\n";
  auto squashed = getCumulatePointsHistory();
  for (auto &&point : squashed) {
    nPoint++;
    std::cout << "Point " << nPoint << " {";
    for (auto &&dim : point) {
      std::cout << dim << ", ";
    }
    std::cout << "} \n";
  }
}

} // namespace Optimizer
#endif