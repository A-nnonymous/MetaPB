#include "graphTraits.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
using std::string;

inline const std::string getCordinateStr(int row, int col) noexcept{
  return "[" + std::to_string(row) + "," + std::to_string(col) + "]";
}

TaskGraph genGEA(int matrixSize, size_t batchSize_MiB) {
  int N = matrixSize;
  Graph g;

  TaskProperties geaNode = {OperatorTag::MAC,
                            OperatorType::ComputeBound, 
                            4096,
                            "blue", "MAC"};

  TransferProperties geaTransfer = {batchSize_MiB * sizeof(float), true};

  std::map<std::pair<int, int>, Task> rc2Task;
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
    if (row == 0) {                       // first row
      // root node
      auto rootProp = geaNode;
      rootProp.name = getCordinateStr(row, 0);
      rootProp.op = OperatorTag::MAP_START;
      rootProp.opType = OperatorType::Map;
      rc2Task[{row, 0}] = boost::add_vertex(rootProp, g);
      for (int col = row + 1; col < N;
           ++col) { // N - (row + 1) operatrion each stage
        auto frProp = geaNode;
        frProp.name = getCordinateStr(row,col);
        rootProp.op = OperatorTag::VM;
        rootProp.opType = OperatorType::ComputeBound;
        rc2Task[{row, col}] = boost::add_vertex(frProp, g);
      }
    } else {
      for (int col = row; col < N; ++col) { 
        // N - (row + 1) operatrion each stage
        auto nodeProp = geaNode;
        nodeProp.name = getCordinateStr(row,col);
        rc2Task[{row, col}] = boost::add_vertex(nodeProp, g);
      }
    }
  }
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
    for (int col = row; col < N; ++col) { // N - (row + 1) operatrion each stage
      if (col == row) [[unlikely]] {      // main diagonal node.
        for (int i = col + 1; i < N; ++i) {
          if (rc2Task.contains({row, col}) && rc2Task.contains({row, i}))
            boost::add_edge(rc2Task[{row, col}], rc2Task[{row, i}], geaTransfer,
                            g);
        }
      } else [[likely]] { // off-diagonal nodes.
        if (rc2Task.contains({row, col}) && rc2Task.contains({row + 1, col}))
          boost::add_edge(rc2Task[{row, col}], rc2Task[{row + 1, col}],
                          geaTransfer, g);
      }
    }
  }

  // 输出DOT格式
  std::ofstream dot_file("graph.dot");
  boost::write_graphviz(dot_file, g, 
                        vertex_property_writer(g),
                        edge_property_writer(g));
  return {g};
}