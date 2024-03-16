#include "Executor/homoDAGExec.hpp"

ThreadPool::ThreadPool(size_t num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    threads.emplace_back([this] {
      while (true) {
        Task task;
        { // -------------- Critical Zone --------------------
          std::unique_lock<std::mutex> lock(mutex);
          cv.wait(lock, [this] { return stop_requested || !tasks.empty(); });

          if (stop_requested && tasks.empty()) { // quit execution
            break;
          }

          task = std::move(tasks.front());
          tasks.pop();
        } // -------------- Critical Zone --------------------
        task();
      }
    });
  }
}
void ThreadPool::join() { // manual join
  {
    std::lock_guard<std::mutex> lock(mutex);
    stop_requested = true;
  }
  cv.notify_all();
  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

ThreadPool::~ThreadPool() { this->join(); }

void ThreadPool::enqueue(Task task) { // enquque task
  {
    std::lock_guard<std::mutex> lock(mutex);
    tasks.push(std::move(task));
  }
  cv.notify_one();
}
