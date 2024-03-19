#include "Executor/HeteroComputePool.hpp"
#include "Executor/TaskGraph.hpp"
#include "Scheduler/MetaScheduler.hpp"
#include "Scheduler/HEFTScheduler.hpp"
#include "Operator/OperatorManager.hpp"
#include "Operator/OperatorRegistry.hpp"
#include "omp.h"
#include "utils/Stats.hpp"
#include "utils/typedef.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

using execType = MetaPB::Executor::execType;
using TaskGraph = MetaPB::Executor::TaskGraph;
using TaskNode = MetaPB::Executor::TaskNode;
using Graph = MetaPB::Executor::Graph;
using TaskProperties = MetaPB::Executor::TaskProperties;
using TransferProperties = MetaPB::Executor::TransferProperties;
using OperatorType = MetaPB::Operator::OperatorType;
using OperatorTag = MetaPB::Operator::OperatorTag;
using OperatorManager = MetaPB::Operator::OperatorManager;
using MetaPB::Executor::HeteroComputePool;
using MetaPB::Operator::tag2Name;
using MetaPB::utils::regressionTask;
using MetaPB::utils::Schedule;
using perfStats = MetaPB::utils::perfStats;

inline const std::string getCordinateStr(int row, int col) noexcept {
  return "[" + std::to_string(row) + "," + std::to_string(col) + "]";
}
TaskGraph genFFT(int signalLength, size_t batchSize_MiB) {
  std::uint32_t N = signalLength;
  // Bitwise magic to find next exp2, learned from a English AMDer
  N--;
  N |= N >> 1;
  N |= N >> 2;
  N |= N >> 4;
  N |= N >> 8;
  N |= N >> 16;
  N++;
  std::uint32_t log2N = std::log2(N);

  TaskProperties startNode = {OperatorTag::LOGIC_START, OperatorType::Logical,
                              0, "yellow", "START"};
  TaskProperties endNode = {OperatorTag::LOGIC_END, OperatorType::Logical, 0,
                            "yellow", "END"};

  TaskProperties fftNode = {OperatorTag::MAC, OperatorType::ComputeBound,
                            batchSize_MiB, "red", "MAC"};

  TransferProperties fftTransfer = {1.0f, true};

  TransferProperties logicDependence = {0, false};

  // log2N + 1 stage
  Graph g(N * (log2N + 1) + 2);

  for (int stage = 0; stage < log2N; ++stage) {
    int numBlocks = N / (1 << (stage + 1));
    int blockSize = 1 << (stage + 1);
    for (int block = 0; block < numBlocks; ++block) {
      for (int k = 0; k < blockSize / 2; ++k) {
        int u = block * blockSize + k + 1;
        int v = u + blockSize / 2;
        int uh, ul, vh, vl;
        uh = u + stage * N;
        ul = u + (stage + 1) * N;
        vh = v + stage * N;
        vl = v + (stage + 1) * N;
        boost::add_edge(uh, vl, fftTransfer, g);
        boost::add_edge(vh, ul, fftTransfer, g);
        boost::add_edge(uh, ul, fftTransfer, g);
        boost::add_edge(vh, vl, fftTransfer, g);
      }
    }
  }
  // 遍历所有顶点
  Graph::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
    g[*vi] = fftNode;
  }

  // Universial start and end point adding.
  for (int i = 0; i < N; i++) {
    boost::add_edge(0, i + 1, g);
    boost::add_edge(N * log2N + i + 1, N * (log2N + 1) + 1, g);
  }
  g[0] = startNode;
  g[N * (log2N + 1) + 1] = endNode;
  /*
 g[0] = fftNode;
  g[N * (log2N + 1) + 1] = fftNode;
  */

  return {g, "FFT_" + std::to_string(N)};
}
TaskGraph genGEA(int matrixSize, size_t batchSize_MiB) {
  int N = matrixSize;
  Graph g;

  TaskProperties geaNode = {OperatorTag::MAC, OperatorType::ComputeBound,
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

int main() {
  auto g = genFFT(8, 4096);
  regressionTask rt = g.genRegressionTask();
  OperatorManager om;
  om.trainModel(rt);
  MetaPB::Scheduler::HEFTScheduler he(g, om);
  MetaPB::Scheduler::MetaScheduler ms(1.0, 0.0, 50, g, om);
  std::cout <<"HEFT Scheduling\n";
  Schedule scheduleResult = he.schedule();
  std::cout <<"MetaScheduler Scheduling\n";
  Schedule comparison = ms.schedule();

  void **memPoolPtr = (void **)malloc(3);
#pragma omp parallel for
  for (int i = 0; i < 3; i++) {
    memPoolPtr[i] = malloc(4 * size_t(1 << 30)); // 2GiB
  }
  std::vector<perfStats> result(5);
  std::vector<HeteroComputePool> pools;
  for (int i = 0; i < 5; i++) {
    pools.emplace_back(std::move(
        HeteroComputePool{scheduleResult.order.size(), om, memPoolPtr}));
  }
#pragma omp parallel for
  for (int i = 0; i < 5; i++) {
    result[i] = pools[i].execWorkload(g, scheduleResult, execType::MIMIC);
  }

  perfStats real;
  real = pools[4].execWorkload(g, scheduleResult, execType::DO);
  pools[4].outputTimingsToCSV("./HEFT.csv");
  pools[4].execWorkload(g, comparison, execType::DO);
  pools[4].outputTimingsToCSV("./META.csv");
  std::cout << "Real perf: " << real.timeCost_Second
            << " second, "
               "Mimic perf: "
            << result[4].timeCost_Second << " second" << std::endl;
  std::cout << "Real energy: " << real.energyCost_Joule
            << " joule, "
               "Mimic energy: "
            << result[4].energyCost_Joule << "joule." << std::endl;
  std::cout << "Real xfer: " << real.dataMovement_MiB
            << " MiB,"
               "Mimic xfer: "
            << result[4].dataMovement_MiB << "MiB" << std::endl;
#pragma omp parallel for
  for (int i = 0; i < 3; i++) {
    free(memPoolPtr[i]);
  }
  // hep.outputTimingsToCSV("./gea_gantt.csv");

  /*
std::cout << "Execution Order: ";
for (int id : scheduleResult.order) {
    std::cout << id << " ";
}
std::cout << "\nOffload Ratios: ";
for (float ratio : scheduleResult.offloadRatio) {
    std::cout << ratio << " ";
}
std::cout << std::endl;
  */
  return 0;
}