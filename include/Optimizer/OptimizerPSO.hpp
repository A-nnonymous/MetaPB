#ifndef OPT_PSO_HPP
#define OPT_PSO_HPP
#include "OptimizerBase.hpp"

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
  typedef vector<vType> valFrm_t;
  typedef vector<pt_t> ptFrm_t;
  typedef vector<ptFrm_t> ptHist_t;
  typedef vector<valFrm_t> valHist_t;

  OptimizerPSO(const pt_t vMax, const double omega,
               const double dt, const double ego,
               const pt_t lowerLimits, const pt_t upperLimits,
               const size_t dimNum, const size_t pointNum, const size_t iterNum,
               const function<valFrm_t(const ptFrm_t &)> &evalFunc);

private:
  size_t iterCounter = 0;
  vector<vector<double>> velocities; // velocities of all point in curr frame.
  virtual ptFrm_t updateFunc() override;
  inline virtual void dataLogging(const valFrm_t &frmVal) noexcept override;
};
} // namespace Optimizer
#endif
#include "./implements/OptimizerPSO.cpp"
