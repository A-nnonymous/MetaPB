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
/*
std::pair<Task> HeteroComputePool::genComputeTask(OperatorTag opTag, size_t inputSize_MiB, float offloadRatio, execType eT)const noexcept{

}

std::pair<Task> HeteroComputePool::genXferTask(OperatorTag opTag, size_t inputSize_MiB, float oMax, float oMin, execType eT)const noexcept{

}
*/
// TODO: 
// 1. Further capsulate this function to a multi-level modulized style
// 2. Add coarse-grained schedule specific task parsing.
void HeteroComputePool::parseGraph(const TaskGraph& g, const Schedule& sched, execType eT)noexcept{
    // ----------- A.Task queue initializing -----------
  for (size_t i = 0; i < sched.order.size(); ++i) {
    int taskId = sched.order[i];
    const TaskProperties &tp = g.g[taskId];
    Task cpuTask, dpuTask, mapTask, reduceTask;

    // ----------- A.1. Normal operator processing ------------
    
    // --- A.1.a Dependency maintain: Pred nodes related logic --- 
    auto inEdges = boost::in_edges(taskId, g.g);
    for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
      TaskNode pred = boost::source(*ei, g.g);
      dependencies_[taskId].push_back(pred);
    }

    float offloadRatio = sched.offloadRatio[i];
    float omax = 0.0f;
    float omin = 1.0f;

    // --- A.1.b Xfer definition: Succ nodes related logic(PUSH style)  --- 
    //std::vector<int> successors;
    auto outEdges = boost::out_edges(taskId, g.g);
    for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
      TaskNode succ = boost::target(*ei, g.g);
      float succOffloadRatio = sched.offloadRatio[succ];
      omax = std::max(omax, succOffloadRatio);
      omin = std::min(omin, succOffloadRatio);
     //successors.push_back(succ);
    }

    // --- A.1.c Job specify related to execType--- 
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
      if (tp.op != OperatorTag::LOGIC_END) { 
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
      cpuTask = {taskId,
                 [this, thisTag]() {
                   const auto perf = om.deducePerfCPU(std::get<1>(thisTag), std::get<2>(thisTag));
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
                 "CPU", opType2Name.at(tp.opType)};
      // ------------------------------------------------------

      batchSize = size_t(batchSize_MiB * offloadRatio);
      thisTag = {EUType::DPU, tp.op, batchSize};
      dpuTask = {taskId,
                 [this, thisTag]() {
                   const auto perf = om.deducePerfDPU(std::get<1>(thisTag), std::get<2>(thisTag));
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
                 "DPU", opType2Name.at(tp.opType)};

      mapTask = {taskId, []() {}, "MAP", "MAP"};

      reduceTask = {taskId, []() {}, "REDUCE"};
      
      // undefined tail transfer avoiding
      if (tp.op != OperatorTag::LOGIC_END) {

        if (offloadRatio > omax) {

          size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
          reduceTask.execute = [this, thisTag, reduce_work]() {
            const auto perf = om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
            // ------------- critical zone --------------
            {
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
              totalTransfer_mb += reduce_work;
            }
            // ------------- critical zone --------------
          };
          // keep up and do nothing
          size_t map_work = 0;
          thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
          mapTask.execute = [this, thisTag, map_work]() {
            // ------------- critical zone --------------
            {
              std::lock_guard<std::mutex> lock(this->mutex_);
              earliestMAPIdleTime_ms = earliestDPUIdleTime_ms;
            }
            // ------------- critical zone --------------
          };

        } else if (offloadRatio > omin && offloadRatio < omax) {

          size_t reduce_work = (offloadRatio - omin) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
          reduceTask.execute = [this, thisTag, reduce_work]() {
            const auto perf = om.deducePerfCPU(OperatorTag::REDUCE, reduce_work);
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
              totalTransfer_mb += reduce_work;
            }
            // ------------- critical zone --------------
          };

          size_t map_work = (omax - offloadRatio) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
          mapTask.execute = [this, thisTag, map_work]() {
            const auto perf = om.deducePerfCPU(OperatorTag::MAP, map_work);
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
              totalTransfer_mb += map_work;
            }
            // ------------- critical zone --------------
          };

        } else {

          // keep up and do nothing
          size_t reduce_work = 0;
          thisTag = {EUType::CPU, OperatorTag::REDUCE, reduce_work};
          reduceTask.execute = [this, thisTag, reduce_work]() {
            {
            // ------------- critical zone --------------
              std::lock_guard<std::mutex> lock(this->mutex_);
              earliestREDUCEIdleTime_ms = earliestDPUIdleTime_ms;
            }
            // ------------- critical zone --------------
          };

          size_t map_work = (omax - offloadRatio) * batchSize_MiB;
          thisTag = {EUType::CPU, OperatorTag::MAP, map_work};
          mapTask.execute = [this, thisTag, map_work]() {
            const auto perf = om.deducePerfCPU(OperatorTag::MAP, map_work);
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
