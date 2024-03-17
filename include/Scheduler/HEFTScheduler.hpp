#ifndef HEFT_SCHED_HPP
#define HEFT_SCHED_HPP
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>
#include "Scheduler/SchedulerBase.hpp"

#define MAX_TIME 10000


namespace MetaPB {
namespace Scheduler {

struct ComputeUnitTraits {
  int computeCost;
  int earliestStartTime;
  int earliestFinishTime;
};

struct heftTask {
  float computeCostAvg; // avg. computation cost across all cuTraits
  float uprank;
  int scheduledProcessor;    // the processor on which the Task was finally
                             // scheduled
  int heft_actualStartTime;  // start time in final schedule
  int heft_actualFinishTime; // finish time in final schedule
  vector<ComputeUnitTraits> cuTraits;
  vector<int> predTasks; // indexes of all the predTasks of a particular node
};

  using std::cout, std::endl;
  using std::ifstream;
  using std::max;
  using std::vector, std::string;
  using Executor::TransferEdge;
  using Executor::TransferPropertyMap;
class HEFTScheduler : SchedulerBase{
private:
  vector<heftTask> tasks;
  const int taskCount ;     // number of tasks
  const int heteroCUCount = 2; // number of heterogenous computing units.
  vector<vector<int>>
      interTaskCommMatix; // inter-task communication cost matrix.

  void computationAvgCalculate();
  float rankCalculate(int node);
  vector<int> sortRank();
  int taskInsertion(int search_end, vector<vector<bool>> &processor_state,
                    int task_index, int processor_index, int search_start);
  void schedule(vector<int> rank_index_sorted);

public:
  virtual Schedule schedule(const TaskGraph& gIn, OperatorManager& om)noexcept override;
  HEFTScheduler(size_t taskCount=0) : SchedulerBase(),taskCount(taskCount) {}
};

/*
int main(int argc, char *argv[]) {
  HEFTScheduler scheduler;
  scheduler.initializeData("./one.txt");
  scheduler.mainSchedule();
  return 0;
}
*/
} // namespace Scheduler
} // namespace MetaPB

#endif
