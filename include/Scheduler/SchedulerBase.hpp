#ifndef SCHED_BASE_HPP
#define SCHED_BASE_HPP

#include <vector>

using std::vector;
namespace MetaPB{
namespace Scheduler{

struct ComputeUnitTraits {
  int computeCost;
  int earliestStartTime;
  int earliestFinishTime;
};

struct Task {
  float computeCostAvg; // avg. computation cost across all cuTraits
  float uprank;
  int scheduledProcessor;    // the processor on which the Task was finally
                             // scheduled
  int heft_actualStartTime;  // start time in final schedule
  int heft_actualFinishTime; // finish time in final schedule
  vector<ComputeUnitTraits> cuTraits;
  vector<int> predTasks; // indexes of all the predTasks of a particular node
};

class SchedulerBase{
public:
  typedef vector<double> Schedule;
  virtual Schedule doSchedule(const Workload&) = 0;
   
};

} // namespace MetaPB
} // namespace Scheduler
#endif
