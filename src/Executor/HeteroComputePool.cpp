#include "Executor/HeteroComputePool.hpp"

namespace MetaPB {
namespace Executor {

bool HeteroComputePool::allDependenciesMet(
    const std::vector<bool> &completedVector,
    const std::vector<int> &deps) const noexcept {

  for (int dep : deps) {
    if (!completedVector[dep]) {
      return false;
    }
  }
  return true;
}

// Generic worker function for processing tasks from a queue
void HeteroComputePool::processTasks(
    std::queue<Task> &queue, std::vector<bool> &completedVector,
    std::function<bool(int)> dependencyCheck,
    std::unordered_map<int, std::vector<TaskTiming>> &timings,
    const std::string &type) noexcept {
  while (!queue.empty()) {
    Task task = queue.front();
    queue.pop();

    // Wait for dependencies to be met
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [&] { 
        auto result = dependencyCheck(task.id);
        if(!result){
          // tag the worker with stall mark (used in MIMIC)
          // TODO: refractor stall logic
          if(type=="CPU") {
            isCPUStalled = true;
          }
          if(type=="DPU") {
            isDPUStalled = true;
          }
          if(type=="MAP") {
            isMAPStalled = true;
          }
          if(type=="DPU") {
            isREDUCEStalled = true;
          }
        }
        return result;});
    }

    // exclusivity
    std::unique_lock<std::mutex> dpuLock(
        type == "MAP" || type == "REDUCE" || type == "DPU" ? dpuMutex_ : mutex_,
        std::defer_lock);
    if (type == "MAP" || type == "REDUCE" || type == "DPU") {
      dpuLock.lock();
    }

    // Record the start time
    auto start = std::chrono::high_resolution_clock::now();

    task.execute();

    // Record the end time
    auto end = std::chrono::high_resolution_clock::now();

    // Update task completion status and record timings
    {
      std::lock_guard<std::mutex> lock(mutex_);
      completedVector[task.id] = true;
      timings[task.id].push_back({start, end, task.opType});
    }

    cv_.notify_all();

    // Release the mapReduceMutex for map and reduce tasks
    if (type == "MAP" || type == "REDUCE" || type == "DPU") {
      dpuLock.unlock();
    }
  }
}

