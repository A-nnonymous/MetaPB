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
      timings[task.id].push_back({start, end, task.opType});
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

  // for first job that is no dependencies(buggy)
  cpuCompleted_[0] = true;
  dpuCompleted_[0] = true;
  mapCompleted_[0] = true;
  reduceCompleted_[0] = true;

  for (size_t i = 0; i < sched.order.size(); ++i) {
    int taskId = sched.order[i];
    const TaskProperties &tp = g.g[taskId];
    Task cpuTask, dpuTask, mapTask, reduceTask;

    if (tp.op == OperatorTag::LOGIC_END) {
      cpuTask = {taskId, []() {}, "CPU", opType2Name.at(tp.opType)};
      dpuTask = {taskId, []() {}, "DPU", opType2Name.at(tp.opType)};
      reduceTask = {taskId, []() {}, "REDUCE", "REDUCE"};
      mapTask = {taskId, []() {}, "MAP", "MAP"};

      size_t totalOutputSize_MiB = 0.0f;
      auto inEdges = boost::in_edges(taskId, g.g);
      for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
        TaskNode pred = boost::source(*ei, g.g);
        dependencies_[taskId].push_back(pred);
        const TaskProperties &predTp = g.g[pred];
        totalOutputSize_MiB += predTp.inputSize_MiB * sched.offloadRatio[pred];
      }
      totalTransfer_mb += totalOutputSize_MiB;
      if (eT == execType::DO) {
        reduceTask.execute = [this, totalOutputSize_MiB]() {
          om.execCPU(OperatorTag::REDUCE, totalOutputSize_MiB,
                     this->memPoolPtr);
        };
      } else { // MIMIC
        perfTag thisTag = {EUType::CPU, OperatorTag::REDUCE,
                           totalOutputSize_MiB};
        if (!cachedCPUPerfDeduce.contains(thisTag))
          cachedCPUPerfDeduce[thisTag] =
              om.deducePerfCPU(OperatorTag::REDUCE, totalOutputSize_MiB);
        mapTask.execute = [this, thisTag]() {
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
          }
        };
      }
      cpuQueue_.push(cpuTask);
      dpuQueue_.push(dpuTask);
      mapQueue_.push(mapTask);
      reduceQueue_.push(reduceTask);
      continue; // continue to processing next node.
    }
    // -------------------------------------------------------------------
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
    if (eT == execType::DO) {
      cpuTask = {taskId,
                 [this, op = tp.op,
                  batchSize = size_t(batchSize_MiB * (1 - offloadRatio))]() {
                   om.execCPU(op, batchSize, this->memPoolPtr);
                 },
                 "CPU", opType2Name.at(tp.opType)};

      dpuTask = {taskId,
                 [this, op = tp.op,
                  batchSize = size_t(batchSize_MiB * offloadRatio)]() {
                   om.execDPU(op, batchSize);
                 },
                 "DPU", opType2Name.at(tp.opType)};

      mapTask = {taskId, []() {}, "MAP", "MAP"};

      reduceTask = {taskId, []() {}, "REDUCE", "REDUCE"};
      
      // undefined tail transfer avoiding
      if (outEdges.first != outEdges.second) { // for single no LOGIC_END node
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
      }
    } else {// ------------------ MIMIC ----------------------
      // ----------------TODO: reuse as function --------------
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
                  // ------------- critical zone --------------
                   {
                     std::lock_guard<std::mutex> lock(this->mutex_);
                     earliestCPUIdleTime_ms =
                         std::max(earliestCPUIdleTime_ms,
                                  std::max(earliestMAPIdleTime_ms,
                                           earliestREDUCEIdleTime_ms)) +
                         perf.timeCost_Second * 1000;
                     totalEnergyCost_joule += perf.energyCost_Joule;
                   }
                  // ------------- critical zone --------------
                 },
                 "CPU", opType2Name.at(tp.opType)};
      // ------------------------------------------------------

      batchSize = size_t(batchSize_MiB * offloadRatio);
      thisTag = {EUType::DPU, tp.op, batchSize};
      if (!cachedDPUPerfDeduce.contains(thisTag))
        cachedDPUPerfDeduce[thisTag] = om.deducePerfDPU(tp.op, batchSize);
      dpuTask = {taskId,
                 [this, thisTag]() {
                   const auto perf = this->cachedDPUPerfDeduce[thisTag];
                   // For all DPU work that allowed to execute, estimation is
                   // these: execute immediately after transfer.
                  // ------------- critical zone --------------
                   {
                     std::lock_guard<std::mutex> lock(this->mutex_);
                     earliestDPUIdleTime_ms =
                         std::max(earliestDPUIdleTime_ms,
                                  std::max(earliestMAPIdleTime_ms,
                                           earliestREDUCEIdleTime_ms)) +
                         perf.timeCost_Second * 1000;
                     totalEnergyCost_joule += perf.energyCost_Joule;
                   }
                  // ------------- critical zone --------------
                 },
                 "DPU", opType2Name.at(tp.opType)};

      mapTask = {taskId, []() {}, "MAP", "MAP"};

      reduceTask = {taskId, []() {}, "REDUCE"};
      
      // undefined tail transfer avoiding
      if (outEdges.first != outEdges.second) {

        if (offloadRatio > omax) {

          size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
          if (!cachedCPUPerfDeduce.contains(thisTag))
            cachedCPUPerfDeduce[thisTag] =
                om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
          reduceTask.execute = [this, thisTag, reduce_work]() {
            const auto perf = this->cachedCPUPerfDeduce[thisTag];
            // For all transfer work that allowed to execute, estimation is
            // these: execute immediately after DPU is idle, block the use of
            // dpu.
            // ------------- critical zone --------------
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
            // ------------- critical zone --------------
          };

        } else if (offloadRatio > omin && offloadRatio < omax) {

          size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
          if (!cachedCPUPerfDeduce.contains(thisTag))
            cachedCPUPerfDeduce[thisTag] =
                om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
          reduceTask.execute = [this, thisTag, reduce_work]() {
            const auto perf = this->cachedCPUPerfDeduce[thisTag];
            // For all transfer work that allowed to execute, estimation is
            // these: execute immediately after DPU is idle, block the use of
            // dpu.
            {
            // ------------- critical zone --------------
              std::lock_guard<std::mutex> lock(this->mutex_);
              earliestDPUIdleTime_ms =
                  std::max(earliestDPUIdleTime_ms,
                           std::max(earliestMAPIdleTime_ms,
                                    earliestREDUCEIdleTime_ms)) +
                  perf.timeCost_Second * 1000;
              totalEnergyCost_joule += perf.energyCost_Joule;
              totalTransfer_mb += reduce_work;
            }
            // ------------- critical zone --------------
          };
          size_t map_work = (omax - offloadRatio) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
          if (!cachedCPUPerfDeduce.contains(thisTag))
            cachedCPUPerfDeduce[thisTag] =
                om.deducePerfCPU(OperatorTag::MAP, map_work);
          mapTask.execute = [this, thisTag, map_work]() {
            const auto perf = this->cachedCPUPerfDeduce[thisTag];
            // For all transfer work that allowed to execute, estimation is
            // these: execute immediately after DPU is idle, block the use of
            // dpu.
            // ------------- critical zone --------------
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
            // ------------- critical zone --------------
          };

        } else {

          size_t map_work = (omax - offloadRatio) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
          if (!cachedCPUPerfDeduce.contains(thisTag))
            cachedCPUPerfDeduce[thisTag] =
                om.deducePerfCPU(OperatorTag::MAP, map_work);
          mapTask.execute = [this, thisTag, map_work]() {
            const auto perf = this->cachedCPUPerfDeduce[thisTag];
            // For all transfer work that allowed to execute, estimation is
            // these: execute immediately after DPU is idle, block the use of
            // dpu.
            // ------------- critical zone --------------
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
            // ------------- critical zone --------------
          }; // definition of mapTask's lambda
        } // offloadRatio < omin

      }// if tail (redundant with op:LOGIC_END handling)
      
    } // if(execType::DO) else

    cpuQueue_.push(cpuTask);
    dpuQueue_.push(dpuTask);
    mapQueue_.push(mapTask);
    reduceQueue_.push(reduceTask);
  }

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

  // -------------------- Entering unsafe multithread zone ------------------
  // Thread workers definition
  ChronoTrigger ct;
  if (eT == execType::DO) {
    ct.tick("HCP");
  }
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
        // return cpuCompleted_[taskId] && dpuCompleted_[taskId];
        return cpuCompleted_[taskId];
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
  if (eT == execType::DO) {
    ct.tock("HCP");
  }

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
    return {energyMeanSum + totalDPUTime_Second * 280, timeMean,
            totalTransfer_mb};
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
