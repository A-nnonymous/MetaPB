#ifndef OPT_PSO_HPP
#define OPT_PSO_HPP
#include "OptimizerBase.hpp"

namespace MetaPB {
namespace Optimizer {
template <typename aType, typename vType>
class OptimizerPSO : public OptimizerBase<aType, vType> {
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
  typedef vector<pt_t> ptFrm_t;
  typedef vector<vType> valFrm_t;

  OptimizerPSO(const double vMax, const double omega, const double dt,
               const double ego, const pt_t lowerLimits, const pt_t upperLimits,
               const size_t dimNum, const size_t pointNum, const size_t iterNum,
               const function<valFrm_t(const ptFrm_t &)> &evalFunc);

private:
  // Constants
  const double vMax;  // Max velocity ratio ralated to the bound range.
  const double omega; // Inertia factor.
  const double dt;    // Timestep factor, control the granular of searching.
  const double ego; // Stubborness factor, regulate self history consideration.

  vector<pt_t> pBestPts;             // Personal best memories of all points.
  vector<vType> pBestVals;           // Personal best values.
  vector<vector<double>> velocities; // velocities of all points in curr frame.

  inline virtual void extraction() noexcept override;
  inline virtual void exploration() noexcept override;
};

} // namespace Optimizer
} // namespace MetaPB
#endif
#include "./implements/OptimizerPSO.cpp"
