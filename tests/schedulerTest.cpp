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

  TaskProperties fftNode = {OperatorTag::AFFINE, OperatorType::ComputeBound,
                            batchSize_MiB, "red", "AFFINE"};

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

  return {g, "FFT_" + std::to_string(N)};
}
TaskGraph genGEA(int matrixSize, size_t batchSize_MiB) {
  int N = matrixSize;
  Graph g;

  TaskProperties geaNode = {OperatorTag::AFFINE, OperatorType::ComputeBound,
                            batchSize_MiB, "blue", "AFFINE"};

  TaskProperties geaDiagonal = {OperatorTag::ELEW_PROD,
                                OperatorType::MemoryBound, batchSize_MiB,
                                "blue", "ELEW_PROD"};

  TaskProperties endNode = {OperatorTag::LOGIC_END, OperatorType::Logical, 0,
                            "yellow", "END"};

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
        } else if (row == (N - 2) && col == (N - 1)) {
          g[rc2Task[{row, col}]] = geaNode;
          g[rc2Task[{row, col}]].name = getCordinateStr(row, col);
          auto end = boost::add_vertex(endNode, g);
          boost::add_edge(rc2Task[{N - 2, N - 1}], end, geaTransfer, g);
        }
      }
    }
  }
  
  return {g, "GEA_" + std::to_string(N)};
}

TaskGraph genString(const size_t batchSize_MiB, int opNum) {
  TaskProperties start = {OperatorTag::LOGIC_START, OperatorType::Logical,
                        batchSize_MiB, "yellow", "START"};
  TaskProperties memBound = {OperatorTag::ELEW_PROD, OperatorType::MemoryBound,
                             batchSize_MiB, "blue", "ELEW_PROD"};
  TaskProperties computeBound = {OperatorTag::CONV_1D, OperatorType::ComputeBound,
                                 batchSize_MiB, "red", "CONV_1D"};
  TaskProperties end = {OperatorTag::LOGIC_END, OperatorType::Logical,
                        batchSize_MiB, "black", "END"};
  TransferProperties logicConnect = {1.0f, true};
  Graph g;
  auto prevNode = boost::add_vertex(start, g);
  for (int i = 1; i <= opNum; i++) {
    auto thisNode = boost::add_vertex(i % 2 ? computeBound : memBound, g);
    boost::add_edge(prevNode, thisNode, logicConnect, g);
    prevNode = thisNode;
  }
  auto endNode = boost::add_vertex(end, g);
  boost::add_edge(prevNode, endNode, logicConnect, g);
  return {g, "string_workload"};
}

int main() {
  void **memPoolPtr = (void **)malloc(3);
  memPoolPtr[0] = malloc(72 * size_t(1<<30));
  memPoolPtr[1] = memPoolPtr[0] + 1 * size_t(1<<30);
  memPoolPtr[2] = memPoolPtr[1] + 1 * size_t(1<<30);

  auto g = genGEA(8, 4096);
  //auto g = genFFT(8,4096);
  //auto g = genString(4096,3);
  g.printGraph("./");

  regressionTask rt = g.genRegressionTask();
  OperatorManager om;
  om.trainModel(rt);
  //MetaPB::Scheduler::HEFTScheduler he(g, om);
  MetaPB::Scheduler::MetaScheduler ms(0.5, 0.5, 50, g, om);
  HeteroComputePool hcp(boost::num_vertices(g.g),om, memPoolPtr);

  std::cout <<"MetaScheduler Scheduling\n";
  Schedule comparison = ms.schedule();
  
  /*
  Schedule scheduleResult = he.schedule();
  std::cout << "HEFT Schedule result: \n";
  for(int i = 0; i < scheduleResult.order.size(); i ++){
    std::cout << "task " << scheduleResult.order[i] << " -- " << scheduleResult.offloadRatio[i] << "\n";
  }



  perfStats heftStat, metaPBStat, cpuonlyStat, dpuonlyStat;
  for(int i = 0; i < 3; i++){
    heftStat = hcp.execWorkload(g, scheduleResult, execType::DO);
    hcp.outputTimingsToCSV("./HEFT.csv");
  }
  std::cout << "HEFT scheduler, time consume: "<< heftStat.timeCost_Second<<"second, energy consume: "<<       heftStat.energyCost_Joule<< " joule\n";
  for(int i = 0; i < 3; i++){
    metaPBStat = hcp.execWorkload(g, comparison, execType::DO);
    hcp.outputTimingsToCSV("./META.csv");
  }
  auto metaPBDeduce = hcp.execWorkload(g, comparison, execType::MIMIC);
  std::cout << "MetaPB scheduler, time consume: "<<metaPBStat.timeCost_Second <<"second, energy consume: "<<   metaPBStat.energyCost_Joule    <<  " joule\n";
  std::cout << "MetaPB scheduler, deduced time consume: "<<metaPBDeduce.timeCost_Second <<"second, deduced energy consume: "<<   metaPBDeduce.energyCost_Joule    <<  " joule\n";
  // CPU-only
  for(auto& ratio: comparison.offloadRatio){
    ratio = 0.0f;
  }
  for(int i = 0; i < 3; i++){
    cpuonlyStat = hcp.execWorkload(g, comparison, execType::DO);
    hcp.outputTimingsToCSV("./CPUOnly.csv");
  }
  std::cout << "CPUOnly scheduler, time consume: "<<cpuonlyStat.timeCost_Second <<"second, energy consume: "<< cpuonlyStat.energyCost_Joule    <<  " joule\n";
  // DPU-Only
  for(auto& ratio: comparison.offloadRatio){
    ratio = 1.0f;
  }
  comparison.isAlwaysWrittingBack = true;
  for(int i = 0; i < 3; i++){
    dpuonlyStat = hcp.execWorkload(g, comparison, execType::DO);
    hcp.outputTimingsToCSV("./DPUOnly.csv");
  }
  std::cout << "DPUOnly scheduler, time consume: "<< dpuonlyStat.timeCost_Second<<"second, energy consume: "<< dpuonlyStat.energyCost_Joule    << " joule\n";

  */
  free(memPoolPtr[0]);
  free((void*)memPoolPtr);
  return 0;
}
