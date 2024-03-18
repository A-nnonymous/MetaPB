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

void HeteroComputePool::parseWorkload(TaskGraph &g, const Schedule &sched) noexcept {
    // 清除以前的依赖关系
    for (auto &deps : dependencies_) {
        deps.clear();
    }

    TaskProperties& tp1 = g.g[1];
    size_t wholeSize = tp1.inputSize_MiB; // 假设这是批处理大小
    void* src1 = malloc((1<<20) * wholeSize);
    void* src2 = malloc((1<<20) * wholeSize);
    void* dst1 = malloc((1<<20) * wholeSize);
    void* bfr[3] = {src1, src2, dst1};
    memPoolPtr = bfr;

    // 遍历调度中的任务
    for (size_t i = 0; i < sched.order.size(); ++i) {
        int taskId = sched.order[i];
        float offloadRatio = sched.offloadRatio[i];

        // 获取当前任务的后续节点的offloadRatio
        float omax = 0.0f; // 后续节点的最大offloadRatio
        float omin = 0.0f; // 后续节点的最小offloadRatio
        std::vector<int> successors; // 用于存储后续节点的ID

        auto outEdges = boost::out_edges(taskId, g.g);
        for (auto ei = outEdges.first; ei != outEdges.second; ++ei) {
            TaskNode succ = boost::target(*ei, g.g);
            float succOffloadRatio = sched.offloadRatio[succ];
            omax = std::max(omax, succOffloadRatio);
            omin = std::min(omin, succOffloadRatio);
            successors.push_back(succ);
        }

        // 更新依赖项
        auto inEdges = boost::in_edges(taskId, g.g);
        for (auto ei = inEdges.first; ei != inEdges.second; ++ei) {
            TaskNode pred = boost::source(*ei, g.g);
            dependencies_[taskId].push_back(pred);
        }

        // 创建任务并分配到队列
        TaskProperties& tp = g.g[taskId];
        float batchSize_MiB = tp.inputSize_MiB; // 假设这是批处理大小

        Task cpuTask{
            taskId,
            [this, op=tp.op, batchSize=size_t(batchSize_MiB*(1-offloadRatio))]() {
                om.execCPU(op, batchSize, this->memPoolPtr);
            },
            "CPU"
        };

        Task dpuTask{
            taskId,
            [this, op=tp.op, batchSize = size_t(batchSize_MiB * offloadRatio)]() {
                om.execDPU(op, batchSize);
            },
            "DPU"
        };

        Task mapTask{
            taskId,
            []() {}, // 初始化为空执行函数
            "MAP"
        };

        Task reduceTask{
            taskId,
            []() {}, // 初始化为空执行函数
            "REDUCE"
        };

        if (offloadRatio > omax) {
            size_t reduce_work=0;
            reduceTask.execute = [this, reduce_work]() {
                om.execCPU(OperatorTag::REDUCE, reduce_work, this->memPoolPtr);
            };
        } else if (offloadRatio > omin && offloadRatio < omax) {
            size_t reduce_work=0;
            reduceTask.execute = [this, reduce_work]() {
                om.execCPU(OperatorTag::REDUCE, reduce_work,this->memPoolPtr);
            };
            size_t map_work =0;
            mapTask.execute = [this, map_work]() {
                om.execCPU(OperatorTag::MAP, map_work, this->memPoolPtr);
            };
        } else {
            size_t map_work =0;
            mapTask.execute = [this, map_work]() {
                om.execCPU(OperatorTag::MAP, map_work, this->memPoolPtr);
            };
        }

        // 将任务添加到队列
        cpuQueue_.push(cpuTask);
        dpuQueue_.push(dpuTask);
        mapQueue_.push(mapTask);
        reduceQueue_.push(reduceTask);
    }
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