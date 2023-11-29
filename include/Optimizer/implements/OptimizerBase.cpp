#ifndef OPT_BASE_SRC
#define OPT_BASE_SRC
#include "../OptimizerBase.hpp"
namespace Optimizer {


template <typename aType, typename vType>
OptimizerBase<aType, vType>::OptimizerBase(const pt_t lowerLimits,
                                           const pt_t upperLimits,
                                           const size_t dimNum,
                                           const size_t agentNum,
                                           const size_t iterNum,
                                           const function<valFrm_t(const ptFrm_t&)> &evalFunc)
    : lowerLimits(lowerLimits), upperLimits(upperLimits), dimNum(dimNum),
      agentNum(agentNum), iterNum(iterNum), evaluateFunc(evalFunc) {
  ptHistory.reserve(iterNum);
  valHistory.reserve(iterNum);
}

/// @brief Main execute routine.
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

/// @brief Random initiate first search frame.
template <typename aType, typename vType>
void OptimizerBase<aType, vType>::randStart() noexcept {
  vector<pt_t> firstFrm;
  std::mt19937 rng(std::random_device{}());
  firstFrm.reserve(agentNum);
  for (size_t agtIdx = 0; agtIdx < agentNum; ++agtIdx) {
    pt_t agent(dimNum, (aType)0);
    for (size_t dimIdx = 0; dimIdx < dimNum; ++dimIdx) {
      std::uniform_real_distribution<double> uni(lowerLimits[dimIdx],
                                                 upperLimits[dimIdx]);
      agent[dimIdx] = (aType)uni(rng);
    }
    firstFrm.emplace_back(std::move(agent));
  }
  ptHistory.emplace_back(std::move(firstFrm));
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
  std::cout << "All point in frame3\n";
  for (auto &&point : ptHistory[3]) {
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