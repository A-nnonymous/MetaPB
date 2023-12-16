#ifndef OPT_AOA_HPP
#define OPT_AOA_HPP
#include "OptimizerBase.hpp"

namespace Optimizer {

template <typename aType, typename vType>
class OptimizerAOA : public OptimizerBase<aType, vType> {
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
  OptimizerAOA(const pt_t lowerLimits, const pt_t upperLimits,
               const size_t dimNum, const size_t pointNum, const size_t iterNum,
               const function<valFrm_t(const ptFrm_t &)> &evalFunc)
      : OptimizerBase<aType, vType>(lowerLimits, upperLimits, dimNum, pointNum,
                                    iterNum, evalFunc) {

  worstPtHist.reserve(iterNum);
  worstPtHist.reserve(iterNum);
                                    }

private:

  size_t iterCounter = 0; // Store iteration time related informations
  vType gWorstVal = std::numeric_limits<vType>::min(); // (Experimental reserve)
  pt_t gWorstPt;          // (Experimental reserve)
  pt_t fWorstPt;          // Frame worst point founded at last evaluation.
  vType fWorstVal = std::numeric_limits<vType>::min(); // (Experimental reserve)
  vector<pt_t> fWorstPtHist;   // History of worst point in all frame.
  vector<vType> worstValHist; // History of worst points' values in all frame.
  virtual ptFrm_t updateFunc() override;
};
} // namespace Optimizer
#endif
#include "./implements/OptimizerAOA.cpp"
