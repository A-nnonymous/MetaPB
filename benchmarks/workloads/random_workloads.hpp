#ifndef RND_WORKLOADS
#define RND_WORKLOADS
#include "helpers/helper.hpp"
#include <random>

using namespace benchmarks;
using MetaPB::Executor::Graph;
using MetaPB::Executor::TaskNode;
using MetaPB::Executor::TaskProperties;
using MetaPB::Executor::TransferProperties;
using MetaPB::Operator::OperatorTag;
using MetaPB::Operator::OperatorType;
using MetaPB::Operator::memoryBoundOPSet;
using MetaPB::Operator::computeBoundOPSet;

consteval auto get_MO_edges() {
  return std::array{std::pair{0, 6},   std::pair{0, 7},   std::pair{0, 16},
                    std::pair{1, 6},   std::pair{1, 8},   std::pair{1, 17},
                    std::pair{2, 7},   std::pair{2, 8},   std::pair{2, 9},
                    std::pair{2, 10},  std::pair{2, 19},  std::pair{3, 9},
                    std::pair{3, 11},  std::pair{3, 12},  std::pair{3, 18},
                    std::pair{4, 10},  std::pair{4, 11},  std::pair{4, 13},
                    std::pair{4, 20},  std::pair{5, 12},  std::pair{5, 13},
                    std::pair{5, 21},  std::pair{6, 14},  std::pair{7, 14},
                    std::pair{8, 14},  std::pair{9, 14},  std::pair{10, 14},
                    std::pair{11, 14}, std::pair{12, 14}, std::pair{13, 14},
                    std::pair{14, 15}, std::pair{15, 16}, std::pair{15, 17},
                    std::pair{15, 18}, std::pair{15, 19}, std::pair{15, 20},
                    std::pair{15, 21}, std::pair{16, 22}, std::pair{17, 22},
                    std::pair{18, 22}, std::pair{19, 22}, std::pair{20, 22},
                    std::pair{21, 22}, std::pair{22, 23}, std::pair{16, 23},
                    std::pair{17, 23}, std::pair{18, 23}, std::pair{19, 23},
                    std::pair{20, 23}, std::pair{21, 23}};
}
consteval auto get_HO_edges() {
  return std::array{std::pair{0, 1}, std::pair{0, 2}, std::pair{0, 3},
                    std::pair{0, 4}, std::pair{1, 5}, std::pair{1, 6},
                    std::pair{2, 5}, std::pair{2, 7}, std::pair{3, 5},
                    std::pair{4, 6}, std::pair{4, 7}, std::pair{5, 8},
                    std::pair{6, 8}, std::pair{7, 8}};
}

TaskProperties randomProperty(const float CBRatio,const size_t batchSize_MiB){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    TaskProperties tp;
    const std::set<OperatorTag>& chosen_set = 
      dis(gen) < CBRatio
      ? computeBoundOPSet : memoryBoundOPSet;
    if (!chosen_set.empty()) {
        auto it = chosen_set.begin();
        std::advance(it, std::uniform_int_distribution<>(0, chosen_set.size() - 1)(gen));
        tp.op = *it;
        tp.opType = (chosen_set == computeBoundOPSet) ? OperatorType::ComputeBound : OperatorType::MemoryBound;
        tp.color = (chosen_set == computeBoundOPSet) ? "green" : "yellow";
        tp.inputSize_MiB = batchSize_MiB;
    }
    return tp;
}
TaskGraph randomizeHO(const float CBRatio, const size_t batchSize_MiB) {
  TransferProperties logicConnect = {1.0f, true};
  auto edges = get_HO_edges();
  Graph g(edges.begin(), edges.end(), 9);
  for (auto vp = vertices(g); vp.first != vp.second; ++vp.first) {
    g[*vp.first] = randomProperty(CBRatio, batchSize_MiB);
  }
  TaskProperties startNode = {OperatorTag::LOGIC_START, OperatorType::Logical,
                            0, "yellow", "START"};
  TaskProperties end = {OperatorTag::LOGIC_END, OperatorType::Logical,
                        batchSize_MiB, "black", "END"};
  auto lastNode = std::prev(vertices(g).second); 
  g[*lastNode] = end;
  g[0] = startNode;
  return {g, "H-"+ std::to_string(int(CBRatio * 100))};
}
TaskGraph randomizeMO(const float CBRatio, const size_t batchSize_MiB) {
  auto edges = get_MO_edges();
  Graph g(edges.begin(), edges.end(), 25);
  for (auto vp = vertices(g); vp.first != vp.second; ++vp.first) {
    g[*vp.first] = randomProperty(CBRatio, batchSize_MiB);
  }
  TaskProperties end = {OperatorTag::LOGIC_END, OperatorType::Logical,
                        batchSize_MiB, "black", "END"};
  auto lastNode = std::prev(vertices(g).second); 
  g[*lastNode] = end;
  TaskProperties startNode = {OperatorTag::LOGIC_START, OperatorType::Logical,
                            0, "yellow", "START"};
  g[0] = startNode;
  return {g, "M-"+ std::to_string(int(CBRatio * 100))};
}

#endif