std::pair<Task,Task> HeteroComputePool::genComputeTask(int taskId, OperatorTag opTag, OperatorType opType, size_t inputSize_MiB, float offloadRatio, execType eT)noexcept{
  Task cpuTask, dpuTask;
  size_t cpuInputSize_MiB = size_t(inputSize_MiB * (1 - offloadRatio));
  size_t dpuInputSize_MiB = size_t(inputSize_MiB * offloadRatio);
  std::string opTypeStr= opType2Name.at(opType);

  
  /*
  ////////// DEBUG /////////////
  std::cout << "CPU Task "<< taskId << " ("<< tag2Name.at(opTag)<< "): "<< cpuInputSize_MiB << "MiB.\n";
  std::cout << "DPU Task "<< taskId << " ("<< tag2Name.at(opTag)<< "): "<< dpuInputSize_MiB << "MiB.\n";
  ////////// DEBUG /////////////
  */

  if (eT == execType::DO) {
      cpuTask = {taskId,
                 [this, opTag, cpuInputSize_MiB]() {
                   if(cpuInputSize_MiB)om.execCPU(opTag, cpuInputSize_MiB, this->memPoolPtr);
                 },
                 "CPU", opTypeStr};

      dpuTask = {taskId,
                 [this, opTag, dpuInputSize_MiB]() {
                   if(dpuInputSize_MiB)om.execDPU(opTag, dpuInputSize_MiB);
                 },
                 "DPU", opTypeStr};

  }else{ // MIMIC logic
    cpuTask = {taskId,
                [this, opTag, cpuInputSize_MiB]() {
                  const auto perf = cpuInputSize_MiB? om.deducePerfCPU(opTag, cpuInputSize_MiB) : perfStats{};
                // ------------- critical zone --------------
                  {
                    std::lock_guard<std::mutex> lock(this->mutex_);
                    if(!this->isCPUStalled){ // immediate execution without stalling
                      earliestCPUIdleTime_ms +=  (perf.timeCost_Second * 1000);
                    }else{
                      earliestCPUIdleTime_ms = std::max(earliestCPUIdleTime_ms, earliestREDUCEIdleTime_ms) + (perf.timeCost_Second * 1000);
                      this->isCPUStalled = false; // maintain stall mark
                    }
                    totalEnergyCost_joule += perf.energyCost_Joule;
                  }
                // ------------- critical zone --------------
                },
                "CPU", opTypeStr};

    dpuTask = {taskId,
                [this, opTag, dpuInputSize_MiB]() {
                  const auto perf = dpuInputSize_MiB? om.deducePerfDPU(opTag, dpuInputSize_MiB) : perfStats{};
                // ------------- critical zone --------------
                  {
                    std::lock_guard<std::mutex> lock(this->mutex_);
                    if(!this->isDPUStalled){ // immediate execution without stalling
                      earliestDPUIdleTime_ms +=  (perf.timeCost_Second * 1000);
                    }else{
                      earliestDPUIdleTime_ms = std::max(earliestMAPIdleTime_ms, earliestDPUIdleTime_ms) + (perf.timeCost_Second * 1000);
                      this->isDPUStalled = false; // maintain stall mark
                    }
                    totalEnergyCost_joule += perf.energyCost_Joule;
                  }
                // ------------- critical zone --------------
                },
                "DPU", opTypeStr};

  }

  return {cpuTask, dpuTask};
}

