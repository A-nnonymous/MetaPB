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

namespace MetaPB{
using OperatorTag = Operator::OperatorTag;
using OperatorType = Operator::OperatorType;
using Schedule = Scheduler::Schedule;

namespace Executor{


typedef struct TransferProperties {
  double dataSize_Ratio = 1.0f;
  // ----- Schedule adjust zone ------
  bool isNeedTransfer = false;
  double prevDRAMRatio = 1.0f;
  double nextDRAMRatio = 1.0f;
  // ----- Schedule adjust zone ------
} TransferProperties;

typedef boost::adjacency_list<boost::vecS,
                              boost::vecS, 
                              boost::bidirectionalS,
                              TaskProperties, 
                              TransferProperties> Graph;

typedef typename boost::graph_traits<Graph>::vertex_descriptor Task;

/// @brief Control the output graph nodes' attributes
class op_property_writer {
public:
  op_property_writer(Graph &g) : g_(g) {}
  template <class Vertex>
  void operator()(std::ostream &out, const Vertex &v) const {
    out << "[label=\"" << g_[v].name << "\", color=\"" << opType2Color[g_[v].opType]<< "\"]";
  }
private:
  Graph &g_;
  inline static std::map<OperatorType, std::string> opType2Color = {
    {OperatorType::CoumputeBound, "red"},
    {OperatorType::MemoryBound, "blue"},
    {OperatorType::Logical, "yello"},
    {OperatorType::Map, "green"},
    {OperatorType::Reduce, "orange"},
    {OperatorType::Undefined, "grey"}
  };
};

/// @brief Control the output graph edges' attributes
class xfer_property_writer {
public:
  xfer_property_writer(Graph &g) : g_(g) {}

  template <class Edge>
  void operator()(std::ostream &out, const Edge &e) const {
    out << "[label=\"" << g_[e].dataSize_Ratio<< "\"]";
  }
private:
  Graph &g_;
};

} // namespace Executor
} // namespace MetaPB

#endif