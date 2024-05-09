#ifndef STRING_WL
#define STRING_WL
#include "helpers/helper.hpp"

using namespace benchmarks;
using MetaPB::Executor::Graph;
using MetaPB::Executor::TaskNode;
using MetaPB::Executor::TaskProperties;
using MetaPB::Executor::TransferProperties;
using MetaPB::Operator::OperatorTag;
using MetaPB::Operator::OperatorType;

TaskGraph genInterleavedWorkload(const size_t batchSize_MiB, int opNum) {
  TaskProperties start = {OperatorTag::LOGIC_START, OperatorType::Logical,
                          batchSize_MiB, "yellow", "START"};
  TaskProperties memBound = {OperatorTag::ELEW_ADD, OperatorType::MemoryBound,
                             batchSize_MiB, "blue", "ELEW_ADD"};
  TaskProperties computeBound = {OperatorTag::CONV_1D,
                                 OperatorType::ComputeBound, batchSize_MiB,
                                 "red", "CONV_1D"};
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
#endif
