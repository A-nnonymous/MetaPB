#include <iostream>
#include "Operator/OperatorManager.hpp"
#include "Operator/OperatorRegistry.hpp"
#include "Executor/TaskGraph.hpp"
#include "Scheduler/SchedulerManager.hpp"
#include "utils/typedef.hpp"

using TaskGraph = MetaPB::Executor::TaskGraph;
using TaskNode = MetaPB::Executor::TaskNode ;
using CPUOnlyScheduler = MetaPB::Scheduler::CPUOnlyScheduler;
using Graph = MetaPB::Executor::Graph;
using TaskProperties = MetaPB::Executor::TaskProperties;
using TransferProperties = MetaPB::Executor::TransferProperties;
using OperatorType = MetaPB::Operator::OperatorType;
using OperatorTag = MetaPB::Operator::OperatorTag;
using OperatorManager = MetaPB::Operator::OperatorManager;
using MetaPB::Operator::tag2Name;
using MetaPB::utils::regressionTask;

inline const std::string getCordinateStr(int row, int col) noexcept {
  return "[" + std::to_string(row) + "," + std::to_string(col) + "]";
}
TaskGraph genGEA(int matrixSize, size_t batchSize_MiB) {
  int N = matrixSize;
  Graph g;

  TaskProperties geaNode = {OperatorTag::MAC, OperatorType::CoumputeBound,
                            batchSize_MiB, "blue", "MAC"};
  TaskProperties geaDiagonal = {OperatorTag::DOT_PROD,
                                OperatorType::MemoryBound, batchSize_MiB,
                                "blue", "DOT_PROD"};

  TransferProperties geaTransfer = {1.0f, true};

  std::map<std::pair<int, int>, TaskNode> rc2Task;
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
    for (int col = row; col < N; ++col) {
      rc2Task[{row, col}] = boost::add_vertex(g);
    }
  }
  for (int row = 0; row < N - 1; ++row) { // N - 1 stage
    for (int col = row; col < N; ++col) { // N - (row + 1) operatrion each stage
      if (col == row) [[unlikely]] {      // main diagonal node.
        for (int i = col + 1; i < N; ++i) {
          if (rc2Task.contains({row, col}) && rc2Task.contains({row, i})) {
            g[rc2Task[{row, col}]] = geaDiagonal;
            g[rc2Task[{row, col}]].name = getCordinateStr(row, col);
            boost::add_edge(rc2Task[{row, col}], rc2Task[{row, i}], geaTransfer,
                            g);
          }
        }
      } else [[likely]] { // off-diagonal nodes.
        if (rc2Task.contains({row, col}) && rc2Task.contains({row + 1, col})) {
          g[rc2Task[{row, col}]] = geaNode;
          g[rc2Task[{row, col}]].name = getCordinateStr(row, col);
          boost::add_edge(rc2Task[{row, col}], rc2Task[{row + 1, col}],
                          geaTransfer, g);
        }
      }
    }
  }

  return {g, "GEA_" + std::to_string(N)};
}
int main(){
  auto g = genGEA(5,256);
  regressionTask rt = g.genRegressionTask();
  OperatorManager om;
  om.trainModel(rt);
  CPUOnlyScheduler s;
  MetaPB::Scheduler::HEFTScheduler he;
  for(const auto&[k,v]: rt){
    std::cout << "operator "<< tag2Name.at(k) << " train target: "<< v << " MiB" <<std::endl;
  }
  auto result = s.schedule(g, om);
  auto result1 = he.schedule(g, om);
  for(const auto& v : result.order){
    std::cout << g.g[v].name <<std::endl;
  }
  return 0;
}