#ifndef TASK_GRAPH
#define TASK_GRAPH
#include "Executor/Task.hpp"
#include "Executor/graphTraits.hpp"
#include "utils/typedef.hpp"
#include "utils/Stats.hpp"
#include "boost/graph/topological_sort.hpp"

namespace MetaPB {
namespace Executor {

using OperatorTag = Operator::OperatorTag;
using Schedule = utils::Schedule;
using perfStats = utils::perfStats;
using regressionTask = utils::regressionTask;

class TaskGraph {
public:
  TaskGraph() = delete;
  TaskGraph(Graph gIn, const std::string n) : g(gIn), name(n) {}

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
  std::vector<int> topoSort()const noexcept;


  Graph g;
private:
  const std::string name;
}; // class TaskGraph

} // namespace Executor
} // namespace MetaPB
#endif
