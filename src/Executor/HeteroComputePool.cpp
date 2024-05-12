#include "Executor/HeteroComputePool.hpp"

namespace MetaPB {
namespace Executor {

bool HeteroComputePool::allDependenciesMet(
    const std::vector<completeSgn> &completedVector,
    const std::vector<int> &deps, double &lastWakerTime_ms,
    std::string my ,std::string other) const noexcept {

  for (const int dep : deps) {
    if (completedVector[dep].isCompleted==false) {
      return false;
    } else {
      lastWakerTime_ms =
          std::max(lastWakerTime_ms, completedVector[dep].completeTime_ms);
    }
  }
  return true;
}

// Generic worker function for processing tasks from a queue
void HeteroComputePool::processTasks(
    std::queue<Task> &queue, std::vector<completeSgn> &completedVector,
    std::function<bool(int)> dependencyCheck,
    std::unordered_map<int, std::vector<TaskTiming>> &timings) noexcept {
  while (!queue.empty()) {
    Task task = queue.front();
    queue.pop();

    // Wait for dependencies to be met
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [&] {
        return dependencyCheck(task.id);
      });
    }

    // exclusivity
    std::unique_lock<std::mutex> dpuLock(
        task.isDPURelated ? dpuMutex_ : mutex_,
        std::defer_lock);
    if (task.isDPURelated){
      dpuMutex_.lock();
    }

    // Record the start time
    auto start = std::chrono::high_resolution_clock::now();

    task.execute();
    //std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Record the end time
    auto end = std::chrono::high_resolution_clock::now();

    // Release the mapReduceMutex for map and reduce tasks
    if (task.isDPURelated){
      dpuMutex_.unlock();
    }
    
    // Update task completion status and record timings
    {
      std::lock_guard<std::mutex> lock(mutex_);
      completedVector[task.id].isCompleted = true;
      timings[task.id].push_back({start, end, task.opType});
      cv_.notify_all();
    }

  }
}

std::pair<Task, Task>
HeteroComputePool::genComputeTask(int taskId, OperatorTag opTag,
                                  OperatorType opType, size_t inputSize_MiB,
                                  float offloadRatio, execType eT) noexcept {
  Task cpuTask, dpuTask;
  size_t cpuInputSize_MiB = size_t(inputSize_MiB * (1 - offloadRatio));
  size_t dpuInputSize_MiB = size_t(inputSize_MiB * offloadRatio);
  std::string opTypeStr = opType2Name.at(opType);

/*
  ////////// DEBUG /////////////
  std::cout << "CPU Task "<< taskId << " ("<< tag2Name.at(opTag)<< "): "<<
  cpuInputSize_MiB << "MiB.\n"; std::cout << "DPU Task "<< taskId << " ("<<
  tag2Name.at(opTag)<< "): "<< dpuInputSize_MiB << "MiB.\n";
  ////////// DEBUG /////////////
  */

  if (eT == execType::DO) {
    cpuTask = {false,
               taskId,
               [this, opTag, cpuInputSize_MiB]() {
                 if (cpuInputSize_MiB)
                   om.execCPU(opTag, cpuInputSize_MiB, this->memPoolPtr);
               },
               opTypeStr};

    dpuTask = {true, taskId,
               [this, opTag, dpuInputSize_MiB]() {
                 if (dpuInputSize_MiB)
                   om.execDPU(opTag, dpuInputSize_MiB);
               },
               opTypeStr};

  } else { // MIMIC logic
    cpuTask = {false,
               taskId,
               [this, taskId, opTag, cpuInputSize_MiB]() {
                 const auto perf =
                     cpuInputSize_MiB
                         ? om.deducePerfCPU(opTag, cpuInputSize_MiB)
                         : perfStats{};
                 // ------------- critical zone --------------
                 {
                   std::lock_guard<std::mutex> lock(this->mutex_);
                   cpuMIMICTime_ms =
                       std::max(cpuMIMICTime_ms, cpuLastWakerTime_ms[taskId]) +
                       perf.timeCost_Second * 1000;
                   cpuCompleted_[taskId].completeTime_ms = cpuMIMICTime_ms;
                   totalEnergyCost_joule += perf.energyCost_Joule;
                 }
                 // ------------- critical zone --------------
               },
               opTypeStr};

    dpuTask = {true,
               taskId,
               [this, taskId, opTag, dpuInputSize_MiB]() {
                 const auto perf = 
                     dpuInputSize_MiB
                         ? om.deducePerfDPU(opTag, dpuInputSize_MiB)
                         : perfStats{};
                 // ------------- critical zone --------------
                 {
                   std::lock_guard<std::mutex> lock(this->mutex_);
                   dpuMIMICTime_ms =
                       std::max(dpuMIMICTime_ms, dpuLastWakerTime_ms[taskId]) +
                       perf.timeCost_Second * 1000;
                   dpuCompleted_[taskId].completeTime_ms = dpuMIMICTime_ms;
                   totalEnergyCost_joule += perf.energyCost_Joule;
                 }
                 // ------------- critical zone --------------
               },
               opTypeStr};
  }

  return {cpuTask, dpuTask};
}

