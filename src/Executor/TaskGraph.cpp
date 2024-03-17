#include "Executor/TaskGraph.hpp"

namespace MetaPB {
namespace Executor {

void TaskGraph::traverse() {
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    std::cout << "In graph " << name << ", node " << *vi << " is "
              << g[*vi].name << "\n";
  }
}
// -----------MetaPB related functions -----------
// Generate regression task according to op set and batch size.
regressionTask TaskGraph::genRegressionTask() {
  regressionTask rt;
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    if(!rt.contains(g[*vi].op)){
      rt[g[*vi].op] = g[*vi].inputSize_MiB;
    }else{
      if(g[*vi].inputSize_MiB > rt[g[*vi].op]) rt[g[*vi].op] = g[*vi].inputSize_MiB;
    }
  }
  return rt;
}
// Using regression model to predict the performance metrics
// of a specific schedule, batchSize_MiB.
perfStats TaskGraph::deduceMetrics(const Schedule &, size_t) { return {}; }
// Execute the whole graph with specific schedule
perfStats TaskGraph::exec(const Schedule &, size_t) { return {}; }

void TaskGraph::printGraph(const std::string &filePath) const noexcept {
  auto dotPath = filePath + this->name + ".dot";
  std::ofstream dot_file(dotPath);
  boost::write_graphviz(dot_file, g, op_property_writer(g),
                        xfer_property_writer(g), graph_property_writer());
}

std::vector<int> TaskGraph::topoSort()const noexcept{
      // Create a vector to store the topological sort result
    std::vector<TaskNode> topo_order;
    // Perform topological sort
    try {
        boost::topological_sort(g, std::back_inserter(topo_order));
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during topological sort: " << e.what() << std::endl;
    }

    // Reverse the vector to get the correct order
    std::reverse(topo_order.begin(), topo_order.end());

    // Convert the topological order to the desired format
    std::vector<int> order(topo_order.begin(), topo_order.end());
    return order;
} 
// -----------MetaPB related functions -----------

} // namespace Executor
} // namespace MetaPB