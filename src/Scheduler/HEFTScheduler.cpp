#include "Scheduler/HEFTScheduler.hpp"

using std::cout, std::endl;
using std::ifstream;
using std::max;
using std::vector, std::string;

namespace MetaPB {
namespace Scheduler {

Schedule HEFTScheduler::schedule() {
  // Initialize scheduling info for all tasks
  std::unordered_map<TaskNode, TaskSchedulingInfo> schedulingInfo;
  auto vertices = boost::vertices(graw);
  for (auto vi = vertices.first; vi != vertices.second; ++vi) {
    TaskNode tn = *vi;
    TaskProperties &tp = graw[tn];
    TaskSchedulingInfo &info = schedulingInfo[tn];
    info.computationCostCPU =
        om.deducePerfCPU(tp.op, tp.inputSize_MiB).timeCost_Second;
    info.computationCostDPU =
        om.deducePerfDPU(tp.op, tp.inputSize_MiB).timeCost_Second;
  }

  // Calculate upward ranks for all tasks
  for (auto vi = vertices.first; vi != vertices.second; ++vi) {
    calculateUpwardRank(*vi, schedulingInfo);
  }

  // Sort tasks by their upward rank
  std::vector<TaskNode> sortedTasks(vertices.first, vertices.second);
  std::sort(sortedTasks.begin(), sortedTasks.end(),
            [&schedulingInfo](TaskNode a, TaskNode b) {
              return schedulingInfo[a].upwardRank >
                     schedulingInfo[b].upwardRank;
            });

  Schedule scheduleResult;
  scheduleResult.order.reserve(boost::num_vertices(graw));
  scheduleResult.offloadRatio.reserve(boost::num_vertices(graw));

  // Schedule tasks and populate the Schedule instance
  for (TaskNode tn : sortedTasks) {
    scheduleTask(tn, schedulingInfo);
    TaskProperties &tp = graw[tn];
    TaskSchedulingInfo &info = schedulingInfo[tn];

    // Add the task id to the execution order
    scheduleResult.order.push_back(tn);

    // Calculate and add the offload ratio for the task
    // Assuming offloadRatio is 1 if assigned to DPU and 0 if assigned to CPU
    float ratio = (info.assignedProcessor == 1) ? 1.0f : 0.0f;
    scheduleResult.offloadRatio.push_back(ratio);
  }

  return scheduleResult;
}

double HEFTScheduler::calculateUpwardRank(
    TaskNode tn,
    std::unordered_map<TaskNode, TaskSchedulingInfo> &schedulingInfo) {
  TaskSchedulingInfo &info = schedulingInfo[tn];
  if (info.upwardRank != -1)
    return info.upwardRank; // Already calculated

  double maxSuccessorRank = 0.0;
  auto outEdges = boost::out_edges(tn, graw);
  for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
    TaskNode succ = boost::target(*ei, graw);
    maxSuccessorRank =
        std::max(maxSuccessorRank, calculateUpwardRank(succ, schedulingInfo));
  }
  info.upwardRank = (info.computationCostCPU + info.computationCostDPU) / 2.0 +
                    maxSuccessorRank;
  return info.upwardRank;
}

void HEFTScheduler::scheduleTask(
    TaskNode tn,
    std::unordered_map<TaskNode, TaskSchedulingInfo> &schedulingInfo) {
  TaskSchedulingInfo &info = schedulingInfo[tn];
  Processor *bestProcessor = &processors[0];
  double earliestFinishTime = std::numeric_limits<double>::max();

  for (Processor &processor : processors) {
    double earliestStartTime = processor.nextAvailableTime;
    // Add communication costs from predecessor tasks
    auto inEdges = boost::in_edges(tn, graw);
    for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
      TaskNode pred = boost::source(*ei, graw);
      TransferProperties &edgeProps = graw[*ei];
      TaskProperties &sourceNodeProps = graw[pred];
      TaskSchedulingInfo &predInfo = schedulingInfo[pred];
      if (predInfo.assignedProcessor != processor.id) {
        double transferCost =
            om.deducePerfCPU(OperatorTag::MAP,
                             edgeProps.dataSize_Ratio *
                                 (sourceNodeProps.op != OperatorTag::LOGIC_START
                                      ? sourceNodeProps.inputSize_MiB
                                      : 0))
                .timeCost_Second;
        earliestStartTime =
            std::max(earliestStartTime, predInfo.endTime + transferCost);
      }
    }
    double computationCost =
        (processor.id == 0) ? info.computationCostCPU : info.computationCostDPU;
    double finishTime = earliestStartTime + computationCost;
    if (finishTime < earliestFinishTime) {
      earliestFinishTime = finishTime;
      bestProcessor = &processor;
    }
  }

  info.assignedProcessor = bestProcessor->id;
  info.startTime = bestProcessor->nextAvailableTime;
  info.endTime = earliestFinishTime;
  bestProcessor->nextAvailableTime = earliestFinishTime;
}

} // namespace Scheduler
} // namespace MetaPB
