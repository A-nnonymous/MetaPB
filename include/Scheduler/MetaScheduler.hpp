#ifndef METASCHED_HPP
#define METASCHED_HPP
#include "Executor/HeteroComputePool.hpp"
#include "Executor/TaskGraph.hpp"
#include "Optimizer/OptimizerAOA.hpp"
#include "Optimizer/OptimizerBase.hpp"
#include "Optimizer/OptimizerPSO.hpp"
#include "Optimizer/OptimizerRSA.hpp"
#include "Scheduler/HEFTScheduler.hpp"
#include <cstdlib>
#include <omp.h>

namespace MetaPB {
namespace Scheduler {
using OperatorManager = Operator::OperatorManager;
using OptimizerBase = Optimizer::OptimizerBase<float, float>;
using HeteroComputePool = Executor::HeteroComputePool;
using execType = Executor::execType;

using  pt_t       =    Optimizer::OptimizerBase<float,float>::pt_t;
using  valFrm_t   =    Optimizer::OptimizerBase<float,float>::valFrm_t;
using  ptFrm_t    =    Optimizer::OptimizerBase<float,float>::ptFrm_t;
using  ptHist_t   =    Optimizer::OptimizerBase<float,float>::ptHist_t;
using  valHist_t  =    Optimizer::OptimizerBase<float,float>::valHist_t;

class MetaScheduler {
public:
  typedef struct {
    std::string optimizerName;
    ptFrm_t totalPtFrm; // flatten all point to single frame
    ptFrm_t convergePtFrm; // flatten frame best point to signel frame
    valHist_t totalValHist;
    valFrm_t convergeValFrm; // flatten frame best value to single frame.
  }OptimizerInfos;
  MetaScheduler(const double Alpha, const double Beta, const size_t OptIterMax,
                const TaskGraph &tg, OperatorManager &om)
      : Arg_Alpha(Alpha), Arg_Beta(Beta), OptIterMax(OptIterMax), tg(tg),
        om(om) {
    dummyPool =
        (void **)malloc(sizeof(void **)); // legal place for dummy memory pool.
    // Use only order that provides from HEFT
    MetaPB::Scheduler::HEFTScheduler he(tg, om);
    Schedule heSchedule = he.schedule();
    Schedule result;
    HEFTorder = heSchedule.order;
  }
  Schedule schedule() noexcept;
  std::vector<float> evalSchedules(const vector<vector<float>> &ratioVecs);
  OptimizerInfos getOptInfo()const noexcept{
    return optInfo;
  }

private:
  std::vector<int> HEFTorder;
  const double Arg_Alpha;
  const double Arg_Beta;
  const TaskGraph &tg;
  const OperatorManager &om;
  void **dummyPool; // to make HeteroComputePool happy, although need't actually
                    // execute on this
  const size_t OptIterMax;

  // --------- showoff use ----------
  OptimizerInfos optInfo;
  // --------- showoff use ----------
}; // class MetaScheduler

} // namespace Scheduler
} // namespace MetaPB
#endif
