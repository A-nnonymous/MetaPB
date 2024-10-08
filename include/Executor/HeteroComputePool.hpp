#ifndef HCP_HPP
#define HCP_HPP
#include "Executor/TaskGraph.hpp"
#include "Operator/OperatorManager.hpp"
#include "utils/ChronoTrigger.hpp"
#include "utils/MetricsGather.hpp"
#include "utils/Stats.hpp"
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace MetaPB {
namespace Executor {
using Operator::CPU_TCB;
using Operator::OperatorManager;
using Operator::OperatorTag;
using DPU_TCB = Operator::DPU_TCB;
using Operator::opType2Name;
using Operator::tag2Name;
using utils::ChronoTrigger;
using utils::metricTag;
using utils::perfStats;
using utils::Stats;
// TaskTiming struct to store start and end times for tasks
typedef struct TaskTiming {
  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point end;
  std::string taskType;
} TaskTiming;

// Task struct to store task ID and execution function
typedef struct {
  bool isDPURelated = false;
  int id;
  std::function<void()> execute;
  std::string opType;
} Task;

enum class execType { MIMIC, DO };

enum class EUType { CPU, DPU };

typedef std::tuple<EUType, OperatorTag, size_t> perfTag;

typedef struct {
  bool isCompleted = false;
  double completeTime_ms = 0.0f; // time elapsed from begin. maintained in MIMIC
} completeSgn;

class HeteroComputePool {
public:
  // Constructor initializes the pool with the expected maximum task ID.
  HeteroComputePool(const OperatorManager &om, void **memPoolPtr) noexcept
      : om(om), memPoolPtr(memPoolPtr) {}

  HeteroComputePool(HeteroComputePool &&other) noexcept
      : memPoolPtr(std::exchange(other.memPoolPtr, nullptr)),
        memPoolNum(std::exchange(other.memPoolNum, 0)), om(other.om), mutex_(),
        cv_(), dpuMutex_(), dependencies_(std::move(other.dependencies_)),
        cpuCompleted_(std::move(other.cpuCompleted_)),
        dpuCompleted_(std::move(other.dpuCompleted_)),
        mapCompleted_(std::move(other.mapCompleted_)),
        reduceCompleted_(std::move(other.reduceCompleted_)),
        cpuQueue_(std::move(other.cpuQueue_)),
        dpuQueue_(std::move(other.dpuQueue_)),
        mapQueue_(std::move(other.mapQueue_)),
        reduceQueue_(std::move(other.reduceQueue_)),
        cpuTimings_(std::move(other.cpuTimings_)),
        dpuTimings_(std::move(other.dpuTimings_)),
        mapTimings_(std::move(other.mapTimings_)),
        reduceTimings_(std::move(other.reduceTimings_)),
        totalEnergyCost_joule(std::exchange(other.totalEnergyCost_joule, 0.0)),
        totalTransfer_mb(std::exchange(other.totalTransfer_mb, 0.0)) {}

  void parseGraph(const TaskGraph &g, const Schedule &sched,
                  execType eT) noexcept;

  perfStats execWorkload(const TaskGraph &g, const Schedule &sched,
                         execType) noexcept;

  // Print timings for each type of task
  void printTimings() const noexcept;
  void outputTimingsToCSV(const std::string &filename) const noexcept;

  ~HeteroComputePool() noexcept {}

private:
  // Helper function to print timings for a specific type of task
  void
  printTimingsForType(const std::string &type,
                      const std::unordered_map<int, std::vector<TaskTiming>>
                          &timings) const noexcept;
  // Check if all dependencies for a task are met
  bool allDependenciesMet(const std::vector<completeSgn> &completedVector,
                          const std::vector<int> &deps,
                          double &lastWakerTime_ms, std::string my,
                          std::string other) const noexcept;

  void cleanStatus(int taskNum) noexcept;

  // Generic worker function for processing tasks from a queue
  void processTasks(
      std::queue<Task> &queue, std::vector<completeSgn> &completedVector,
      std::function<bool(int)> dependencyCheck,
      std::unordered_map<int, std::vector<TaskTiming>> &timings) noexcept;

  std::pair<Task, Task> genComputeTask(int taskId, OperatorTag opTag,
                                       OperatorType opType,
                                       const CPU_TCB &cpuTCB,
                                       const DPU_TCB &dpuTCB,
                                       execType eT) noexcept;

  std::pair<Task, Task> genXferTask(int taskId, OperatorTag opTag,
                                    const CPU_TCB &mapTCB,
                                    const CPU_TCB &reduceTCB,
                                    execType eT) noexcept;

  std::tuple<CPU_TCB,DPU_TCB,CPU_TCB,CPU_TCB> memPlan(const TaskGraph& g, int i,
                                                            uint32_t cpuPageBlkCnt,
                                                            uint32_t dpuPageBlkCnt,
                                                            uint32_t mapPageBlkCnt,
                                                            uint32_t reducePageBlkCnt){
   char* heapBasePtr = (char*)memPoolPtr[0];
   uint32_t dpuPageBaseIdx = 0;
   CPU_TCB cpuTCB{heapBasePtr, 
                  heapBasePtr + cpuPageBlkCnt * om.getPageBlkSize(),
                  heapBasePtr + 2 * cpuPageBlkCnt * om.getPageBlkSize(),
                  cpuPageBlkCnt};
   DPU_TCB dpuTCB{dpuPageBaseIdx,
                  dpuPageBaseIdx + dpuPageBlkCnt,
                  dpuPageBaseIdx + 2 * dpuPageBlkCnt,
                  dpuPageBlkCnt};
   CPU_TCB mapTCB;    mapTCB.sgInfo =   {heapBasePtr, dpuPageBaseIdx, mapPageBlkCnt}; 
   CPU_TCB reduceTCB; reduceTCB.sgInfo ={heapBasePtr, dpuPageBaseIdx, reducePageBlkCnt};
   return {cpuTCB, dpuTCB, mapTCB, reduceTCB};
   }

private:
  void** memPoolPtr;
  int memPoolNum = 3;
  double totalDPUTime_Second = 0.0f;
  const OperatorManager &om;
  std::mutex mutex_;
  std::mutex dpuMutex_;
  std::condition_variable cv_;
  std::vector<std::vector<int>> dependencies_;

  std::vector<completeSgn> cpuCompleted_, dpuCompleted_, mapCompleted_,
      reduceCompleted_;

  std::vector<double> cpuLastWakerTime_ms, dpuLastWakerTime_ms,
      mapLastWakerTime_ms, reduceLastWakerTime_ms;

  std::queue<Task> cpuQueue_, dpuQueue_, mapQueue_, reduceQueue_;

  std::unordered_map<int, std::vector<TaskTiming>> cpuTimings_, dpuTimings_,
      mapTimings_, reduceTimings_;
  // ---------- MIMIC mode statistics -------------

  double cpuMIMICTime_ms = 0.0f;
  double dpuMIMICTime_ms = 0.0f;

  double totalEnergyCost_joule = 0.0f;
  double totalTransfer_mb = 0.0f;
  ChronoTrigger ct;
};

} // namespace Executor
} // namespace MetaPB
#endif
