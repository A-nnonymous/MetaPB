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
  ThreadPool(size_t num_threads);
  void join();

  ~ThreadPool();

  void enqueue(Task task);

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