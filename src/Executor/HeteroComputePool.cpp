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
    std::unique_lock<std::mutex> mapReduceLock(
        type == "MAP" || type == "REDUCE" ? mapReduceMutex_ : mutex_,
        std::defer_lock);
    if (type == "MAP" || type == "REDUCE") {
      mapReduceLock.lock();
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
    if (type == "MAP" || type == "REDUCE") {
      mapReduceLock.unlock();
    }
  }
}

void HeteroComputePool::parseWorkload(const TaskGraph& g,
                                      const Schedule& sched) noexcept{

}

/*
// Adds a task with random execution length to all queues.
void archive::addTask(int id, std::vector<int> deps) noexcept {
  if (id >= dependencies_.size()) {
    int newSize = id + 1;
    cpuCompleted_.resize(newSize, false);
    dpuCompleted_.resize(newSize, false);
    mapCompleted_.resize(newSize, false);
    reduceCompleted_.resize(newSize, false);
    dependencies_.resize(newSize);
  }

  dependencies_[id] = std::move(deps);
  auto execFunc = [id, this]() { simulateTaskExecution(id, dis_(gen_)); };

  // Push tasks into the respective queues
  cpuQueue_.push({id, execFunc, "CPU"});
  dpuQueue_.push({id, execFunc, "DPU"});
  mapQueue_.push({id, execFunc, "MAP"});
  reduceQueue_.push({id, execFunc, "REDUCE"});
}
*/

// Starts the worker threads to process the tasks.
void HeteroComputePool::start() noexcept {
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
                             .count() /
                         1000.0;
            auto end = std::chrono::duration_cast<std::chrono::milliseconds>(
                           timing.end - minStart)
                           .count() /
                       1000.0;
            csvFile << pair.first << "," << type << "," << std::fixed
                    << std::setprecision(3) << start << "," << std::fixed
                    << std::setprecision(3) << end << "\n";
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