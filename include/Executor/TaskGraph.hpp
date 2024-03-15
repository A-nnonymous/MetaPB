#ifndef TASK_GRAPH
#define TASK_GRAPH
#include "Operator/OperatorRegistry.hpp"
#include "Executor/graphTraits.hpp"
#include "Executor/Task.hpp"

namespace MetaPB{
namespace Executor{
using OperatorTag = Operator::OperatorTag;
using Schedule = Scheduler::Schedule;
typedef std::map<OperatorTag, size_t> regressionTask;
typedef struct perfStats{
  double energyCost_Joule;
  double timeCost_Second;
  size_t dataMovement_GiB;
}perfStats;

class TaskGraph {
public:
  TaskGraph() = delete;
  TaskGraph(Graph gIn, const std::string n): g(gIn), name(n){}

  void traverse() {
    Graph::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
      std::cout <<"In graph "<<name<< ", node " << *vi << " is " << g[*vi].name << "\n";
    }
  }
  // -----------MetaPB related functions -----------
  // Generate regression task according to op set and batch size.
  regressionTask genRegressionTask(size_t);
  // Using regression model to predict the performance metrics
  // of a specific schedule, batchsize.
  perfStats deduceMetrics(const Schedule&, size_t);
  // Execute the whole graph with specific schedule
  perfStats exec(const Schedule&, size_t);

  // -----------MetaPB related functions -----------
private:
  Graph g;
  const std::string name;
}; // class TaskGraph

} // namespace Executor
} // namespace MetaPB
#endif