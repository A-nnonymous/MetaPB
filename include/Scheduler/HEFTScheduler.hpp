#ifndef HEFT_SCHED_HPP
#define HEFT_SCHED_HPP
#include "Scheduler/SchedulerBase.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

namespace MetaPB {
namespace Scheduler {
using Executor::TaskNode;
using Executor::TransferProperties;
using Executor::TaskProperties;
using Executor::Graph;
struct Processor {
    int id; // Processor identifier
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

    TaskSchedulingInfo() : upwardRank(-1), assignedProcessor(-1), startTime(0), endTime(0) {}
};

class HEFTScheduler {
public:
  HEFTScheduler(TaskGraph &g, OperatorManager &om)
      : graw(g.g), om(om), processors({{0, 0.0f}, {1, 0.0f}}) {}

  Schedule schedule();

private:
  Graph &graw;
  OperatorManager &om;
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
