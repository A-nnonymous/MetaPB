#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Executor {
// TaskTiming struct to store start and end times for tasks
typedef struct TaskTiming {
  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point end;
}TaskTiming;

// Task struct to store task ID and execution function
typedef struct  {
  int id;
  std::function<void()> execute;
  std::string type; // To distinguish between CPU, DPU, MAP, and REDUCE tasks
}Task;

class HeteroComputePool {

public:
  // Constructor initializes the pool with the expected maximum task ID.
  HeteroComputePool(int maxTaskId) noexcept
      : cpuCompleted_(maxTaskId + 1, false),
        dpuCompleted_(maxTaskId + 1, false),
        mapCompleted_(maxTaskId + 1, false),
        reduceCompleted_(maxTaskId + 1, false), dependencies_(maxTaskId + 1),
        gen_(rd_()), dis_(100, 500) {}

  void parseWorkload(const TaskGraph& g, const Schedule& sched) noexcept;

  // Starts the worker threads to process the tasks.
  void start() noexcept;

  // Print timings for each type of task
  void printTimings() const noexcept;
  void outputTimingsToCSV(const std::string &filename) const noexcept;

private:
  // Helper function to print timings for a specific type of task
  void
  printTimingsForType(const std::string &type,
                      const std::unordered_map<int,
                      std::vector<TaskTiming>> &timings) const noexcept;
  // Check if all dependencies for a task are met
  bool allDependenciesMet(const std::vector<bool> &completedVector,
                          const std::vector<int> &deps) const noexcept;

  // Generic worker function for processing tasks from a queue
  void processTasks(std::queue<Task> &queue, 
                    std::vector<bool> &completedVector,
                    std::function<bool(int)> dependencyCheck,
                    std::unordered_map<int, std::vector<TaskTiming>> &timings,
                    const std::string &type) noexcept;

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::mutex mapReduceMutex_;
  std::vector<std::vector<int>> dependencies_;

  std::vector<bool> cpuCompleted_,
                    dpuCompleted_,
                    mapCompleted_,
                    reduceCompleted_;

  std::queue<Task> cpuQueue_, 
                   dpuQueue_,
                   mapQueue_,
                   reduceQueue_;

  std::unordered_map<int, std::vector<TaskTiming>> cpuTimings_,
                                                   dpuTimings_, 
                                                   mapTimings_, 
                                                   reduceTimings_;

  std::random_device rd_;
  std::mt19937 gen_;
  std::uniform_int_distribution<> dis_;
};

} // namespace Executor
} // namespace MetaPB