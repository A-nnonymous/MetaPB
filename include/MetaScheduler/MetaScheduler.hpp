// TODO:
// 1. Optimizer with:
//      a. A factory class that generates Optimizer instance
//      (Optimizer::generate(optType))
#ifndef METASCHED_HPP
#define METASCHED_HPP

#include <Executor/Task.hpp>
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace MetaPB {
using Executor::Task;
using Operator::OperatorTag;
using Optimizer::OptimizerTag;
namespace MetaScheduler {

class MetaScheduler {
public:
  typedef vector<double> Schedule;
  
  typedef struct PerfCost{
    double timeCost_second;
    double energyCost_joule;
  }perfCost;

  typedef struct ScheduleResult{
    Schedule schedule;
    perfCost estimateCost; 
  }ScheduleResult;

  static void cachedModelize(const vector<Task> &, const string)noexcept;


  MetaScheduler(const Task &, const double, const double,
                const Optimizer::OptimizerTag, const size_t, const bool);

  scheduleResult schedule(const Task&, double, double);
  scheduleResult scheduleAllCPU(const Task&);
  scheduleResult scheduleAllDPU(const Task&);

private:
  inline static const string defaultPerfModelPath = "/tmp/MetaPB_PerfModels";

  const double Arg_Alpha;
  const double Arg_Beta;
  const Optimizer::OptimizerTag optType;
  const size_t OptIterMax;
  const size_t batchSize;
  const bool isEarlyEndEnable = false;

  double perfEval(const Task&, const ScheduleVec &) const noexcept;
  void schedOptimize(const Task& const ScheduleVec &) noexcept;
}; // class MetaScheduler

} // namespace MetaScheduler
} // namespace MetaPB
#endif