std::pair<Task, Task> HeteroComputePool::genXferTask(int taskId,
                                                     OperatorTag opTag,
                                                     size_t mapWork_MiB,
                                                     size_t reduceWork_MiB,
                                                     execType eT) noexcept {
  Task mapTask, reduceTask;

/*
  ////////// DEBUG /////////////
  std::cout << "MAP Task "<< taskId << ": " << mapWork_MiB << " MiB.\n";
  std::cout << "REDUCE Task "<< taskId << ": "<< reduceWork_MiB << "MiB.\n";
  ////////// DEBUG /////////////
  */

  mapTask = {true,taskId, []() {}, "MAP"};
  reduceTask = {true,taskId, []() {},  "REDUCE"};

  // If this node is LOGIC_END, no xfer to next(because no succNodes)
  if (opTag != OperatorTag::LOGIC_END && opTag != OperatorTag::LOGIC_START) {
    if (eT == execType::DO) {
      reduceTask.execute = [this, reduceWork_MiB]() {
        om.execCPU(OperatorTag::REDUCE, reduceWork_MiB, this->memPoolPtr);
      };
      mapTask.execute = [this, mapWork_MiB]() {
        om.execCPU(OperatorTag::MAP, mapWork_MiB, this->memPoolPtr);
      };
    } else {
      reduceTask.execute = [this, taskId, reduceWork_MiB]() {
        const auto perf =
            (reduceWork_MiB == 0)
                ? perfStats{}
                : om.deducePerfCPU(OperatorTag::REDUCE, reduceWork_MiB);
        {
          // ------------- critical zone --------------
          std::lock_guard<std::mutex> lock(this->mutex_);
          dpuMIMICTime_ms =
              std::max(dpuMIMICTime_ms, reduceLastWakerTime_ms[taskId]) +
              perf.timeCost_Second * 1000;
          reduceCompleted_[taskId].completeTime_ms = dpuMIMICTime_ms;
          totalEnergyCost_joule += perf.energyCost_Joule;
        }
        // ------------- critical zone --------------
      };

      mapTask.execute = [this, taskId, mapWork_MiB]() {
        const auto perf = 
          (mapWork_MiB == 0)
                              ? perfStats{}
                              : om.deducePerfCPU(OperatorTag::MAP, mapWork_MiB);
        // ------------- critical zone --------------
        {
          std::lock_guard<std::mutex> lock(this->mutex_);
          dpuMIMICTime_ms =
              std::max(dpuMIMICTime_ms, mapLastWakerTime_ms[taskId]) +
              perf.timeCost_Second * 1000;
          mapCompleted_[taskId].completeTime_ms = dpuMIMICTime_ms;
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
void HeteroComputePool::parseGraph(const TaskGraph &g, const Schedule &sched,
                                   execType eT) noexcept {
  int taskNum = sched.order.size();
  dependencies_ = std::vector<std::vector<int>>(taskNum, std::vector<int>());
  cpuCompleted_ = std::vector<completeSgn>(taskNum, {false,0.0f});
  dpuCompleted_= std::vector<completeSgn>(taskNum, {false,0.0f});
  mapCompleted_= std::vector<completeSgn>(taskNum, {false,0.0f});
  reduceCompleted_= std::vector<completeSgn>(taskNum, {false,0.0f});
  cpuQueue_ = std::queue<Task>();
  dpuQueue_ = std::queue<Task>();
  mapQueue_= std::queue<Task>();
  reduceQueue_= std::queue<Task>();
  cpuTimings_.clear();
  dpuTimings_.clear();
  mapTimings_.clear();
  reduceTimings_.clear();
  cpuLastWakerTime_ms = std::vector<double>(taskNum, 0.0f);
  dpuLastWakerTime_ms= std::vector<double>(taskNum, 0.0f);
  mapLastWakerTime_ms= std::vector<double>(taskNum, 0.0f);
  reduceLastWakerTime_ms= std::vector<double>(taskNum, 0.0f);
  cpuMIMICTime_ms = 0.0f;
  dpuMIMICTime_ms = 0.0f;
  totalEnergyCost_joule = 0.0f;
  totalTransfer_mb = 0.0f;
  ct.clear();

  // Order Must Ensure first node is LOGIC_START
  // This node is completed without prerequisities.
  //cpuCompleted_[0] = {true, 0.0f};
  //dpuCompleted_[0] = {true, 0.0f};
  //mapCompleted_[0] = {true, 0.0f};
  //reduceCompleted_[0] = {true, 0.0f};

  for (size_t i = 0; i < taskNum; ++i) {
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
    if (outEdges.first != outEdges.second) {
      for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
        TaskNode succ = boost::target(*ei, g.g);
        float succOffloadRatio = sched.offloadRatio[succ];
        omax = std::max(omax, succOffloadRatio);
        omin = std::min(omin, succOffloadRatio);
      }
    } else {
      omax = 0.0f;
      omin = 0.0f;
    }

    size_t reduceWork_MiB, mapWork_MiB;

    if (sched.isAlwaysWrittingBack) {
      reduceWork_MiB = offloadRatio == 0.0f ? 0.0f : tp.inputSize_MiB;
      mapWork_MiB = omax * tp.inputSize_MiB;
    } else {
      if (offloadRatio > omax) {
        reduceWork_MiB = (offloadRatio - omin) * tp.inputSize_MiB;
        mapWork_MiB = 0;
      } else if (offloadRatio > omin && offloadRatio < omax) {
        reduceWork_MiB = (offloadRatio - omin) * tp.inputSize_MiB;
        mapWork_MiB = (omax - offloadRatio) * tp.inputSize_MiB;
      } else {
        reduceWork_MiB = 0;
        mapWork_MiB = (omax - offloadRatio) * tp.inputSize_MiB;
      } // offloadRatio < omin
    }

    auto [cpuTask, dpuTask] = genComputeTask(
        taskId, tp.op, tp.opType, tp.inputSize_MiB, offloadRatio, eT);
    auto [mapTask, reduceTask] =
        genXferTask(taskId, tp.op, mapWork_MiB, reduceWork_MiB, eT);

    cpuQueue_.push(cpuTask);
    dpuQueue_.push(dpuTask);
    mapQueue_.push(mapTask);
    reduceQueue_.push(reduceTask);
  } // if tail (redundant with op:LOGIC_END handling)
}

perfStats HeteroComputePool::execWorkload(const TaskGraph &g,
                                          const Schedule &sched,
                                          execType eT) noexcept {
  parseGraph(g, sched, eT);

  if (eT == execType::DO) {
    ct.tick("HCP");
  }
  // -------------------- Entering unsafe multithread zone ------------------
  // Thread workers definition
  std::thread dpuThread(
      &HeteroComputePool::processTasks, this, std::ref(dpuQueue_),
      std::ref(dpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(mapCompleted_, dependencies_[taskId],
                                  dpuLastWakerTime_ms[taskId],
                                  "DPU","MAP");
      },
      std::ref(dpuTimings_));
  std::thread reduceThread(
      &HeteroComputePool::processTasks, this, std::ref(reduceQueue_),
      std::ref(reduceCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(dpuCompleted_, {taskId},
                                  reduceLastWakerTime_ms[taskId],
                                  "REDUCE","DPU");
      },
      std::ref(reduceTimings_));
  std::thread cpuThread(
      &HeteroComputePool::processTasks, this, std::ref(cpuQueue_),
      std::ref(cpuCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(reduceCompleted_, dependencies_[taskId],
                                  cpuLastWakerTime_ms[taskId],
                                  "CPU", "Reduce");
      },
      std::ref(cpuTimings_));


  std::thread mapThread(
      &HeteroComputePool::processTasks, this, std::ref(mapQueue_),
      std::ref(mapCompleted_),
      [this](int taskId) noexcept {
        return allDependenciesMet(cpuCompleted_, {taskId},
                                  mapLastWakerTime_ms[taskId],
                                  "MAP","CPU");
      },
      std::ref(mapTimings_));


  cpuThread.join();
  dpuThread.join();
  mapThread.join();
  reduceThread.join();

  //---------------  result gathering ----------------
  if (eT == execType::DO) {
    ct.tock("HCP");
    totalDPUTime_Second = 0.0f;
    for (const auto &[_, tps] : dpuTimings_) {
      for (const auto &tp : tps) {
        auto duration = tp.end - tp.start;
        totalDPUTime_Second += std::chrono::duration<double>(duration).count();
      }
    }
    auto report = ct.getReport("HCP");
    auto minStart = std::chrono::high_resolution_clock::now();
    auto maxEnd = std::chrono::high_resolution_clock::time_point::min();

    for (const auto &timings :
         {cpuTimings_, dpuTimings_, mapTimings_, reduceTimings_}) {
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

    auto maxEndDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              maxEnd.time_since_epoch())
                              .count();
    auto minStartDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            minStart.time_since_epoch())
            .count();
    double differenceInMilliseconds =
        static_cast<double>(maxEndDuration - minStartDuration);

    double timeMean = differenceInMilliseconds / 1000;
    auto energyMeans = std::get<vector<Stats>>(
        report.reportItems[metricTag::CPUPowerConsumption_Joule].data);
    auto energyMeanSum =
        energyMeans[0].mean +
        energyMeans[1].mean; // This can only count CPU energy cost.
    return {energyMeanSum + totalDPUTime_Second * 280, timeMean,
            totalTransfer_mb};
  } else { // MIMIC
    perfStats result;
    result.timeCost_Second = std::max(cpuMIMICTime_ms, dpuMIMICTime_ms) / 1000;
    result.energyCost_Joule = totalEnergyCost_joule;
    result.dataMovement_MiB = totalTransfer_mb; // This is precise.
    return result;
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
