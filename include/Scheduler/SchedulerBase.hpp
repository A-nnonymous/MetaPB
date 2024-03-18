#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorManager.hpp"
#include "utils/Stats.hpp"
#include "utils/typedef.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;
using Schedule = utils::Schedule;
using OperatorManager = Operator::OperatorManager;
using perfStats = utils::Stats;

class SchedulerBase {
public:
  SchedulerBase() = default;
  virtual Schedule schedule(const TaskGraph &gIn,
                            OperatorManager &om) noexcept = 0;
};

} // namespace Scheduler
} // namespace MetaPB
#endif