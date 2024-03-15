#include "Executor/TaskGraph.hpp"

namespace MetaPB{
namespace Executor{

void TaskGraph::traverse() {
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    std::cout <<"In graph "<<name<< ", node " << *vi << " is " << g[*vi].name << "\n";
  }
}
// -----------MetaPB related functions -----------
// Generate regression task according to op set and batch size.
regressionTask TaskGraph::genRegressionTask(size_t){
  return {};
}
// Using regression model to predict the performance metrics
// of a specific schedule, batchsize.
perfStats TaskGraph::deduceMetrics(const Schedule&, size_t){
  return {};
}
// Execute the whole graph with specific schedule
perfStats TaskGraph::exec(const Schedule&, size_t){
  return {};
}

void TaskGraph::printGraph(const std::string & filePath)const noexcept{
  auto dotPath = filePath + this->name + ".dot";
  std::ofstream dot_file(dotPath);
  boost::write_graphviz(dot_file, g, op_property_writer(g),
                        xfer_property_writer(g), graph_property_writer());
}
  // -----------MetaPB related functions -----------

} // namespace Executor
} // namespace MetaPB