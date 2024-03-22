#ifndef TASK_GRAPH
#define TASK_GRAPH
#include "Executor/Task.hpp"
#include "Executor/graphTraits.hpp"
#include "boost/graph/topological_sort.hpp"
#include "utils/Stats.hpp"
#include "utils/typedef.hpp"

namespace MetaPB {
namespace Executor {

using OperatorTag = Operator::OperatorTag;
using Schedule = utils::Schedule;
using perfStats = utils::perfStats;
using regressionTask = utils::regressionTask;

class TaskGraph {
public:
  TaskGraph(){}
  TaskGraph(Graph gIn, const std::string n) : g(gIn), name(n) {}
    TaskGraph(const TaskGraph& other) noexcept
    : g(std::move(other.g)), 
      name(std::move(other.name)) {}
  TaskGraph& operator=(TaskGraph&& other) noexcept {
    if (this != &other) { // 防止自赋值
      g = std::move(other.g); // 移动Graph对象
      this->name = other.name;
    }
    return *this; // 返回当前对象的引用
  }
  void traverse();
  // -----------MetaPB related functions -----------
  // Generate regression task according to op set and batch size.
  regressionTask genRegressionTask();
  // Using regression model to predict the performance metrics
  // of a specific schedule, batchSize_MiB.
  perfStats deduceMetrics(const Schedule &, size_t);
  // Execute the whole graph with specific schedule
  perfStats exec(const Schedule &, size_t);

  // -----------MetaPB related functions -----------
  void printGraph(const std::string &filePath) const noexcept;
  std::vector<int> topoSort() const noexcept;

  Graph g;

private:
  std::string name;
}; // class TaskGraph

} // namespace Executor
} // namespace MetaPB
#endif
