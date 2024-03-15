#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#include "Executor/Task.hpp"
#include "Operator/OperatorManager.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using std::map;
using Graph = MetaPB::Executor::Task::Graph;
using Vertex = MetaPB::Executor::Task::Vertex;
using Edge = MetaPB::Executor::Task::Edge;

// TODO: Finish the developing of these Traits.
using OPTraits = MetaPB::Operator::OperatorTraits;
using TransferTraits = MetaPB::Operator::TransferTraits;

namespace MetaPB {
namespace utils {

class vertex_writer {
public:
  vertex_writer(){};
  template <class Vertex>
  void operator()(std::ostream &out, const Vertex &) const {
    out << "[style=filled"
        << "]";
  }
};

template <class OPTrait2Attr> class vertex_writer_w_traits {
public:
  vertex_writer_w_traits(const OPTrait2Attr &in) : attrMaps(in) {}

  template <class Vertex>
  void operator()(std::ostream &out, const Vertex &v) const {
    out << "[label=\"" << attrMaps.name.at(v) << "\", shape=circle";
    out << ", style=filled, fillcolor=\"" << attrMaps.color.at(v) << "\"]";
  }

private:
  const OPTrait2Attr &attrMaps;
};

class edge_writer {
public:
  template <class Edge> void operator()(std::ostream &out, const Edge &) const {
    out << "[style=curved]";
  }
};

class edge_writer_w_transfer_sgn {
public:
  edge_writer_w_transfer_sgn(const &mapin) : attrMap(in) {}

  template <class Edge>
  void operator()(std::ostream &out, const Edge &e) const {
    // out << "[style=curved]";
    // TODO: Add logic dependence as blue, data dependence as red.
    out << "[style=curved,"
        << "fillcolor=\"" << attrMap.type.at(e) << "\""
        << "]";
  }

private:
  const EdgeTraits2Attr &attrMap;
};

class graph_writer {
public:
  void operator()(std::ostream &out) const {
    out << "rankdir=TB;\n"
        << "node [fontname=\"JetBrains Mono\"];\n"
        << "edge [fontname=\"JetBrains Mono\"];\n";
  }
};

size_t find_max_node(const std::string &filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open()) {
    std::cerr << "Unable to open file: " << filename << std::endl;
    return -1;
  }

  std::string line;
  int max_node = 0;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string start_node, end_node;
    if (!(std::getline(iss, start_node, ',') && std::getline(iss, end_node))) {
      continue; // Skip malformed lines
    }
    max_node = std::max(max_node,
                        std::max(std::stoi(start_node), std::stoi(end_node)));
  }
  return max_node;
}

void saveGraph(const Graph &g, const std::string &filePath,
               const map<Vertex, OPTraits> &vMap,
               const map<Edge, TransferTraits> &eMap) {
  std::ofstream fout(filePath + "graph.dot");

  boost::write_graphviz(
      fout, g, vertex_writer_w_traits<map<Vertex, OPTraits>>(vMap),
      edge_writer_w_transfer_sgn<map<Edge, TransferTraits>>(eMap),
      graph_writer());
}

} // namespace utils
} // namespace MetaPB