std::pair<Task,Task> HeteroComputePool::genXferTask(int taskId, OperatorTag opTag, size_t mapWork_MiB, size_t reduceWork_MiB, execType eT)noexcept{
  Task mapTask, reduceTask;

  /*
  ////////// DEBUG /////////////
  std::cout << "MAP Task "<< taskId << ": " << mapWork_MiB << " MiB.\n";
  std::cout << "REDUCE Task "<< taskId << ": "<< reduceWork_MiB << "MiB.\n";
  ////////// DEBUG /////////////
  */

  mapTask = {taskId, []() {}, "MAP", "MAP"};
  reduceTask = {taskId, []() {}, "REDUCE", "REDUCE"};

  // If this node is LOGIC_END, no xfer to next(because no succNodes)
  if (opTag != OperatorTag::LOGIC_END && opTag != OperatorTag::LOGIC_START) { 
    if (eT == execType::DO) {
      reduceTask.execute = [this, reduceWork_MiB]() {
        om.execCPU(OperatorTag::REDUCE, reduceWork_MiB, this->memPoolPtr);
      };
      mapTask.execute = [this, mapWork_MiB]() {
        om.execCPU(OperatorTag::MAP, mapWork_MiB, this->memPoolPtr);
      };
    }else{
      reduceTask.execute = [this, reduceWork_MiB]() {
        const auto perf = (reduceWork_MiB == 0)? perfStats{} : om.deducePerfCPU(OperatorTag::REDUCE, reduceWork_MiB);
        {
        // ------------- critical zone --------------
          std::lock_guard<std::mutex> lock(this->mutex_);
          if(!this->isREDUCEStalled){
            earliestDPUIdleTime_ms +=  (perf.timeCost_Second * 1000);
            earliestREDUCEIdleTime_ms = earliestDPUIdleTime_ms;
          }else{
            earliestDPUIdleTime_ms = std::max(earliestDPUIdleTime_ms, earliestCPUIdleTime_ms) +(perf.timeCost_Second * 1000);
            earliestREDUCEIdleTime_ms = earliestDPUIdleTime_ms;
          }
          this->isREDUCEStalled = false;
          totalEnergyCost_joule += perf.energyCost_Joule;
        }
        // ------------- critical zone --------------
      };

      mapTask.execute = [this, mapWork_MiB]() {
        const auto perf = (mapWork_MiB == 0)? perfStats{}: om.deducePerfCPU(OperatorTag::MAP, mapWork_MiB);
        // ------------- critical zone --------------
        {
          std::lock_guard<std::mutex> lock(this->mutex_);
          if(!this->isMAPStalled){
            earliestDPUIdleTime_ms +=  (perf.timeCost_Second * 1000);
            earliestMAPIdleTime_ms = earliestDPUIdleTime_ms;
          }else{
            earliestDPUIdleTime_ms = std::max(earliestDPUIdleTime_ms, earliestCPUIdleTime_ms) +(perf.timeCost_Second * 1000);
            earliestMAPIdleTime_ms = earliestDPUIdleTime_ms;
          }

          this->isMAPStalled = false;
          totalEnergyCost_joule += perf.energyCost_Joule;
        }
        // ------------- critical zone --------------
      };
    }
    totalTransfer_mb += reduceWork_MiB + mapWork_MiB;
  }

  return {mapTask, reduceTask};
}
// TODO: 
// 1. Further capsulate this function to a multi-level modulized style
// 2. Add coarse-grained schedule specific task parsing.
void HeteroComputePool::parseGraph(const TaskGraph& g, const Schedule& sched, execType eT)noexcept{
  for (size_t i = 0; i < sched.order.size(); ++i) {
    int taskId = sched.order[i];
    const TaskProperties &tp = g.g[taskId];

    // Looking upward: Dependencies maintain.
    auto inEdges = boost::in_edges(taskId, g.g);
    for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
      TaskNode pred = boost::source(*ei, g.g);
      dependencies_[taskId].push_back(pred);
    }

    float offloadRatio = sched.offloadRatio[i];
    float omax = 0.0f;
    float omin = 1.0f;

    // Looking downward: Transfer measuring.
    auto outEdges = boost::out_edges(taskId, g.g);
    if(outEdges.first != outEdges.second){
      for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
        TaskNode succ = boost::target(*ei, g.g);
        float succOffloadRatio = sched.offloadRatio[succ];
        omax = std::max(omax, succOffloadRatio);
        omin = std::min(omin, succOffloadRatio);
      }
    }else{
      omax = 0.0f; omin = 0.0f;
    }

    size_t reduceWork_MiB, mapWork_MiB;

    if(sched.isAlwaysWrittingBack){
      reduceWork_MiB = offloadRatio == 0.0f? 0.0f : tp.inputSize_MiB;
      mapWork_MiB = omax * tp.inputSize_MiB;
    }else{
      if (offloadRatio > omax) {
        reduceWork_MiB = (offloadRatio - omin) * tp.inputSize_MiB;
        mapWork_MiB = 0;
      } else if (offloadRatio > omin && offloadRatio < omax) {
        reduceWork_MiB = (offloadRatio - omin) * tp.inputSize_MiB;
        mapWork_MiB = (omax - offloadRatio) * tp.inputSize_MiB;
      } else {
        reduceWork_MiB = 0;
        mapWork_MiB= (omax - offloadRatio) * tp.inputSize_MiB;
      } // offloadRatio < omin
    }

    auto [cpuTask, dpuTask] = genComputeTask(taskId, tp.op, tp.opType,
                              tp.inputSize_MiB, offloadRatio, eT);
    auto [mapTask, reduceTask] = genXferTask(taskId, tp.op, mapWork_MiB, reduceWork_MiB, eT);

    cpuQueue_.push(cpuTask);
    dpuQueue_.push(dpuTask);
    mapQueue_.push(mapTask);
    reduceQueue_.push(reduceTask);
  }// if tail (redundant with op:LOGIC_END handling)
}

