#ifdef TODO_HETERO
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// task itself is a wrapper of function
// when task executed 
using Task = std::function<void()>;

class CPUSlave {
public:
  void execute(Task task) {}
};

class DPUSlave {
public:
  void execute(Task task) {}
};

class HeteroComputePool {
public:
  HeteroComputePool() : stop_requested(false) {
    master = std::thread(&HeteroComputePool::masterThread, this);
  }

  ~HeteroComputePool() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      stop_requested = true;
      cv.notify_all();
    }
    if (master.joinable()) {
      master.join();
    }
  }

  void enqueue(Task task) {
    std::lock_guard<std::mutex> lock(mutex);
    enqueue_unsage(task);
    cv.notify_one();
  }

  void enqueue_unsafe(Task task){
    tasks.push(std::move(task));
  }


private:
  std::thread master;
  CPUSlave cpuSlave;
  DPUSlave dpuSlave;
  std::queue<Task> tasks;
  std::mutex mutex;
  std::condition_variable cv;
  bool stop_requested;

  void masterThread() {
    while (true) {
      Task task;
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return stop_requested || !tasks.empty(); });
        if (stop_requested && tasks.empty()) {
          break;
        }

        task = std::move(tasks.front());
        tasks.pop();
      }
      const auto [cpuTask, dpuTask] = split(task);
        cpuSlave.execute(cpuTask);
        dpuSlave.execute(dpuTask);
      }
    }
  };
#endif
