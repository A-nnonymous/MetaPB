#ifndef OPT_NAIVE_HPP
#define OPT_NAIVE_HPP
#include "OptimizerBase.hpp"

namespace MetaPB{
namespace Optimizer {
template <typename aType, typename vType>
class OptimizerNaive : public OptimizerBase<aType, vType> {
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
  OptimizerNaive(const pt_t optima, const pt_t lowerLimits,
                 const pt_t upperLimits, const size_t dimNum,
                 const size_t pointNum, const size_t iterNum,
                 const function<valFrm_t(const ptFrm_t &)> &evalFunc)
      : OptimizerBase<aType, vType>(lowerLimits, upperLimits, dimNum, pointNum,
                                    iterNum, evalFunc),
        optima(optima) {}

private:
  pt_t optima;
  size_t iterCounter = 0;
  inline virtual void exploration() noexcept override;
  inline virtual void extraction() noexcept override;
};

} // namespace Optimizer
} // namespace MetaPB
#endif
#include "./implements/OptimizerNaive.cpp"
