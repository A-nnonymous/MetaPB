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

using std::cout, std::endl;
using std::ifstream;
using std::max;
using std::vector, std::string;

namespace MetaPB{
namespace Scheduler{

class HEFTScheduler {
private:
  vector<Task> tasks;
  int taskCount;     // number of tasks
  int heteroCUCount; // number of heterogenous computing units.
  vector<vector<int>> interTaskCommMatix; // inter-task communication cost matrix.

  void computationAvgCalculate();
  float rankCalculate(int node);
  vector<int> sortRank();
  int taskInsertion(int search_end, vector<vector<bool>> &processor_state,
                    int task_index, int processor_index, int search_start);
  void schedule(vector<int> rank_index_sorted);

public:

  HEFTScheduler() : taskCount(0), heteroCUCount(0) {}
  void initializeData(const string &filename);
  void mainSchedule();
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