perfStats HeteroComputePool::execWorkload(const TaskGraph &g,
                                          const Schedule &sched,
                                          execType eT) noexcept {
  // ------------- State cleaning -------------
  for (auto &deps : dependencies_) {
    deps.clear();
  }
  std::fill(cpuCompleted_.begin(), cpuCompleted_.end(), false);
  std::fill(dpuCompleted_.begin(), dpuCompleted_.end(), false);
  std::fill(mapCompleted_.begin(), mapCompleted_.end(), false);
  std::fill(reduceCompleted_.begin(), reduceCompleted_.end(), false);
  earliestCPUIdleTime_ms = 0.0f;
  earliestDPUIdleTime_ms = 0.0f;
  earliestMAPIdleTime_ms = 0.0f;
  earliestREDUCEIdleTime_ms = 0.0f;
  totalEnergyCost_joule = 0.0f;
  totalTransfer_mb = 0.0f;
  ct.clear();

  // Order Must Ensure first node is LOGIC_START
  // This node is completed without prerequisities.
  cpuCompleted_[0] = true;
  dpuCompleted_[0] = true;
  mapCompleted_[0] = true;
  reduceCompleted_[0] = true;

  parseGraph(g, sched, eT);

  cpuTimings_.clear();
  if (eT == execType::DO) { // DPU Energy deduce
    totalDPUTime_Second = 0.0f;
    for (const auto &[_, tps] : dpuTimings_) {
      for (const auto &tp : tps) {
        auto duration = tp.end - tp.start;
        totalDPUTime_Second += std::chrono::duration<double>(duration).count();
      }
    }
  }
  dpuTimings_.clear();
  mapTimings_.clear();
  reduceTimings_.clear();

  if (eT == execType::DO) {
    ct.tick("HCP");
  }
  // -------------------- Entering unsafe multithread zone ------------------
  // Thread workers definition
  {
  std::jthread cpuThread(
      &HeteroComputePool::processTasks, this, std::ref(cpuQueue_),
      std::ref(cpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(reduceCompleted_, dependencies_[taskId]);
      },
      std::ref(cpuTimings_), "CPU");

  std::jthread dpuThread(
      &HeteroComputePool::processTasks, this, std::ref(dpuQueue_),
      std::ref(dpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(mapCompleted_, dependencies_[taskId]);
               
      },
      std::ref(dpuTimings_), "DPU");

  std::jthread mapThread(
      &HeteroComputePool::processTasks, this, std::ref(mapQueue_),
      std::ref(mapCompleted_),
      [this](int taskId) noexcept {
        return cpuCompleted_[taskId];
      },
      std::ref(mapTimings_), "MAP");

  std::jthread reduceThread(
      &HeteroComputePool::processTasks, this, std::ref(reduceQueue_),
      std::ref(reduceCompleted_),
      [this](int taskId) noexcept { 
        return dpuCompleted_[taskId]; 
      },
      std::ref(reduceTimings_), "REDUCE");
  }

  if (eT == execType::DO) {
    ct.tock("HCP");
  }
  if (eT == execType::MIMIC) {
    perfStats result;
    result.timeCost_Second = std::max(
        std::max(earliestCPUIdleTime_ms / 1000, earliestDPUIdleTime_ms / 1000),
        std::max(earliestMAPIdleTime_ms / 1000,
                 earliestREDUCEIdleTime_ms / 1000));
    result.energyCost_Joule = totalEnergyCost_joule;
    result.dataMovement_MiB = totalTransfer_mb; // This is precise.
    return result;
  } else {
    auto report = ct.getReport("HCP");
    /*
    auto timeMean =
        std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
            .mean /
        1e9;
        */
    auto minStart = std::chrono::high_resolution_clock::now();
    auto maxEnd = std::chrono::high_resolution_clock::time_point::min();

    for (const auto &timings : {cpuTimings_, dpuTimings_, mapTimings_, reduceTimings_}) {
        for (const auto &pair : timings) {
            for (const auto &timing : pair.second) {
                if (timing.start < minStart) {
                    minStart = timing.start;
                }
                if (timing.end > maxEnd) {
                    maxEnd = timing.end;
                }
            }
        }
    }

    auto maxEndDuration = std::chrono::duration_cast<std::chrono::milliseconds>(maxEnd.time_since_epoch()).count();
    auto minStartDuration = std::chrono::duration_cast<std::chrono::milliseconds>(minStart.time_since_epoch()).count();
    double differenceInMilliseconds = static_cast<double>(maxEndDuration - minStartDuration);

    double timeMean = differenceInMilliseconds / 1000;
    auto energyMeans = std::get<vector<Stats>>(
        report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
    auto energyMeanSum =
        energyMeans[0].mean +
        energyMeans[1].mean; // This can only count CPU energy cost.
    return {energyMeanSum + totalDPUTime_Second * 280, timeMean,
            totalTransfer_mb};
  }

}

// ---------------------- Timing visualization ------------------------------
void HeteroComputePool::printTimings() const noexcept {
  printTimingsForType("CPU", cpuTimings_);
  printTimingsForType("DPU", dpuTimings_);
  printTimingsForType("MAP", mapTimings_);
  printTimingsForType("REDUCE", reduceTimings_);
}
void HeteroComputePool::outputTimingsToCSV(
    const std::string &filename) const noexcept {
  std::ofstream csvFile(filename);
  std::cout << "CSV writting started" << std::endl;
  if (!csvFile.is_open()) {
    std::cerr << "Failed to open the CSV file." << std::endl;
    return;
  }

  // Find the smallest start time across all tasks
  auto minStart = std::chrono::high_resolution_clock::now();
  for (const auto &timings :
       {cpuTimings_, dpuTimings_, mapTimings_, reduceTimings_}) {
    for (const auto &pair : timings) {
      for (const auto &timing : pair.second) {
        if (timing.start < minStart) {
          minStart = timing.start;
        }
      }
    }
  }

  // Write the headers to the CSV file
  csvFile << "TaskID,Worker,OpType,Start(ms),End(ms)\n";

  // Helper lambda to write timings for a specific type of task
  auto writeTimings =
      [&csvFile, minStart](
          const std::string &worker,
          const std::unordered_map<int, std::vector<TaskTiming>> &timings) {
        for (const auto &pair : timings) {
          for (const auto &timing : pair.second) {
            auto start = std::chrono::duration_cast<std::chrono::milliseconds>(
                             timing.start - minStart)
                             .count();
            auto end = std::chrono::duration_cast<std::chrono::milliseconds>(
                           timing.end - minStart)
                           .count();
            auto type = timing.taskType;
            csvFile << pair.first << "," << worker << "," << type << ","
                    << std::fixed << std::setprecision(4) << start << ","
                    << std::fixed << std::setprecision(4) << end << "\n";
          }
        }
      };

  // Write timings for each type of task
  writeTimings("CPU", cpuTimings_);
  writeTimings("DPU", dpuTimings_);
  writeTimings("MAP", mapTimings_);
  writeTimings("REDUCE", reduceTimings_);

  // Close the CSV file
  csvFile.close();
}

// Helper function to print timings for a specific type of task
void HeteroComputePool::printTimingsForType(
    const std::string &type,
    const std::unordered_map<int, std::vector<TaskTiming>> &timings)
    const noexcept {
  std::cout << type << " Timings:" << std::endl;
  for (const auto &pair : timings) {
    for (const auto &timing : pair.second) {
      auto start = std::chrono::duration_cast<std::chrono::milliseconds>(
                       timing.start.time_since_epoch())
                       .count();
      auto end = std::chrono::duration_cast<std::chrono::milliseconds>(
                     timing.end.time_since_epoch())
                     .count();
      std::cout << "Task " << pair.first << " - Start: " << start
                << "ms, End: " << end << "ms" << std::endl;
    }
  }
}

} // namespace Executor
} // namespace MetaPB
