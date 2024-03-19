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
      cv_.wait(lock, [&] { return dependencyCheck(task.id); });
    }

    // For map and reduce tasks, acquire the mapReduceMutex to ensure
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
      timings[task.id].push_back({start, end});
    }

    cv_.notify_all();

    // Release the mapReduceMutex for map and reduce tasks
    if (type == "MAP" || type == "REDUCE" || type == "DPU") {
      dpuLock.unlock();
    }
  }
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

  // iterate through task order
  for (size_t i = 0; i < sched.order.size(); ++i) {
    int taskId = sched.order[i];
    const TaskProperties &tp = g.g[taskId];

    if (tp.op == OperatorTag::LOGIC_START) {
      cpuCompleted_[taskId] = true;
      dpuCompleted_[taskId] = true;
      mapCompleted_[taskId] = true;
      reduceCompleted_[taskId] = true;
      continue;
    }
    auto inEdges = boost::in_edges(taskId, g.g);
    for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
      TaskNode pred = boost::source(*ei, g.g);
      dependencies_[taskId].push_back(pred);
    }

    float offloadRatio = sched.offloadRatio[i];
    float omax = 0.0f;
    float omin = 1.0f;
    std::vector<int> successors;
    auto outEdges = boost::out_edges(taskId, g.g);
    for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
      TaskNode succ = boost::target(*ei, g.g);
      float succOffloadRatio = sched.offloadRatio[succ];
      omax = std::max(omax, succOffloadRatio);
      omin = std::min(omin, succOffloadRatio);
      successors.push_back(succ);
    }

    float batchSize_MiB = tp.inputSize_MiB;
    Task cpuTask, dpuTask, mapTask, reduceTask;
    if (eT == execType::DO) {
      cpuTask = {taskId,
                 [this, op = tp.op,
                  batchSize = size_t(batchSize_MiB * (1 - offloadRatio))]() {
                   om.execCPU(op, batchSize, this->memPoolPtr);
                 },
                 "CPU"};

      dpuTask = {taskId,
                 [this, op = tp.op,
                  batchSize = size_t(batchSize_MiB * offloadRatio)]() {
                   om.execDPU(op, batchSize);
                 },
                 "DPU"};

      mapTask = {taskId, []() {}, // 初始化为空执行函数
                 "MAP"};

      reduceTask = {taskId, []() {}, // 初始化为空执行函数
                    "REDUCE"};
      if (offloadRatio > omax) {
        size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
        reduceTask.execute = [this, reduce_work]() {
          om.execCPU(OperatorTag::REDUCE, reduce_work, this->memPoolPtr);
        };
        totalTransfer_mb += reduce_work;
      } else if (offloadRatio > omin && offloadRatio < omax) {
        size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
        reduceTask.execute = [this, reduce_work]() {
          om.execCPU(OperatorTag::REDUCE, reduce_work, this->memPoolPtr);
        };
        size_t map_work = (omax - offloadRatio) * batchSize_MiB;
        mapTask.execute = [this, map_work]() {
          om.execCPU(OperatorTag::MAP, map_work, this->memPoolPtr);
        };
        totalTransfer_mb += reduce_work + map_work;
      } else {
        size_t map_work = (omax - offloadRatio) * batchSize_MiB;
        mapTask.execute = [this, map_work]() {
          om.execCPU(OperatorTag::MAP, map_work, this->memPoolPtr);
        };
        totalTransfer_mb += map_work;
      }
    } else {            // ----------- MIMIC dependency and time forwarding
                        // --------------------
      size_t batchSize; // works that to be done on this EU
      perfTag thisTag;  // performance tag of this work

      batchSize = size_t(batchSize_MiB * (1 - offloadRatio));
      thisTag = {EUType::CPU, tp.op, batchSize};
      if (!cachedCPUPerfDeduce.contains(thisTag))
        cachedCPUPerfDeduce[thisTag] = om.deducePerfCPU(tp.op, batchSize);
      cpuTask = {taskId,
                 [this, thisTag]() {
                   const auto perf = this->cachedCPUPerfDeduce[thisTag];
                   // For all CPU work that allowed to execute, estimation is
                   // these: execute immediately after transfer.
                   {
                     std::lock_guard<std::mutex> lock(this->mutex_);
                     earliestCPUIdleTime_ms =
                         std::max(earliestCPUIdleTime_ms,
                                  std::max(earliestMAPIdleTime_ms,
                                           earliestREDUCEIdleTime_ms)) +
                         perf.timeCost_Second * 1000;
                     totalEnergyCost_joule += perf.energyCost_Joule;
                   }
                 },
                 "CPU"};

      batchSize = size_t(batchSize_MiB * offloadRatio);
      thisTag = {EUType::DPU, tp.op, batchSize};
      if (!cachedDPUPerfDeduce.contains(thisTag))
        cachedDPUPerfDeduce[thisTag] = om.deducePerfDPU(tp.op, batchSize);
      dpuTask = {taskId,
                 [this, thisTag]() {
                   const auto perf = this->cachedDPUPerfDeduce[thisTag];
                   // For all DPU work that allowed to execute, estimation is
                   // these: execute immediately after transfer.
                   {
                     std::lock_guard<std::mutex> lock(this->mutex_);
                     earliestDPUIdleTime_ms =
                         std::max(earliestDPUIdleTime_ms,
                                  std::max(earliestMAPIdleTime_ms,
                                           earliestREDUCEIdleTime_ms)) +
                         perf.timeCost_Second * 1000;
                     totalEnergyCost_joule += perf.energyCost_Joule;
                   }
                 },
                 "DPU"};

      mapTask = {taskId, []() {}, // 初始化为空执行函数
                 "MAP"};

      reduceTask = {taskId, []() {}, // 初始化为空执行函数
                    "REDUCE"};
      if (offloadRatio > omax) {
        size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
        thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
        if (!cachedCPUPerfDeduce.contains(thisTag))
          cachedCPUPerfDeduce[thisTag] =
              om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
        reduceTask.execute = [this, thisTag, reduce_work]() {
          const auto perf = this->cachedCPUPerfDeduce[thisTag];
          // For all transfer work that allowed to execute, estimation is these:
          // execute immediately after DPU is idle, block the use of dpu.
          {
            std::lock_guard<std::mutex> lock(this->mutex_);
            earliestDPUIdleTime_ms =
                std::max(earliestDPUIdleTime_ms,
                         std::max(earliestMAPIdleTime_ms,
                                  earliestREDUCEIdleTime_ms)) +
                perf.timeCost_Second * 1000;
            totalEnergyCost_joule += perf.energyCost_Joule;
            totalTransfer_mb += reduce_work;
          }
        };
      } else if (offloadRatio > omin && offloadRatio < omax) {
        size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
        thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
        if (!cachedCPUPerfDeduce.contains(thisTag))
          cachedCPUPerfDeduce[thisTag] =
              om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
        reduceTask.execute = [this, thisTag, reduce_work]() {
          const auto perf = this->cachedCPUPerfDeduce[thisTag];
          // For all transfer work that allowed to execute, estimation is these:
          // execute immediately after DPU is idle, block the use of dpu.
          {
            std::lock_guard<std::mutex> lock(this->mutex_);
            earliestDPUIdleTime_ms =
                std::max(earliestDPUIdleTime_ms,
                         std::max(earliestMAPIdleTime_ms,
                                  earliestREDUCEIdleTime_ms)) +
                perf.timeCost_Second * 1000;
            totalEnergyCost_joule += perf.energyCost_Joule;
            totalTransfer_mb += reduce_work;
          }
        };

        size_t map_work = (omax - offloadRatio) * batchSize_MiB;
        thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
        if (!cachedCPUPerfDeduce.contains(thisTag))
          cachedCPUPerfDeduce[thisTag] =
              om.deducePerfCPU(OperatorTag::MAP, map_work);
        mapTask.execute = [this, thisTag, map_work]() {
          const auto perf = this->cachedCPUPerfDeduce[thisTag];
          // For all transfer work that allowed to execute, estimation is these:
          // execute immediately after DPU is idle, block the use of dpu.
          {
            std::lock_guard<std::mutex> lock(this->mutex_);
            earliestDPUIdleTime_ms =
                std::max(earliestDPUIdleTime_ms,
                         std::max(earliestMAPIdleTime_ms,
                                  earliestREDUCEIdleTime_ms)) +
                perf.timeCost_Second * 1000;
            totalEnergyCost_joule += perf.energyCost_Joule;
            totalTransfer_mb += map_work;
          }
        };
      } else {
        size_t map_work = (omax - offloadRatio) * batchSize_MiB;
        thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
        if (!cachedCPUPerfDeduce.contains(thisTag))
          cachedCPUPerfDeduce[thisTag] =
              om.deducePerfCPU(OperatorTag::MAP, map_work);
        mapTask.execute = [this, thisTag, map_work]() {
          const auto perf = this->cachedCPUPerfDeduce[thisTag];
          // For all transfer work that allowed to execute, estimation is these:
          // execute immediately after DPU is idle, block the use of dpu.
          {
            std::lock_guard<std::mutex> lock(this->mutex_);
            earliestDPUIdleTime_ms =
                std::max(earliestDPUIdleTime_ms,
                         std::max(earliestMAPIdleTime_ms,
                                  earliestREDUCEIdleTime_ms)) +
                perf.timeCost_Second * 1000;
            totalEnergyCost_joule += perf.energyCost_Joule;
            totalTransfer_mb += map_work;
          }
        };
      }
    } // if(execType::DO)

    cpuQueue_.push(cpuTask);
    dpuQueue_.push(dpuTask);
    mapQueue_.push(mapTask);
    reduceQueue_.push(reduceTask);
  }

  cpuTimings_.clear();
  dpuTimings_.clear();
  mapTimings_.clear();
  reduceTimings_.clear();

  // -------------------- Entering unsafe multithread zone ------------------
  // Thread workers definition
  std::cout << "HCP running ...\n";
  ChronoTrigger ct;
  ct.tick("HCP");
  std::thread cpuThread(
      &HeteroComputePool::processTasks, this, std::ref(cpuQueue_),
      std::ref(cpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(mapCompleted_, dependencies_[taskId]) &&
               allDependenciesMet(reduceCompleted_, dependencies_[taskId]);
      },
      std::ref(cpuTimings_), "CPU");

  std::thread dpuThread(
      &HeteroComputePool::processTasks, this, std::ref(dpuQueue_),
      std::ref(dpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(mapCompleted_, dependencies_[taskId]) &&
               allDependenciesMet(reduceCompleted_, dependencies_[taskId]);
      },
      std::ref(dpuTimings_), "DPU");

  std::thread mapThread(
      &HeteroComputePool::processTasks, this, std::ref(mapQueue_),
      std::ref(mapCompleted_),
      [this](int taskId) noexcept {
        return cpuCompleted_[taskId] && dpuCompleted_[taskId];
      },
      std::ref(mapTimings_), "MAP");

  std::thread reduceThread(
      &HeteroComputePool::processTasks, this, std::ref(reduceQueue_),
      std::ref(reduceCompleted_),
      [this](int taskId) noexcept { return dpuCompleted_[taskId]; },
      std::ref(reduceTimings_), "REDUCE");

  cpuThread.join();
  dpuThread.join();
  mapThread.join();
  reduceThread.join();
  ct.tock("HCP");

  std::cout << "HCP running completed\n";
  perfStats result;
  if (eT == execType::MIMIC) {
    result.timeCost_Second = std::max(
        std::max(earliestCPUIdleTime_ms / 1000, earliestDPUIdleTime_ms / 1000),
        std::max(earliestMAPIdleTime_ms / 1000,
                 earliestREDUCEIdleTime_ms / 1000));
    result.energyCost_Joule = totalEnergyCost_joule;
    result.dataMovement_MiB = totalTransfer_mb; // This is precise.
    return result;
  } else {
    auto report = ct.getReport("HCP");
    auto timeMean =
        std::get<Stats>(report.reportItems[metricTag::TimeConsume_ns].data)
            .mean /
        1e9;
    auto energyMeans = std::get<vector<Stats>>(
        report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
    auto energyMeanSum =
        energyMeans[0].mean +
        energyMeans[1].mean; // This can only count CPU energy cost.
    return {energyMeanSum * 2, timeMean, totalTransfer_mb};
  }
}

// Print timings for each type of task
void HeteroComputePool::printTimings() const noexcept {
  printTimingsForType("CPU", cpuTimings_);
  printTimingsForType("DPU", dpuTimings_);
  printTimingsForType("MAP", mapTimings_);
  printTimingsForType("REDUCE", reduceTimings_);
}
void HeteroComputePool::outputTimingsToCSV(
    const std::string &filename) const noexcept {
  std::ofstream csvFile(filename);
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
  csvFile << "TaskID,Type,Start(ms),End(ms)\n";

  // Helper lambda to write timings for a specific type of task
  auto writeTimings =
      [&csvFile, minStart](
          const std::string &type,
          const std::unordered_map<int, std::vector<TaskTiming>> &timings) {
        for (const auto &pair : timings) {
          for (const auto &timing : pair.second) {
            auto start = std::chrono::duration_cast<std::chrono::milliseconds>(
                             timing.start - minStart)
                             .count();
            auto end = std::chrono::duration_cast<std::chrono::milliseconds>(
                           timing.end - minStart)
                           .count();
            csvFile << pair.first << "," << type << "," << std::fixed
                    << std::setprecision(4) << start << "," << std::fixed
                    << std::setprecision(4) << end << "\n";
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