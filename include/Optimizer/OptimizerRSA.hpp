#ifndef OPT_RSA_HPP
#define OPT_RSA_HPP
#include "OptimizerBase.hpp"

namespace MetaPB {
namespace Optimizer {

template <typename aType, typename vType>
class OptimizerRSA : public OptimizerBase<aType, vType> {
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

  OptimizerRSA(const pt_t lowerLimits, const pt_t upperLimits,
               const size_t dimNum, const size_t pointNum, const size_t iterNum,
               const function<valFrm_t(const ptFrm_t &)> &evalFunc)
      : OptimizerBase<aType, vType>(lowerLimits, upperLimits, dimNum, pointNum,
                                    iterNum, evalFunc) {}

private:
  const double arg_ALPHA = 0.1;
  const double arg_BETA = 0.005;

  std::uniform_int_distribution<size_t> choice{0, this->pointNum - 1};
  std::uniform_int_distribution<int> evoScaleDist{-2, 2};

  size_t currIterIdx = 0;
  double evoSense;

  inline virtual void extraction() noexcept override;
  inline virtual void exploration() noexcept override;
};

} // namespace Optimizer
} // namespace MetaPB
#endif
#include "./implements/OptimizerRSA.cpp"
