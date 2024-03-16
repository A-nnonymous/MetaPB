#ifndef TASK_GRAPH
#define TASK_GRAPH
#include "Executor/Task.hpp"
#include "Executor/graphTraits.hpp"
#include "utils/Stats.hpp"

namespace MetaPB {
namespace Executor {

using OperatorTag = Operator::OperatorTag;
using Schedule = Scheduler::Schedule;
using perfStats = utils::perfStats;

typedef std::map<OperatorTag, size_t> regressionTask;

class TaskGraph {
public:
  TaskGraph() = delete;
  TaskGraph(Graph gIn, const std::string n) : g(gIn), name(n) {}

  void traverse();
  // -----------MetaPB related functions -----------
  // Generate regression task according to op set and batch size.
  regressionTask genRegressionTask(size_t);
  // Using regression model to predict the performance metrics
  // of a specific schedule, batchSize_MiB.
  perfStats deduceMetrics(const Schedule &, size_t);
  // Execute the whole graph with specific schedule
  perfStats exec(const Schedule &, size_t);

  void printGraph(const std::string &filePath) const noexcept;
  // -----------MetaPB related functions -----------
private:
  Graph g;
  const std::string name;
}; // class TaskGraph

} // namespace Executor
} // namespace MetaPB
#endif