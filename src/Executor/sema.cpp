#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <semaphore>
#include <chrono>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

// 全局计时器和互斥锁
std::atomic<std::chrono::milliseconds::rep> global_timer(0);

struct TaskProperties {
    int id;
    std::string name;
    std::chrono::milliseconds duration; // 任务的预测执行时间
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, TaskProperties> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor TaskNode;
typedef boost::graph_traits<Graph>::edge_descriptor TransferEdge;

class TaskExecutor {
public:
    TaskExecutor(const Graph& g) : graph(g) {
        // 初始化信号量
        for (auto vp = vertices(g); vp.first != vp.second; ++vp.first) {
            auto v = *vp.first;
            semaphores[v] = std::make_unique<std::counting_semaphore<1>>(0);
            // 初始化每个任务的开始时间为0
            task_start_times[v] = 0;
        }

        // 释放没有依赖的任务的信号量
        for (auto vp = vertices(g); vp.first != vp.second; ++vp.first) {
            auto v = *vp.first;
            if (boost::in_degree(v, g) == 0) {
                semaphores[v]->release();
                task_start_times[v] = global_timer.load();
            }
        }
    }

    void executeTask(TaskNode node) {
        auto& g = graph;
        // 等待所有依赖任务完成
        for (auto i = 0; i < boost::in_degree(node, g); ++i) {
            semaphores[node]->acquire();
        }

        // 获取任务被允许执行的开始时间
        auto start_time = task_start_times[node];
        auto predicted_end_time = start_time + g[node].duration.count();

        // 更新全局计时器
        updateGlobalTimer(predicted_end_time);

        log("任务 " + std::to_string(g[node].id) + " 执行完毕。");

        // 更新依赖此任务的节点的开始时间并释放信号量
        for (auto ep = boost::out_edges(node, g); ep.first != ep.second; ++ep.first) {
            auto target = boost::target(*ep.first, g);
            task_start_times[target] = std::max(task_start_times[target], predicted_end_time);
            semaphores[target]->release();
        }
    }

    void run() {
        std::vector<std::thread> threads;
        for (auto vp = vertices(graph); vp.first != vp.second; ++vp.first) {
            auto v = *vp.first;
            threads.emplace_back(&TaskExecutor::executeTask, this, v);
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

private:
    Graph graph;
    std::unordered_map<TaskNode, std::unique_ptr<std::counting_semaphore<1>>> semaphores;
    std::unordered_map<TaskNode, std::chrono::milliseconds::rep> task_start_times;
    std::mutex logMutex;

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << message << std::endl;
    }

    void updateGlobalTimer(std::chrono::milliseconds::rep predicted_end_time) {
        auto current_timer = global_timer.load();
        while (predicted_end_time > current_timer) {
            if (global_timer.compare_exchange_weak(current_timer, predicted_end_time)) {
                break;
            }
        }
    }
};

int main() {
    Graph g;

    // 创建图中的任务节点，并为每个任务设置预测执行时间
    TaskNode t0 = add_vertex({0, "Task 0", std::chrono::milliseconds(50)}, g);
    TaskNode t1 = add_vertex({1, "Task 1", std::chrono::milliseconds(100)}, g);
    TaskNode t2 = add_vertex({2, "Task 2", std::chrono::milliseconds(150)}, g);
    TaskNode t3 = add_vertex({3, "Task 3", std::chrono::milliseconds(200)}, g);
    TaskNode t4 = add_vertex({4, "Task 4", std::chrono::milliseconds(250)}, g);
    TaskNode t5 = add_vertex({5, "Task 5", std::chrono::milliseconds(300)}, g);

    // 添加任务之间的依赖关系
    add_edge(t0, t1, g);
    add_edge(t0, t2, g);
    add_edge(t1, t3, g);
    add_edge(t2, t3, g);
    add_edge(t3, t5, g);
    add_edge(t4, t5, g);

    // 执行任务
    TaskExecutor executor(g);
    executor.run();

    // 输出全局计时器的值
    std::cout << "全局计时器累计时间: " << global_timer.load() << "毫秒" << std::endl;

    return 0;
}
