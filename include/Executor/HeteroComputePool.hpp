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
#include <vector>

namespace MetaPB {
namespace Executor {
using Operator::OperatorManager;
using Operator::OperatorTag;
using Operator::opType2Name;
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
  int id;
  std::function<void()> execute;
  std::string worker;
  std::string opType;
} Task;

enum class execType { MIMIC, DO };

enum class EUType { CPU, DPU };

typedef std::tuple<EUType, OperatorTag, size_t> perfTag;

class HeteroComputePool {
public:
  // Constructor initializes the pool with the expected maximum task ID.
  HeteroComputePool(size_t maxTaskId, const OperatorManager &om,
                    void **memPoolPtr) noexcept
      : cpuCompleted_(maxTaskId + 1, false),
        dpuCompleted_(maxTaskId + 1, false),
        mapCompleted_(maxTaskId + 1, false),
        reduceCompleted_(maxTaskId + 1, false), dependencies_(maxTaskId + 1),
        gen_(rd_()), dis_(100, 500), om(om), memPoolPtr(memPoolPtr) {}

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
        earliestCPUIdleTime_ms(
            std::exchange(other.earliestCPUIdleTime_ms, 0.0)),
        earliestDPUIdleTime_ms(
            std::exchange(other.earliestDPUIdleTime_ms, 0.0)),
        earliestMAPIdleTime_ms(
            std::exchange(other.earliestMAPIdleTime_ms, 0.0)),
        earliestREDUCEIdleTime_ms(
            std::exchange(other.earliestREDUCEIdleTime_ms, 0.0)),
        totalEnergyCost_joule(std::exchange(other.totalEnergyCost_joule, 0.0)),
        totalTransfer_mb(std::exchange(other.totalTransfer_mb, 0.0)), rd_(),
        gen_(std::move(other.gen_)), dis_(std::move(other.dis_)) {}
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
  bool allDependenciesMet(const std::vector<bool> &completedVector,
                          const std::vector<int> &deps) const noexcept;


  // Generic worker function for processing tasks from a queue
  void processTasks(std::queue<Task> &queue, std::vector<bool> &completedVector,
                    std::function<bool(int)> dependencyCheck,
                    std::unordered_map<int, std::vector<TaskTiming>> &timings,
                    const std::string &type) noexcept;

private:
  void **memPoolPtr;
  int memPoolNum = 3;
  double totalDPUTime_Second = 0.0f;
  const OperatorManager &om;
  ChronoTrigger ct;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::mutex dpuMutex_;
  std::vector<std::vector<int>> dependencies_;

  std::vector<bool> cpuCompleted_, dpuCompleted_, mapCompleted_,
      reduceCompleted_;

  std::queue<Task> cpuQueue_, dpuQueue_, mapQueue_, reduceQueue_;

  std::unordered_map<int, std::vector<TaskTiming>> cpuTimings_, dpuTimings_,
      mapTimings_, reduceTimings_;
  // ---------- MIMIC mode statistics -------------
  double earliestCPUIdleTime_ms = 0.0f;
  double earliestDPUIdleTime_ms = 0.0f;
  double earliestMAPIdleTime_ms = 0.0f;
  double earliestREDUCEIdleTime_ms = 0.0f;
  bool isCPUStalled = false;
  bool isDPUStalled = false;

  double totalEnergyCost_joule = 0.0f;
  double totalTransfer_mb = 0.0f;
  std::map<perfTag, perfStats> cachedCPUPerfDeduce;
  std::map<perfTag, perfStats> cachedDPUPerfDeduce;
  std::random_device rd_;
  std::mt19937 gen_;
  std::uniform_int_distribution<> dis_;
};

} // namespace Executor
} // namespace MetaPB
#endif
