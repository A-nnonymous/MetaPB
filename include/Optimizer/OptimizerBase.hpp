#ifndef OPT_BASE_HPP
#define OPT_BASE_HPP
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <vector>

using std::function;
using std::map;
using std::vector;

namespace Optimizer {
/// @brief The base class of Metaheuristic optimizer is constructed on three
/// levels of strictly nested data structure, namely, point, frame and history,
/// performing cost **Minimize** optimization, yield a minimum cost point as
/// result.
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

  /// @brief Main execute logic of all optimizer
  void exec() noexcept;

  //----------------- Members getters -------------------
  /// @brief Get point argument history shaped in (iterNum, pointNum, dimNum)
  /// @return A 3-D aType vector.
  inline ptHist_t getPointsHistory() const noexcept { return ptHistory; }

  /// @brief Get point value history shaped in (iterNum, pointNum)
  /// @return A 2-D vType vector.
  inline valHist_t getValueHistory() const noexcept { return valHistory; }

  /// @brief Get the best-at-a-time points of all search frame
  /// @return History of global optima point till that frame, a 2-D aType vector
  /// shaped in (iterNum, dimNum)
  inline vector<pt_t> getGlobalConvergePointtVec() const noexcept {
    return gBestPtHist;
  }

  /// @brief Flatten and cumulate all points arguments history and output
  /// @return A 2-dimensional aType vector shaped in (pointNum * iterNum,
  /// dimNum)
  ptFrm_t getCumulatePointsHistory() const;

  /// @brief Get the best-at-a-time points' value of all search frame
  /// @return History of global optima point till that frame, a 1-D aType vector
  /// shaped in (iterNum)
  inline vector<vType> getGlobalConvergeValueVec() const noexcept {
    return gBestValHist;
  }

  /// @brief Get the best result point after all optimization
  /// @return Global optima point, a 1-D aType vector shaped in (dimNum)
  inline pt_t getGlobalOptimaPoint() const noexcept { return gBestPt; }

  /// @brief Get the value of global optima
  /// @return A vType scalar
  inline vType getGlobalOptimaValue() const noexcept { return gBestVal; }

protected:
  /// @brief Wrapper of evaluateFunc, maintain the back of valHistory.
  inline void exploitation() noexcept;

  /// @brief Logging the converge related information.
  inline void convergeLogging() noexcept;

  /// @brief Derived optimizer's specific feature gathering function using all
  /// available data.
  inline virtual void extraction() noexcept = 0;

  /// @brief This function(virtual) is the core of all metaheuristic algorithms.
  /// The datatype adapting is expected to be handled inside this funciton
  /// @return A new frame of argument vectors that await evaluating.
  inline virtual void exploration() noexcept = 0;

  /// @brief Debug function used in the end of exec(), disabled in NDEBUG mode.
  void debug_check() noexcept;

  //-------------- constants--------------
  const pt_t lowerLimits;
  const pt_t upperLimits;
  const size_t dimNum;
  const size_t pointNum;
  const size_t iterNum;
  /// @brief This function is used to provide evaluation of given argument
  /// vector. Must be provided explicitly by user.
  const function<valFrm_t(const ptFrm_t &)> &evaluateFunc;

  // -------------- variables -------------
  // Custom comparison function for vectors
  struct ptCompare {
    bool operator()(const pt_t &a, const pt_t &b) const {
      // Assuming both vectors have the same size
      return std::lexicographical_compare(a.begin(), a.end(), b.begin(),
                                          b.end());
    }
  };
  map<pt_t, vType, ptCompare> pt2Val; // Cumulative point to value mapping.
  // ------- Results --------
  pt_t gBestPt; // Global optima founded at last.
  vType gBestVal = std::numeric_limits<vType>::max(); // Value of global optima.
  // ------- Histories -------
  ptHist_t ptHistory;         // History of all points' data vector.
  valHist_t valHistory;       // History of all points' value.
  vector<pt_t> gBestPtHist;   // History of optimas till given frame.
  vector<vType> gBestValHist; // History of optimas' values till given frame.
                              // Can be used to draw converging line.
};                            // namespace OptimizerBase
} // namespace Optimizer

#endif
// Tail include of template method implementaions
#include "./implements/OptimizerBase.cpp"
