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
                const size_t dimNum, const size_t pointNum,
                const size_t iterNum,
                const function<valFrm_t(const ptFrm_t &)> &evalFunc);

  void exec() noexcept;

  //Members getters.
  /// @brief Get point value history shaped in (iterNum, pointNum)
  /// @return A 2-dimensional vType vector.
  inline valHist_t getValueHistory() const noexcept { return valHistory;}

  /// @brief Get point argument history shaped in (iterNum, pointNum, dimNum)
  /// @return A 3-dimensional aType vector.
  inline ptHist_t getPointsHistory() const noexcept { return ptHistory;}

  /// @brief Get specific frame of point value shaped in (pointNum)
  /// @param frmIdx Input frame index.
  /// @return A 1-dimensional vType vector.
  inline valFrm_t getValueFrame(size_t frmIdx) const noexcept { return valHistory[frmIdx];}

  /// @brief Get specific frame of point arguments shaped in (pointNum)
  /// @param frmIdx Input frame index.
  /// @return A 1-dimensional aType vector.
  inline ptFrm_t getPointsFrame(size_t frmIdx) const noexcept { return ptHistory[frmIdx];}

  /// @brief Flatten and cumulate all points arguments history and output
  /// @return A 2-dimensional aType vector shaped in (pointNum * iterNum, dimNum)
  ptFrm_t getCumulatePointsHistory() const;

protected:
  /// @brief This function(virtual) is the core of all metaheuristic algorithms
  /// @return A new frame of argument vectors that await evaluating.
  virtual ptFrm_t updateFunc(const ptFrm_t &, const valFrm_t &) = 0;

  /// @brief Random initialize the first search frame.
  void randStart() noexcept;

  /// @brief Debug function used in the end of exec(), disabled in NDEBUG mode.
  void debug_check() noexcept;

  ptHist_t ptHistory;
  valHist_t valHistory;
  const pt_t lowerLimits;
  const pt_t upperLimits;
  const size_t dimNum;
  const size_t pointNum;
  const size_t iterNum;
  /// @brief This function is used to provide evaluation of given argument
  /// vector. Must be provided explicitly by user.
  const function<valFrm_t(const ptFrm_t &)> &evaluateFunc;
}; // class OptimizerBase.
} // namespace Optimizer
#endif
// Tail include of template method implementaions
#include "./implements/OptimizerBase.cpp"