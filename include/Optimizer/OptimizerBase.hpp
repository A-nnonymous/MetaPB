#ifndef OPT_BASE_HPP
#define OPT_BASE_HPP
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

using std::function;
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

  void exec() noexcept;

  //----------------- Members getters -------------------
  /// @brief Get point argument history shaped in (iterNum, pointNum, dimNum)
  /// @return A 3-D aType vector.
  inline ptHist_t getPointsHistory() const noexcept { return ptHistory; }

  /// @brief Flatten and cumulate all points arguments history and output
  /// @return A 2-dimensional aType vector shaped in (pointNum * iterNum,
  /// dimNum)
  ptFrm_t getCumulatePointsHistory() const;

  /// @brief Get the local best points of all search frame
  /// @return History of global optima point, a 2-D aType vector shaped in
  /// (iterNum, dimNum)
  inline vector<pt_t> getGlobalOptimaPointsHistory() const noexcept {
    return bestPtHist;
  }

  /// @brief Get point value history shaped in (iterNum, pointNum)
  /// @return A 2-D vType vector.
  inline valHist_t getValueHistory() const noexcept { return valHistory; }

  /// @brief Get the history of values of all searched optimas
  /// @return A 1-D vType vector shaped in (iterNum)
  inline vector<vType> getGlobalOptimaValueHistory() const noexcept {
    return bestValHist;
  }

  /// @brief Get specific frame of point value shaped in (pointNum)
  /// @param frmIdx Input frame index.
  /// @return A 1-D vType vector.
  inline valFrm_t getValueFrame(size_t frmIdx) const noexcept {
    return valHistory[frmIdx];
  }

  /// @brief Get specific frame of point arguments shaped in (pointNum)
  /// @param frmIdx Input frame index.
  /// @return A 1-D aType vector.
  inline ptFrm_t getPointsFrame(size_t frmIdx) const noexcept {
    return ptHistory[frmIdx];
  }

  /// @brief Get the best result point after all optimization
  /// @return Global optima point, a 1-D aType vector shaped in (dimNum)
  inline pt_t getGlobalOptimaPoint() const noexcept { return gBestPt; }

  /// @brief Get the value of global optima
  /// @return A vType scalar
  inline vType getGlobalOptimaValue() const noexcept { return gBestVal; }

protected:
  
  /// @brief Logging evaluated frame's data into histories.
  inline virtual void dataLogging(const valFrm_t &frmVal) noexcept;

  /// @brief This function(virtual) is the core of all metaheuristic algorithms.
  /// The datatype adapting is ought to be handled inside this funciton
  /// @return A new frame of argument vectors that await evaluating.
  virtual ptFrm_t updateFunc() = 0;

  /// @brief Debug function used in the end of exec(), disabled in NDEBUG mode.
  void debug_check() noexcept;

  //-------------- constants--------------
  const pt_t lowerLimits;
  const pt_t upperLimits;
  const size_t dimNum;
  const size_t pointNum;
  const size_t iterNum;

  // -------------- variables -------------
  /// @brief This function is used to provide evaluation of given argument
  /// vector. Must be provided explicitly by user.
  const function<valFrm_t(const ptFrm_t &)> &evaluateFunc;
  size_t iterCounter = 0; // Store iteration related informations
  // ----- Global scale vars-----
  pt_t gBestPt;           // Global optima founded at last.
  pt_t gWorstPt;          // (Experimental reserve)
  vType gBestVal = std::numeric_limits<vType>::max(); // Value of global optima.
  vType gWorstVal = std::numeric_limits<vType>::min(); // (Experimental reserve)

  // ----- Frame scale vars-----
  pt_t fBestPt;           // Frame optima founded at last evaluation.
  pt_t fWorstPt;          // Frame worst point founded at last evaluation.
  vType fBestVal = std::numeric_limits<vType>::max(); // Value of frame optima.
  vType fWorstVal = std::numeric_limits<vType>::min(); // (Experimental reserve)

  // --- Histories ---
  vector<pt_t> bestPtHist;    // History of optimas point in all frame.
  vector<pt_t> worstPtHist;   // History of worst point in all frame.
  vector<vType> bestValHist;  // History of optimas' values in all frame.
  vector<vType> worstValHist; // History of worst points' values in all frame.
  ptHist_t ptHistory;         // History of all points' data vector.
  valHist_t valHistory;       // History of all points' value.
}; // class OptimizerBase.
} // namespace Optimizer
#endif
// Tail include of template method implementaions
#include "./implements/OptimizerBase.cpp"
