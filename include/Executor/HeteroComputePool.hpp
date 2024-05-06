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
using Operator::OperatorManager;
using Operator::OperatorTag;
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
  int id;
  std::function<void()> execute;
  bool isDPURelated;
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
  HeteroComputePool(size_t maxTaskId, const OperatorManager &om,
                    void **memPoolPtr) noexcept
      /*
        : cpuCompleted_(maxTaskId + 1, {false,0.0f}),
          dpuCompleted_(maxTaskId + 1, {false,0.0f}),
          mapCompleted_(maxTaskId + 1, {false,0.0f}),
          reduceCompleted_(maxTaskId + 1, {false,0.0f}),
          cpuLastWakerTime_ms(maxTaskId + 1 , 0.0f),
          dpuLastWakerTime_ms(maxTaskId + 1 , 0.0f),
          mapLastWakerTime_ms(maxTaskId + 1 , 0.0f),
          reduceLastWakerTime_ms(maxTaskId + 1 , 0.0f),
          dependencies_(maxTaskId + 1),
          */
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
                          double &lastWakerTime_ms) const noexcept;

  // Generic worker function for processing tasks from a queue
  void processTasks(
      std::queue<Task> &queue, std::vector<completeSgn> &completedVector,
      std::function<bool(int)> dependencyCheck,
      std::unordered_map<int, std::vector<TaskTiming>> &timings) noexcept;

  std::pair<Task, Task> genComputeTask(int taskId, OperatorTag opTag,
                                       OperatorType opType,
                                       size_t inputSize_MiB, float offloadRatio,
                                       execType eT) noexcept;

  std::pair<Task, Task> genXferTask(int taskId, OperatorTag opTag,
                                    size_t mapWork_MiB, size_t reduceWork_MiB,
                                    execType eT) noexcept;

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

  std::vector<completeSgn> cpuCompleted_, dpuCompleted_, mapCompleted_,
      reduceCompleted_;

  std::queue<Task> cpuQueue_, dpuQueue_, mapQueue_, reduceQueue_;

  std::unordered_map<int, std::vector<TaskTiming>> cpuTimings_, dpuTimings_,
      mapTimings_, reduceTimings_;
  // ---------- MIMIC mode statistics -------------
  std::vector<double> cpuLastWakerTime_ms, dpuLastWakerTime_ms,
      mapLastWakerTime_ms, reduceLastWakerTime_ms;

  double cpuMIMICTime_ms = 0.0f;
  double dpuMIMICTime_ms = 0.0f;

  double totalEnergyCost_joule = 0.0f;
  double totalTransfer_mb = 0.0f;
};

} // namespace Executor
} // namespace MetaPB
#endif
