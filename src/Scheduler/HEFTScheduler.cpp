#include "Scheduler/HEFTScheduler.hpp"

using std::cout, std::endl;
using std::ifstream;
using std::max;
using std::vector, std::string;

namespace MetaPB {
namespace Scheduler {

void HEFTScheduler::computationAvgCalculate() {
  for (int i = 0; i < taskCount; i++) {
    tasks[i].computeCostAvg = 0;
    for (int cuIdx = 0; cuIdx < heteroCUCount; cuIdx++) {
      tasks[i].computeCostAvg += tasks[i].cuTraits[cuIdx].computeCost;
    }
    tasks[i].computeCostAvg /= heteroCUCount;
  }
}

float HEFTScheduler::rankCalculate(int node) {
  float sub_rank = 0; // rank of the dependent task
  float temp_max = 0; // temp_max = maximum of(cost of communication between
                      // tasks + rank(i))

  for (int i = 0; i < taskCount; i++) { // for every node
    if (interTaskCommMatix[node][i] >
        0) {                       // that has an incoming edge from main node
      sub_rank = rankCalculate(i); // calculate its upper rank
      if (sub_rank + interTaskCommMatix[node][i] >
          temp_max) { // if the upper rank is max among others
        temp_max =
            sub_rank + interTaskCommMatix[node][i]; // set this as the max
      }
    }
  }
  return temp_max + tasks[node].computeCostAvg; // rank of the main node
}

vector<int> HEFTScheduler::sortRank() {
  vector<int> index(taskCount);
  iota(index.begin(), index.end(), 0);
  sort(index.begin(), index.end(), [this](int a, int b) -> bool {
    return tasks[a].uprank > tasks[b].uprank;
  });
  return index;
}

int HEFTScheduler::taskInsertion(int search_end,
                                 vector<vector<bool>> &processor_state,
                                 int task_index, int processor_index,
                                 int search_start) {
  int t = tasks[task_index].cuTraits[processor_index].computeCost;
  int counter = 0;
  for (int i = search_start; i < search_end; i++) {
    if (processor_state[processor_index][i] == false) {
      counter++;
    } else {
      counter = 0;
    }
    if (counter ==
        t) { // as soon as a compatible gap is found, return its location
      return i - t + 1;
    }
  }
  return max(
      search_end,
      search_start); // if no gaps were found, resume the scheduling as normal
}

void HEFTScheduler::schedule(vector<int> rank_index_sorted) {
  vector<int> processor_free(heteroCUCount, 0);
  vector<vector<bool>> processor_state(heteroCUCount,
                                       vector<bool>(MAX_TIME, false));

  for (int i = 0; i < taskCount; i++) {
    int ii = rank_index_sorted[i];
    vector<int> est_values(heteroCUCount, 0);
    vector<int> eft_values(heteroCUCount, 0);
    vector<int> parent_task_delay(heteroCUCount, 0);

    for (int j = 0; j < tasks[ii].predTasks.size(); j++) {
      int parent_index = tasks[ii].predTasks[j];
      for (int k = 0; k < heteroCUCount; k++) {
        int communication_time = (k != tasks[parent_index].scheduledProcessor)
                                     ? interTaskCommMatix[parent_index][ii]
                                     : 0;
        int ready_time =
            tasks[parent_index].heft_actualFinishTime + communication_time;
        parent_task_delay[k] = max(parent_task_delay[k], ready_time);
      }
    }

    for (int j = 0; j < heteroCUCount; j++) {
      est_values[j] = taskInsertion(processor_free[j], processor_state, ii, j,
                                    parent_task_delay[j]);
      eft_values[j] = est_values[j] + tasks[ii].cuTraits[j].computeCost;
    }

    int best_processor =
        min_element(eft_values.begin(), eft_values.end()) - eft_values.begin();
    tasks[ii].heft_actualStartTime = est_values[best_processor];
    tasks[ii].heft_actualFinishTime = eft_values[best_processor];
    tasks[ii].scheduledProcessor = best_processor;
    processor_free[best_processor] = tasks[ii].heft_actualFinishTime;

    for (int j = tasks[ii].heft_actualStartTime;
         j < tasks[ii].heft_actualFinishTime; j++) {
      processor_state[best_processor][j] = true;
    }
  }
}

Schedule HEFTScheduler::schedule(const TaskGraph& gIn, OperatorManager& om) noexcept{
  Schedule result; 
  // initialize compute & communication matrix.
  size_t taskCount= boost::num_vertices(gIn.g);

  tasks.resize(taskCount);
  for (int i = 0; i < taskCount; i++) {
    tasks[i].cuTraits.resize(heteroCUCount);
  }

  
  std::cout <<"Task time consume matrix:  CPU-DPU "<<std::endl;
  for (int j = 0; j < taskCount; j++) {
    std::cout << "In "<< gIn.g[j].inputSize_MiB << "MiB, Operator: "<< MetaPB::Operator::tag2Name.at(gIn.g[j].op)<<std::endl;
    tasks[j].cuTraits[0].computeCost = 10000 * om.deducePerfCPU(gIn.g[j].op, gIn.g[j].inputSize_MiB).timeCost_Second;
    tasks[j].cuTraits[1].computeCost = 10000 * om.deducePerfDPU(gIn.g[j].op, gIn.g[j].inputSize_MiB).timeCost_Second;
    std::cout << "CPU: "<< tasks[j].cuTraits[0].computeCost << " DPU: "<< tasks[j].cuTraits[0].computeCost<<std::endl;
  }

  interTaskCommMatix.resize(taskCount, vector<int>(taskCount));
  auto edgePropertyMap = get(boost::edge_bundle, gIn.g);
  std::cout <<"Interconnection matrix:   "<<std::endl;
  for (int i = 0; i < taskCount; i++) {
    for (int j = 0; j < taskCount; j++) {
      std::pair<TransferEdge, bool> edgeCheck = boost::edge(i, j, gIn.g);
      if (edgeCheck.second) { // exist edge
        const auto& edgeProps = edgePropertyMap[edgeCheck.first];
        interTaskCommMatix[i][j] = 1000 * om.deducePerfCPU(OperatorTag::MAP,
                                                    edgeProps.dataSize_Ratio).timeCost_Second;
        tasks[j].predTasks.push_back(i);
      } else {
        interTaskCommMatix[i][j] = 0;
      }
      std::cout << interTaskCommMatix[i][j] <<", "; 
    }
    std::cout <<std::endl;
  }
  // main algorithm
  computationAvgCalculate();

  for (int i = 0; i < taskCount; i++) {
    tasks[i].uprank = rankCalculate(i);
  }

  vector<int> rank_index_sorted = sortRank();
  schedule(rank_index_sorted);

  // Output the schedule...
  for (int i = 0; i < taskCount; i++) {
    cout << "Task " << i + 1 << " is executed on processor "
         << tasks[i].scheduledProcessor + 1 << " from time "
         << tasks[i].heft_actualStartTime << " to "
         << tasks[i].heft_actualFinishTime << endl;
  }
  std::sort(tasks.begin(), tasks.end(), [](const heftTask& a, const heftTask& b) {
    return a.heft_actualStartTime < b.heft_actualStartTime;
  });

  for (const auto& task : tasks) {
    result.order.push_back(task.scheduledProcessor); 
    result.offloadRatio.push_back(static_cast<float>(task.scheduledProcessor)); 
  }
  return result;
}

} // namespace Scheduler
} // namespace MetaPB
