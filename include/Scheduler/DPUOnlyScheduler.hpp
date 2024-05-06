#ifndef DPU_ONLY_SCHED_HPP
#define DPU_ONLY_SCHED_HPP
#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;
using OperatorManager = Operator::OperatorManager;

class DPUOnlyScheduler {
public:
  Schedule schedule(const TaskGraph &gIn, OperatorManager &om) noexcept {
    Schedule s;
    s.order = gIn.topoSort();
    s.offloadRatio = std::vector<float>(s.order.size(), 1.0f);
    s.offloadRatio[s.order.size() - 1] = 0.0f; // logic end;
    s.isAlwaysWrittingBack = true;
    return s;
  }
};

} // namespace Scheduler
} // namespace MetaPB
#endif
