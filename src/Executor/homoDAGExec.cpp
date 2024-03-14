#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/topological_sort.hpp>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using Task = std::function<void()>;
using Graph = boost::adjacency_list<boost::vecS, boost::vecS,
                                    boost::bidirectionalS, 
                                    boost::no_property,
                                    boost::property<boost::edge_weight_t, 
                                    int>>;
using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

class ThreadPool {
public:
  ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
      threads.emplace_back([this] {
        while (true) {
          Task task;
          { // -------------- Critical Zone --------------------
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock,
                [this] { return stop_requested || !tasks.empty(); });

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
  void join() { // manual join
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

  ~ThreadPool() { this->join(); }

  void enqueue(Task task) { // enquque task
    {
      std::lock_guard<std::mutex> lock(mutex);
      tasks.push(std::move(task));
    }
    cv.notify_one();
  }

private:
  std::vector<std::thread> threads;
  std::queue<Task> tasks;
  std::mutex mutex;
  std::condition_variable cv;
  bool stop_requested = false;
};

struct TaskLog {
  int task_id;
  std::thread::id exec_thrID;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
};

std::mutex log_mutex;
std::vector<TaskLog> task_logs;

void execute_task(Vertex v, Graph &g,
                  std::vector<std::atomic<int>> &in_degree_counter,
                  int sleep_time) {
  while (in_degree_counter[v] > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto start_time = std::chrono::steady_clock::now();

  std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

  auto end_time = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(log_mutex);
    task_logs.push_back(
        {int(v), std::this_thread::get_id(), start_time, end_time});
  }

  Graph::adjacency_iterator ai, ai_end;
  for (boost::tie(ai, ai_end) = adjacent_vertices(v, g); ai != ai_end; ++ai) {
    --in_degree_counter[*ai];
  }
  std::cout << "Task " << v << " completed after sleeping for " << sleep_time
            << " seconds.\n";
}

int main() {
  Graph g;

  auto v0 = add_vertex(g);
  auto v1 = add_vertex(g);
  auto v2 = add_vertex(g);
  auto v3 = add_vertex(g);

  add_edge(v0, v1, g);
  add_edge(v1, v2, g);
  add_edge(v0, v3, g);
  std::vector<std::atomic<int>> in_degree_counter(num_vertices(g));
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    in_degree_counter[*vi] = boost::in_degree(*vi, g);
  }

  // Graph output
  std::ofstream dot_file("graph.dot");
  boost::write_graphviz(dot_file, g);

  // Topological sort before task enqueing
  std::vector<Graph::vertex_descriptor> order;
  boost::topological_sort(g, std::back_inserter(order));
  ThreadPool pool(2); 

  for (auto v = order.rbegin(); v != order.rend(); ++v) {
    pool.enqueue([v, &g, &in_degree_counter] {
      execute_task(*v, g, in_degree_counter, 1);
    });
  }
  pool.join();

  std::ofstream log_file("task_logs.txt");
  for (const auto &log : task_logs) {
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        log.start_time.time_since_epoch())
                        .count();
    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      log.end_time.time_since_epoch())
                      .count();
    log_file << "Task " << log.task_id << " Executed on: " << log.exec_thrID
             << " started at " << start_ms << " ms and ended at " << end_ms
             << " ms.\n";
  }

  return 0;
}
