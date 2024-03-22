#ifndef CPU_ONLY_SCHED_HPP
#define CPU_ONLY_SCHED_HPP
#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;

class CPUOnlyScheduler {
public:
  Schedule schedule(const TaskGraph &gIn, OperatorManager &om) noexcept {
    Schedule s;
    s.order = gIn.topoSort();
    s.offloadRatio = std::vector<float>(s.order.size(), 0.0f);
    s.isCoarseGrain = true;
    return s;
  }
};

} // namespace Scheduler
} // namespace MetaPB
#endif
