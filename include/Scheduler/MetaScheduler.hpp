#ifndef METASCHED_HPP
#define METASCHED_HPP
#include "Executor/HeteroComputePool.hpp"
#include "Executor/TaskGraph.hpp"
#include "Optimizer/OptimizerAOA.hpp"
#include "Optimizer/OptimizerBase.hpp"
#include "Optimizer/OptimizerRSA.hpp"
#include "Optimizer/OptimizerPSO.hpp"
#include "Scheduler/HEFTScheduler.hpp"
#include <omp.h>
#include <cstdlib>

namespace MetaPB {
namespace Scheduler {
using OperatorManager = Operator::OperatorManager;
using OptimizerBase = Optimizer::OptimizerBase<float,float>;
using HeteroComputePool = Executor::HeteroComputePool;
using execType = Executor::execType;

class MetaScheduler {
public:
  MetaScheduler(const double Alpha, const double Beta, const size_t OptIterMax, const TaskGraph &tg, OperatorManager &om)
  : Arg_Alpha(Alpha), Arg_Beta(Beta), OptIterMax(OptIterMax), tg(tg), om(om){
    dummyPool = (void**)malloc(sizeof(void**)); // legal place for dummy memory pool.
    // Use only order that provides from HEFT
    MetaPB::Scheduler::HEFTScheduler he(tg, om);
    Schedule heSchedule = he.schedule();
    Schedule result;
    HEFTorder = heSchedule.order;
  }
  Schedule schedule()noexcept;
  std::vector<float> evalSchedules(const vector<vector<float>> &ratioVecs);
private:
  std::vector<int> HEFTorder;
  const double Arg_Alpha;
  const double Arg_Beta;
  const TaskGraph& tg;
  const OperatorManager &om;
  void** dummyPool; // to make HeteroComputePool happy, although need't actually execute on this
  const size_t OptIterMax;
}; // class MetaScheduler

} // namespace Scheduler
} // namespace MetaPB
#endif
