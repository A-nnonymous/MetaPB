// TODO:
// 1. Optimizer with:
//      a. A factory class that generates Optimizer instance
//      (Optimizer::generate(optType))
#ifndef METASCHED_HPP
#define METASCHED_HPP

#include "Executor/Task.hpp"
#include "Optimizer/OptimizerManager.hpp"
#include <string>
#include <vector>

using std::string;
using std::vector;

namespace MetaPB {
namespace Scheduler {

class MetaScheduler {
public:
  using OptimizerTag = Optimizer::OptimizerTag;

  typedef struct perfCost {
    double timeCost_second;
    double energyCost_joule;
  } perfCost;

  typedef vector<double> Schedule;

  typedef struct ScheduleResult {
    Schedule schedule;
    perfCost estimateCost;
  } ScheduleResult;

public:
  static void cachedModelize(const vector<Task> &, const string) noexcept;
  MetaScheduler(const Task &, const double, const double, const OptimizerTag,
                const size_t, const bool);
  ScheduleResult schedule(const Task &, double, double);
  ScheduleResult scheduleAllCPU(const Task &);
  ScheduleResult scheduleAllDPU(const Task &);

private:
  inline static const string defaultPerfModelPath = "/tmp/MetaPB_PerfModels";

  const double Arg_Alpha;
  const double Arg_Beta;
  const OptimizerTag optType;
  const size_t OptIterMax;
  const size_t batchSize_MiB;
  const bool isEarlyEndEnable = false;

  double perfEval(const Task &, const Schedule &) const noexcept;
  void schedOptimize(const Task &, const Schedule &) noexcept;
}; // class MetaScheduler

} // namespace Scheduler
} // namespace MetaPB
#endif
