#ifndef GRAPH_TRAIT
#define GRAPH_TRAIT
#include <map>
#include <string>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include "Operator/OperatorManager.hpp"
#include "Scheduler/SchedulerManager.hpp"
#include "Executor/Task.hpp"
#include "Executor/Transfer.hpp"

namespace MetaPB{
using OperatorTag = Operator::OperatorTag;
using OperatorType = Operator::OperatorType;
using Schedule = Scheduler::Schedule;

namespace Executor{

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS, 
                              boost::bidirectionalS,
                              TaskProperties, 
                              TransferProperties> Graph;

typedef typename boost::graph_traits<Graph>::vertex_descriptor Task;

// --------------------------- Graphviz related ----------------------------
/// @brief Control the output graph nodes' attributes
class op_property_writer {
public:
  op_property_writer(const Graph &g) : g_(g) {}
  template <class Vertex>
  void operator()(std::ostream &out, const Vertex &v) const {
    out << "[label=\"" << g_[v].name << "\", color=\"" << opType2Color[g_[v].opType]<< "\"]";
  }
private:
  const Graph &g_;
  inline static std::map<OperatorType, std::string> opType2Color = {
    {OperatorType::CoumputeBound, "green"},
    {OperatorType::MemoryBound, "red"},
    {OperatorType::Logical, "blue"},
    {OperatorType::Map, "purple"},
    {OperatorType::Reduce, "orange"},
    {OperatorType::Undefined, "grey"}
  };
};

/// @brief Control the output graph edges' attributes
class xfer_property_writer {
public:
  xfer_property_writer(const Graph &g) : g_(g) {}

  template <class Edge>
  void operator()(std::ostream &out, const Edge &e) const {
    out << "[label=\"" << g_[e].dataSize_Ratio<< "\","<< "style=\"solid\"]";
  }
private:
  const Graph &g_;
};

/// @brief Control the graph global attributes
struct graph_property_writer {
    void operator()(std::ostream& out) const {
        out << "graph [splines=false, rankdir=TB, nodesep=0.2, ranksep=0.4, concentrate=true];\n";
    }
};

} // namespace Executor
} // namespace MetaPB

#endif