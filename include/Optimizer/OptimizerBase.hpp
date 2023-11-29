#ifndef OPT_BASE_HPP
#define OPT_BASE_HPP
#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

using std::function;
using std::vector;

namespace Optimizer {

/// @brief The base class of Metaheuristic optimizer is constructed on three
/// levels of strictly nested data structure, namely, point, frame and history
/// @tparam aType Argument type of each searching point
/// @tparam vType Value type of each searching point
template <typename aType, typename vType> class OptimizerBase {
public:
  /*
    pt_t is a point type that stores multi-dimensional generic type args
    (optimize target). ptFrm_t is a frame type that stores points' args in a
    same iteration. valFrm_t is a frame type that stores value of each point in
    correlated ptFrm_t. ptHist_t is a history type of all points' arg in all
    searching frames. valHist_t is a history type of all points' value in all
    searching frames.
  */
  typedef vector<aType> pt_t;
  typedef vector<vType> valFrm_t;
  typedef vector<pt_t> ptFrm_t;
  typedef vector<ptFrm_t> ptHist_t;
  typedef vector<valFrm_t> valHist_t;

  OptimizerBase(const pt_t lowerLimits, const pt_t upperLimits,
                const size_t dimNum, const size_t agentNum,
                const size_t iterNum);

  void exec() noexcept;

  inline valHist_t getValueHistory() const noexcept { return valHistory; }
  inline ptHist_t getPointsHistory() const noexcept { return ptHistory; }
  inline valFrm_t getValueFrame(size_t frmIdx) const noexcept {
    return valHistory[frmIdx];
  }
  inline ptFrm_t getPointsFrame(size_t frmIdx) const noexcept {
    return ptHistory[frmIdx];
  }

private:
  virtual valFrm_t evaluateFunc(const ptFrm_t &) = 0;
  virtual ptFrm_t updateFunc(const ptFrm_t &, const valFrm_t &) = 0;

  void randStart() noexcept;
  void debug_check() noexcept;

  ptHist_t ptHistory;
  valHist_t valHistory;
  const pt_t lowerLimits;
  const pt_t upperLimits;
  const size_t dimNum;
  const size_t agentNum;
  const size_t iterNum;
}; // class OptimizerBase.
} // namespace Optimizer
#endif
#include "OptimizerBase.cpp"