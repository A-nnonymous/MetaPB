#ifndef HEFT_SCHED_HPP
#define HEFT_SCHED_HPP
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorManager.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace MetaPB {
namespace Scheduler {
using Executor::Graph;
using Executor::TaskNode;
using Executor::TaskProperties;
using Executor::TransferProperties;
using Executor::TaskGraph;
using Operator::OperatorManager;
struct Processor {
  int id;                   // Processor identifier
  double nextAvailableTime; // Time when the processor is ready for a new task
};

// Structure to hold scheduling information for tasks
struct TaskSchedulingInfo {
  double computationCostCPU;
  double computationCostDPU;
  double upwardRank;
  int assignedProcessor;
  double startTime;
  double endTime;

  TaskSchedulingInfo()
      : upwardRank(-1), assignedProcessor(-1), startTime(0), endTime(0) {}
};

class HEFTScheduler {
public:
  HEFTScheduler(const TaskGraph &g, const OperatorManager &om)
      : graw(g.g), om(om), processors({{0, 0.0f}, {1, 0.0f}}) {}

  Schedule schedule();

private:
  const Graph &graw;
  const OperatorManager &om;
  std::vector<Processor> processors;

  double calculateUpwardRank(
      TaskNode tn,
      std::unordered_map<TaskNode, TaskSchedulingInfo> &schedulingInfo);

  void scheduleTask(
      TaskNode tn,
      std::unordered_map<TaskNode, TaskSchedulingInfo> &schedulingInfo);
};
} // namespace Scheduler
} // namespace MetaPB

#endif
