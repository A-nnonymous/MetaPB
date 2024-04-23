#ifndef GREEDY_SCHED_HPP
#define GREEDY_SCHED_HPP
#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;

class GreedyScheduler {
public:
  Schedule schedule(const TaskGraph &gIn, OperatorManager &om) noexcept {
    Schedule s;
    s.order = gIn.topoSort();
    s.offloadRatio = std::vector<float>(s.order.size(), 0.0f);
    for (int i = 0; i < s.order.size(); i++) {
      if (om.deducePerfCPU(gIn.g[i].op, gIn.g[i].inputSize_MiB)
              .timeCost_Second >
          om.deducePerfDPU(gIn.g[i].op, gIn.g[i].inputSize_MiB)
              .timeCost_Second) {
        s.offloadRatio[i] = 1.0f;
      }
    }
    s.isCoarseGrain = true;
    return s;
  }
};

} // namespace Scheduler
} // namespace MetaPB
#endif
