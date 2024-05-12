#ifndef GREEDY_SCHED_HPP
#define GREEDY_SCHED_HPP
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorRegistry.hpp"

namespace MetaPB {
namespace Scheduler {
using TaskGraph = Executor::TaskGraph;
using OperatorType = Operator::OperatorType;
using Operator::opType2Name;
using Operator::tag2Name;

class GreedyScheduler {
public:
  Schedule schedule(const TaskGraph &gIn, OperatorManager &om) noexcept {
    Schedule s;
    s.order = gIn.topoSort();
    s.offloadRatio = std::vector<float>(s.order.size(), 0.0f);
    for (int i = 0; i < s.offloadRatio.size(); i++) {
      if (gIn.g[i].opType != OperatorType::ComputeBound) {
        s.offloadRatio[i] = 1.0f;
      }
    }
    s.offloadRatio[s.offloadRatio.size() - 1] = 0.0f; // LOGIC_END

    s.isAlwaysWrittingBack = true;
    return s;
  }
};

} // namespace Scheduler
} // namespace MetaPB
#endif
