// TODO:
// 1. Executor Task with:
//      b. Fixed argument range (for research only)
//      c. Fixed execution order(for UPMEM cannot run multi-kernel simutaneously
//      in single DPU)
//      d. (based on c) A one-to-one mapping of scheduleVec.
//      e. set of operator Tag (task.opTagSet) that contain operator and
//      auxillary information. f. deduceTime and deduceEnergy method takes
//      schedule and return double g. random schedule Generator that take
//      batchsize and output 2dVec.
// 2. Operator with:
//      a. A factory class that perform tagged Operator instance generating and
//      perf model training.(Operator::modelize(opTag))
// 3. Optimizer with:
//      a. A factory class that generates Optimizer instance
//      (Optimizer::generate(optType))
#ifndef METASCHED_HPP
#define METASCHED_HPP

#include <Executor/Task.hpp>
#include <string>
#include <unordered_map>
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
  typedef vector<Schedule> ScheduleVec;
  MetaScheduler(const Task &, const double, const double,
                const Optimizer::OptimizerTag, const size_t, const bool);

  void setTask(const Task &) noexcept;

  Schedule scheduleGen(string perfModelPath) const noexcept;

private:
  inline static const string defaultPerfModelPath = "/tmp/MetaPB_PerfModels";

  const double Arg_Alpha;
  const double Arg_Beta;
  const Optimizer::OptimizerTag optType;
  const size_t OptIterMax;
  const size_t batchSize;
  const bool isEarlyEndEnable = false;

  Task &task;
  string perfModelPath;
  void modelizeIfNotLoaded(const string perfModelPath) noexcept;
  double perfEval(const ScheduleVec &) const noexcept;
  void schedOptimize(const ScheduleVec &) noexcept;
}; // class MetaScheduler

} // namespace MetaScheduler
} // namespace MetaPB
#endif
