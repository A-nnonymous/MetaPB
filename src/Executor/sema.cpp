#include <atomic>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_map>
#include <vector>

std::atomic<std::chrono::milliseconds::rep> global_timer(0);

struct TaskProperties {
  int id;
  std::string name;
  std::chrono::milliseconds duration; // 任务的预测执行时间
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                              TaskProperties>
    Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor TaskNode;

class TaskExecutor {
public:
  TaskExecutor(const Graph &g, size_t num_workers)
      : graph(g), workers(num_workers) {
    for (auto vp = vertices(g); vp.first != vp.second; ++vp.first) {
      auto v = *vp.first;
      int dependencies = boost::in_degree(v, g);
      semaphores[v] = std::make_unique<std::counting_semaphore<1>>(0);
      lastWakerTime[v] = 0;
    }
  }

  void executeTask(TaskNode node) {
    auto &g = graph;
    for (auto i = 0; i < boost::in_degree(node, g); ++i) {
      semaphores[node]->acquire();
    }

    // 任务开始前的全局时间
    auto start_time = lastWakerTime[node];
    log("node " + std::to_string(node) + "start at " +
        std::to_string(lastWakerTime[node]));
    auto predicted_end_time = start_time + g[node].duration.count();

    // 模拟任务执行
    // std::this_thread::sleep_for(g[node].duration);

    // 更新全局计时器为最晚的结束时间
    updateGlobalTimer(predicted_end_time);

    log("任务 " + std::to_string(g[node].id) + " 执行完毕。");

    // 释放依赖此任务的节点的信号量
    for (auto ep = boost::out_edges(node, g); ep.first != ep.second;
         ++ep.first) {
      auto target = boost::target(*ep.first, g);
      lastWakerTime[target] =
          std::max(lastWakerTime[target], predicted_end_time);
      semaphores[target]->release();
    }
  }

  void run() {
    std::vector<std::thread> threads;
    auto num_vertices = boost::num_vertices(graph);

    std::atomic<size_t> index(0);
    auto task_exec = [&] {
      while (true) {
        size_t i = index.fetch_add(1);
        if (i >= num_vertices)
          break;
        executeTask(i);
      }
    };

    for (size_t i = 0; i < workers; ++i) {
      threads.emplace_back(task_exec);
    }

    for (auto &thread : threads) {
      thread.join();
    }
  }

private:
  Graph graph;
  size_t workers;
  std::unordered_map<TaskNode, std::unique_ptr<std::counting_semaphore<1>>>
      semaphores;
  std::unordered_map<TaskNode, std::chrono::milliseconds::rep> lastWakerTime;
  std::mutex logMutex;

  void log(const std::string &message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << message << std::endl;
  }

  void updateGlobalTimer(std::chrono::milliseconds::rep predicted_end_time) {
    auto current_timer = global_timer.load();
    while (predicted_end_time > current_timer) {
      if (global_timer.compare_exchange_weak(current_timer,
                                             predicted_end_time)) {
        std::cout << "Global timer updated from " << current_timer << " to "
                  << predicted_end_time << "\n";
        break;
      }
    }
  }
};

int main() {
  Graph g;

  TaskNode t0 = add_vertex({0, "Task 0", std::chrono::milliseconds(50)}, g);
  TaskNode t1 = add_vertex({1, "Task 1", std::chrono::milliseconds(100)}, g);
  TaskNode t2 = add_vertex({2, "Task 2", std::chrono::milliseconds(150)}, g);
  TaskNode t3 = add_vertex({3, "Task 3", std::chrono::milliseconds(200)}, g);
  TaskNode t4 = add_vertex({4, "Task 4", std::chrono::milliseconds(250)}, g);
  TaskNode t5 = add_vertex({5, "Task 5", std::chrono::milliseconds(300)}, g);

  add_edge(t0, t1, g);
  add_edge(t0, t2, g);
  add_edge(t1, t3, g);
  add_edge(t2, t3, g);
  add_edge(t3, t5, g);
  add_edge(t4, t5, g);

  size_t num_workers = 4; // 设置工作线程数量
  TaskExecutor executor(g, num_workers);
  executor.run();

  std::cout << "全局计时器累计时间: " << global_timer.load() << "毫秒"
            << std::endl;

  return 0;
}
