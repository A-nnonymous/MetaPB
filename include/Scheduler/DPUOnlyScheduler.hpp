#ifndef DPU_ONLY_SCHED_HPP
#define DPU_ONLY_SCHED_HPP
#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;

class DPUOnlyScheduler {
public:
  Schedule schedule(const TaskGraph &gIn, OperatorManager &om) noexcept {
    Schedule s;
    s.order = gIn.topoSort();
    s.offloadRatio = std::vector<float>(s.order.size(), 1.0f);
    s.isCoarseGrain = true;
    return s;
  }
};

} // namespace Scheduler
} // namespace MetaPB
#endif
